#include <cstddef>
#include <cstdint>
#include <cstring>
#include <thread>

#include "augs/log.h"
#include "augs/log_path_getters.h"
#include "augs/window_framework/shell.h"
#include "augs/filesystem/file.h"
#include "augs/templates/introspect.h"
#include "augs/graphics/renderer.h"
#include "augs/templates/enum_introspect.h"
#include "augs/string/format_enum.h"
#include "augs/misc/enum/enum_array.h"
#include "augs/filesystem/directory.h"

#include "augs/misc/imgui/imgui_control_wrappers.h"
#include "augs/misc/imgui/imgui_enum_combo.h"
#include "augs/misc/imgui/imgui_enum_radio.h"
#include "augs/misc/imgui/imgui_utils.h"
#include "augs/audio/audio_context.h"

#include "augs/window_framework/platform_utils.h"
#include "augs/window_framework/window.h"

#include "view/necessary_resources.h"

#include "application/config_json_table.h"
#include "application/gui/settings_gui.h"
#include "augs/network/network_types.h"
#include "augs/audio/sound_sizes.h"
#include "application/gui/config_nvp.h"
#include "application/gui/do_server_vars.h"
#include "application/gui/pretty_tabs.h"
#include "application/setups/editor/editor_paths.h"
#include "augs/string/typesafe_sscanf.h"
#include "augs/window_framework/platform_utils.h"
#include "make_canon_config.hpp"

void configuration_subscribers::sync_back_into(config_json_table& into) const {
	window.sync_back_into(into.window);
}

void configuration_subscribers::apply(const config_json_table& new_config) const {
	DEBUG_DRAWING = new_config.debug_drawing;
	
	audio_context.apply(new_config.audio);
}

void configuration_subscribers::apply_main_thread(const augs::window_settings& settings) const {
	window.apply(settings);

	const auto screen_size = window.get_screen_size();
	fbos.apply(screen_size);
}

bool settings_gui_state::should_hijack_key() const {
	return hijacking.for_idx.has_value();
}

void settings_gui_state::set_hijacked_key(const augs::event::keys::key k) {
	hijacking.captured = k;
}

int performance_settings::get_default_num_pool_workers() {
#if PLATFORM_WEB
	return 1;
#endif

	const auto concurrency = static_cast<int>(std::thread::hardware_concurrency());

	const auto audio_threads = 1;
	const auto openal_threads = 1;
	const auto rendering_threads = 1;
	const auto main_threads = 1;

	const auto total_other_threads = 
		audio_threads
		+ rendering_threads
		+ main_threads
		+ openal_threads
	;

	return std::max(0, concurrency - total_other_threads);
}

int performance_settings::get_num_pool_workers() const {
	if (custom_num_pool_workers.is_enabled) {
		return std::max(0, custom_num_pool_workers.value);
	}

	return get_default_num_pool_workers();
}

#if BUILD_NATIVE_SOCKETS
#include "augs/network/netcode_utils.h"
#include "augs/templates/container_templates.h"

void stun_server_tester::advance() {
	constexpr auto max_num_sessions = 1;
	constexpr auto request_interval = 0.1;
	constexpr auto timeout = 0.4;

	while (current_sessions.size() < max_num_sessions) {
		if (provider.current_stun_server < static_cast<int>(provider.servers.size())) {
			current_sessions.emplace_back(std::make_unique<stun_session>(provider.get_next(), [](const std::string&){}));
		}
		else {
			break;
		}
	}

	auto handle = [&](const netcode_address_t&, uint8_t* packet_buffer, const int packet_bytes) {
		for (auto& session : current_sessions) {
			if (session->handle_packet(reinterpret_cast<const std::byte*>(packet_buffer), packet_bytes)) {
				return;
			}
		}
	};

	::receive_netcode_packets(socket.socket, handle);

	erase_if(current_sessions, [&](auto& session_ptr) {
		auto& session = *session_ptr;
		const auto state = session.get_current_state();

		if (auto new_packet = session.advance(request_interval, rng)) {
			packet_queue(*new_packet);
			packet_queue.send_one(socket.socket, make_LOG());
		}
		
		if (state == stun_session::state::COMPLETED) {
			const auto resolved_stun_host = *session.get_resolved_stun_host();
			const auto resolved_my_address = *session.query_result();

			bool duplicate = false;

			if (found_in(resolved_stun_hosts, resolved_stun_host)) {
				++num_duplicate_servers;
				duplicate = true;
			}
			else if (found_in(resolved_my_addresses, resolved_my_address)) {
				++num_duplicate_resolved_addresses;
				duplicate = true;
			}
			
			if (!duplicate) {
				auto new_one = std::tuple<double, std::string, std::string>();
				std::get<0>(new_one) = session.get_ping_seconds();
				std::get<1>(new_one) = session.host.address;
				std::get<2>(new_one) = ::ToString(resolved_my_address);

				resolved_servers.emplace(new_one);
				resolved_stun_hosts.emplace(resolved_stun_host);
				resolved_my_addresses.emplace(resolved_my_address);
			}

			return true;
		}
		else {
			if (session.has_timed_out(timeout)) {
				++num_failed_servers;
				return true;
			}

			if (state == stun_session::state::COULD_NOT_RESOLVE_STUN_HOST) {
				++num_failed_servers;
				return true;
			}
		}

		return false;
	});
}

stun_server_tester::stun_server_tester(const stun_server_provider& provider, port_type port) : socket(port), provider(provider) {

}

void stun_manager_window::perform() {
	using namespace augs::imgui;

	if (tester.has_value()) {
		tester->advance();
	}

	centered_size_mult = vec2(0.6f, 0.8f);

	auto manager = make_scoped_window(ImGuiWindowFlags_NoDocking);

	if (!manager) {
		return;
	}

	if (all_candidates == std::nullopt) {
		all_candidates.emplace(augs::path_type(DETAIL_DIR / "web/candidate_stun_servers.txt"));
	}

	if (ImGui::Button("Start analysis")) {
		if (all_candidates.has_value()) {
			tester.reset();
			auto rng = randomization::from_random_device();
			tester.emplace(*all_candidates, rng.randval(1024, 60000));
		}
	}

	std::string all_resolved_servers;
	std::string all_latencies;
	std::string all_ports;

	if (tester.has_value()) {
		for (const auto& resolved : tester->resolved_servers) {
			all_resolved_servers += std::get<1>(resolved) + "\n";
		}

		for (const auto& resolved : tester->resolved_servers) {
			all_latencies += typesafe_sprintf("%x ms", int(std::get<0>(resolved) * 1000)) + "\n";
		}

		for (const auto& resolved : tester->resolved_servers) {
			all_ports += typesafe_sprintf("%x", std::get<2>(resolved)) + "\n";
		}
	}

	ImGui::Columns(3);
	text("Resolved servers");

	{
		auto width = scoped_item_width(-1.0f);
		input_multiline_text<200000>("##Resolved servers", all_resolved_servers, 20, ImGuiInputTextFlags_ReadOnly);
	}

	ImGui::NextColumn();

	text("Latencies");

	{
		auto width = scoped_item_width(-1.0f);
		input_multiline_text<100000>("##Latencies", all_latencies, 20, ImGuiInputTextFlags_ReadOnly);
	}

	ImGui::NextColumn();

	text("Resolved ports");

	{
		auto width = scoped_item_width(-1.0f);
		input_multiline_text<100000>("##Ports", all_ports, 20, ImGuiInputTextFlags_ReadOnly);
	}

	ImGui::NextColumn();

	if (tester.has_value()) {
		const auto processed = tester->num_duplicate_resolved_addresses + tester->num_failed_servers + tester->num_duplicate_servers + tester->resolved_servers.size();

		text("Chosen source port: %x", tester->socket.socket.address.port);
		text("Processed %x out of %x servers.", processed, tester->provider.servers.size());
		text("Resolved %x servers.", tester->resolved_servers.size());
		text("Failed to resolve %x servers.", tester->num_failed_servers);
		text("Duplicate servers: %x", tester->num_duplicate_servers);
		text("Servers with duplicate resolved addresses: %x", tester->num_duplicate_resolved_addresses);
	}
}
#endif

std::string get_custom_binding_name(const game_intent_type intent) {
	switch (intent) {
		case game_intent_type::SHOOT:
			return "Shoot";
		case game_intent_type::SHOOT_SECONDARY:
			return "Shoot secondary/Weapon function";
		default:
			return "";
	}
}

std::string get_custom_binding_name(const general_gui_intent_type) {
	return "";
}

std::string get_custom_binding_name(const inventory_gui_intent_type) {
	return "";
}

std::string get_custom_binding_name(const app_intent_type) {
	return "";
}

void settings_gui_state::perform(
	augs::window& window,
	const augs::audio_context& audio,
	const augs::path_type& config_path_for_saving,
	const config_json_table& canon_config,
	const config_json_table& canon_config_with_confd,
	config_json_table& config,
	config_json_table& last_saved_config,
	vec2i screen_size
) {
	(void)window;

#if BUILD_NATIVE_SOCKETS
	stun_manager.perform();
#endif

	auto for_each_input_map = [&](auto callback) {
		callback(config.app_controls);
		callback(config.game_controls);
		callback(config.general_gui_controls);
		callback(config.inventory_gui_controls);
	};

	if (already_bound_popup) {
		const auto buttons = std::vector<simple_popup::button> {
			{
				"Reassign",
				rgba(200, 80, 0, 255),
				rgba(25, 20, 0, 255),
			},

			{
				"Cancel",
				rgba::zero,
				rgba::zero,
			}
		};

		if (const auto result = already_bound_popup->perform(buttons)) {
			if (result == 1) {
				for_each_input_map([&](auto& m) {
					m.erase(*reassignment_request.captured);
				});

				hijacking = reassignment_request;
				reassignment_request = {};
			}

			already_bound_popup = std::nullopt;
		}
	}

	using namespace augs::imgui;

	centered_size_mult = vec2::square(0.65f);
	
	auto settings = make_scoped_window(ImGuiWindowFlags_NoDocking);

	if (!settings) {
		return;
	}

	{
		auto child = scoped_child("settings view", ImVec2(0, -(ImGui::GetFrameHeightWithSpacing() + 4)));
		auto width = scoped_item_width(ImGui::GetWindowWidth() * 0.35f);

		int field_id = 0;

		do_pretty_tabs(active_pane, [](const settings_pane) -> std::optional<std::string> { 
			return std::nullopt; 
		});

		auto revert = make_revert_button_lambda(config, last_saved_config);

		auto revertable_checkbox = [&](auto l, auto& f, auto&&... args) {
			bool result = checkbox(l, f, std::forward<decltype(args)>(args)...);
			bool rresult = revert(f);

			return result || rresult;
		};

		auto revertable_slider = [&](auto l, auto& f, auto&&... args) {
			bool result = slider(l, f, std::forward<decltype(args)>(args)...);
			bool rresult = revert(f);
			return result || rresult;
		};

		auto revertable_drag_rect_bounded_vec2i = [&](auto l, auto& f, auto&&... args) {
			drag_rect_bounded_vec2i(l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};
		(void)revertable_drag_rect_bounded_vec2i;

		auto revertable_drag = [&](auto l, auto& f, auto&&... args) {
			drag(l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};
		(void)revertable_drag;

		auto revertable_drag_vec2 = [&](auto l, auto& f, auto&&... args) {
			drag_vec2(l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};

		auto revertable_color_edit = [&](auto l, auto& f, auto&&... args) {
			color_edit(l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};

		auto revertable_enum = [&](auto l, auto& f, auto&&... args) {
			enum_combo(l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};

		auto revertable_enum_rev = [&](auto l, auto& f, auto&&... args) {
			text(l);
			ImGui::SameLine();

			auto scope = scoped_item_width(150);
			enum_combo(std::string("###") + l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};

		auto revertable_enum_radio = [&](auto l, auto& f, auto&&... args) {
			std::string ss = l;
			cut_trailing(ss, "#0123456789");
			text(ss);

			auto scope = scoped_indent();

			enum_radio(f, std::forward<decltype(args)>(args)...);
			//revert(f);
		};

		auto revertable_input_text = [&](auto l, auto& f, auto&&... args) {
			input_text(l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};

		auto do_lag_simulator = [&](auto& sim) {
			//#if !IS_PRODUCTION_BUILD
			revertable_checkbox("Enable lag simulator", sim.is_enabled);

			if (sim.is_enabled) {
				auto& scope_cfg = sim.value;
				auto indent = scoped_indent();

				revertable_slider(SCOPE_CFG_NVP(latency_ms), 0.f, 200.f);
				revertable_slider(SCOPE_CFG_NVP(jitter_ms), 0.f, 100.f);
				revertable_slider(SCOPE_CFG_NVP(loss_percent), 0.f, 100.f);
				revertable_slider(SCOPE_CFG_NVP(duplicates_percent), 0.f, 99.f);
			}

			//#else
			// text_disabled("The network simulation is always disabled in production builds.");
			(void)sim;
			//#endif
		};

		auto do_server_settings = [&]() {
			/* This goes to Advanced */
			ImGui::Separator();

			text_color("Arenas", yellow);

			ImGui::Separator();

			do_server_vars(
				config.server,
				last_saved_config.server,
				rcon_pane::ARENAS
			);

			ImGui::Separator();

			do_server_vars(
				config.server,
				last_saved_config.server,
				rcon_pane::VARS
			);

#if !PLATFORM_WEB
			if (auto node = scoped_tree_node("API keys")) {
				auto& scope_cfg = config.server_private;

				thread_local bool show = false;
				const auto flags = show ? 0 : ImGuiInputTextFlags_Password; 

				input_text(SCOPE_CFG_NVP(steam_web_api_key), flags); ImGui::SameLine(); checkbox("Show", show); revert(scope_cfg.steam_web_api_key);
				
				if (ImGui::Button("Get your Steam API Key")) {
					 augs::open_url("https://steamcommunity.com/dev/apikey");
				}
				
				text_disabled("For authenticating Steam users on your server.");
			}
#endif

			
			if (auto node = scoped_tree_node("RCON")) {
				auto& scope_cfg = config.server_private;

				{
					thread_local bool show = false;
					const auto flags = show ? 0 : ImGuiInputTextFlags_Password; 

					input_text(SCOPE_CFG_NVP(rcon_password), flags); ImGui::SameLine(); checkbox("Show", show); revert(scope_cfg.rcon_password);
					text_disabled("A rcon can change maps, alter modes, kick/ban players and perform other administrative activities.");
				}

#if !PLATFORM_WEB
				{
					thread_local bool show = false;
					const auto flags = show ? 0 : ImGuiInputTextFlags_Password; 

					input_text(SCOPE_CFG_NVP(master_rcon_password), flags); revert(scope_cfg.master_rcon_password);
					text_disabled("A master rcon can additionally change the rcon password in case of an emergency.");
				}

				{
					auto& scope_cfg = config.server;
					revertable_checkbox(SCOPE_CFG_NVP(auto_authorize_loopback_for_rcon));
					revertable_checkbox("Auto authorize internal network clients for rcon", scope_cfg.auto_authorize_internal_for_rcon);
					tooltip_on_hover("Use cautiously. This will authorize clients coming from addresses\nlike 192.168.0.1 for total control over the server.\n\nUse only in trusted settings like in your home network.");
				}
#endif
			}
		};
		(void)do_server_settings;

		switch (active_pane) {
			case settings_pane::GENERAL: {
#if !PLATFORM_WEB
				text("At startup, launch.."); ImGui::SameLine();
				revertable_enum("##LaunchAtStartup", config.launch_at_startup);
#endif

				const auto streamer_hotkey = [&]() -> std::string {
					const auto found_k = key_or_default(config.app_controls, app_intent_type::TOGGLE_STREAMER_MODE);

					if (found_k == augs::event::keys::key()) {
						return "(UNASSIGNED)";
					}

					return key_to_string(found_k);
				}();

				auto label = std::string("Streamer mode");

				if (streamer_hotkey != "(UNASSIGNED)") {
					label += "(" + streamer_hotkey + ")";
				}

				revertable_checkbox(label, config.streamer_mode);

				tooltip_on_hover(typesafe_sprintf("Quickly toggle with %x at any time.\nCensors all player-provided content to prevent abuse during streams.\n\nWARNING!\nCommunity servers can host custom arenas with explicit sprites/sounds.\nTo be 100% secure, only play on the official servers -\nor on the servers of people you trust.", streamer_hotkey));

				if (config.streamer_mode) {
					auto ind = scoped_indent();

					auto& scope_cfg = config.streamer_mode_flags;

					text("Censor:");
					revertable_checkbox(SCOPE_CFG_NVP(chat));
					tooltip_on_hover("All messages of other players and their nicknames.");
					revertable_checkbox("Chat (when opened)", scope_cfg.chat_open);
					tooltip_on_hover("When off, shows uncensored chat after explicitly opening it with Y or U.");

					revertable_checkbox("Nicknames above characters", scope_cfg.inworld_hud);
					tooltip_on_hover("Player nicknames floating above your teammates.");

					revertable_checkbox(SCOPE_CFG_NVP(scoreboard));
					tooltip_on_hover("Nicknames and avatar on the scoreboard (available under Tab).");
					revertable_checkbox("Spectator UI", scope_cfg.spectator_ui);
					tooltip_on_hover("Nickname and avatar of the spectated player.");
					revertable_checkbox(SCOPE_CFG_NVP(kill_notifications));
					tooltip_on_hover("Nicknames in kill notifications at the top left.");
					revertable_checkbox(SCOPE_CFG_NVP(death_summary));
					tooltip_on_hover("Nickname and avatar of who killed you.");
					revertable_checkbox(SCOPE_CFG_NVP(community_servers));
					tooltip_on_hover("Server names and custom arena and game mode names.");
					revertable_checkbox(SCOPE_CFG_NVP(map_catalogue));
					tooltip_on_hover("Map descriptions and authors in the catalogue.");
				}

				if (config.streamer_mode) {
					ImGui::Separator();
				}

#if !PLATFORM_WEB
				revertable_checkbox("Fullscreen", config.window.fullscreen);

				if (!config.window.fullscreen) {
					auto indent = scoped_indent();

					{
						thread_local std::string width;
						thread_local std::string height;

						if (width.empty()) {
							width = std::to_string(config.window.size.x);
						}

						if (height.empty()) {
							height = std::to_string(config.window.size.y);
						}

						text("Dimensions");
						ImGui::SameLine();

						ImGui::PushItemWidth(ImGui::CalcTextSize("999999").x);

						if (input_text<10>("##Window width", width, ImGuiInputTextFlags_CharsDecimal)) {
						}

						ImGui::SameLine();

						text("x");

						ImGui::SameLine();

						if (input_text<10>("##Window height", height, ImGuiInputTextFlags_CharsDecimal)) {
						}

						ImGui::PopItemWidth();

						int cx = 0;
						int cy = 0;

						typesafe_sscanf(width, "%x", cx);
						typesafe_sscanf(height, "%x", cy);

						if (vec2i(cx, cy) != config.window.size) {
							ImGui::SameLine();

							if (ImGui::Button("Apply")) {
								config.window.size.set(cx, cy);

								if (config.window.size.x < 300) {
									config.window.size.x = 300;
								}

								if (config.window.size.y < 300) {
									config.window.size.y = 300;
								}
							}
						}
					}

					{
						thread_local std::string posx;
						thread_local std::string posy;

						if (posx.empty()) {
							posx = std::to_string(config.window.position.x);
						}

						if (posy.empty()) {
							posy = std::to_string(config.window.position.y);
						}

						text("Position");
						ImGui::SameLine();

						ImGui::PushItemWidth(ImGui::CalcTextSize("999999").x);

						if (input_text<10>("##Window posx", posx, ImGuiInputTextFlags_CharsDecimal)) {
						}

						ImGui::SameLine();

						if (input_text<10>("##Window posy", posy, ImGuiInputTextFlags_CharsDecimal)) {
						}

						ImGui::PopItemWidth();

						int cx = 0;
						int cy = 0;

						typesafe_sscanf(posx, "%x", cx);
						typesafe_sscanf(posy, "%x", cy);

						if (vec2i(cx, cy) != config.window.position) {
							ImGui::SameLine();

							const auto sz = display_size_for_clipping;

							if (ImGui::Button("Apply##Pos")) {
								config.window.position.set(cx, cy);

								config.window.position.x = std::clamp(config.window.position.x, 0, sz.x - 100);
								config.window.position.y = std::clamp(config.window.position.y, 0, sz.y - 100);
							}
						}
					}

					revertable_checkbox("Border", config.window.border);
				}

				revertable_checkbox("Draw own cursor in fullscreen", config.window.draw_own_cursor_in_fullscreen);

#if 0
				tooltip_on_hover("Try this if the cursor malfunctions for any reason.\nE.g. sometimes the system cursor disappears in fullscreen on Windows,\nor it breaks with fractional scaling on Wayland.");
#else
				tooltip_on_hover("Disable if the cursor malfunctions for any reason.");
#endif

#endif

				//input_text<100>(CONFIG_NVP(window.name), ImGuiInputTextFlags_EnterReturnsTrue); revert(config.window.name);

#if DEPRECATED
				revertable_checkbox("Auto-zoom", config.drawing.auto_zoom);

				tooltip_on_hover("The game will automatically zoom in or out\nto match the height of the player's field of vision.\n(1920x1080 on most maps).");

				if (!config.drawing.auto_zoom) {
					auto indent = scoped_indent();

					revertable_slider("Custom zoom", config.drawing.custom_zoom, 1.0f, 10.0f, "%.1f");
				}
#endif

				revertable_enum_rev("Vsync:", config.window.vsync_mode);

#if !PLATFORM_WEB
				{
					auto& mf = config.window.max_fps;

					revertable_checkbox("Limit maximum FPS", mf.is_enabled);

					if (mf.is_enabled) {
						auto scope = scoped_indent();

						revertable_slider("Max FPS", mf.value, 15, 400);
						revertable_enum("Method", config.window.max_fps_method);
					}
				}
#endif

				revertable_checkbox("Hide this window in-game", config.session.hide_settings_ingame);

				text("\n");
				ImGui::Separator();

#if !PLATFORM_WEB
				if (ImGui::Button("Open user folder in explorer")) {
					const auto config_path = USER_DIR / "config.json";

					if (!augs::exists(config_path)) {
						LOG("%x doesn't exist, create a stub", config_path);
						augs::save_as_text(config_path, "return {}");
					}

					window.reveal_in_explorer(config_path);
				}
#endif

				{
					auto bcol = rgba(200, 80, 0, 255);
					auto bincrement = rgba(25, 20, 0, 255);

					const auto cols = std::make_tuple(
						scoped_style_color(ImGuiCol_Button, bcol),
						scoped_style_color(ImGuiCol_ButtonHovered, bcol + bincrement), 
						scoped_style_color(ImGuiCol_ButtonActive, bcol + bincrement + bincrement)
					);

					if (ImGui::Button("Reset all settings to factory defaults")) {
						auto cvars = config.client;

						config = config_json_table(augs::path_type("default_config.json"));
						::make_canon_config(config, false);

#if IS_PRODUCTION_BUILD
						/*
							Don't break the currently signed in identity,
							although that is reassigned every frame.
						*/
						config.client.signed_in = cvars.signed_in;
						config.client.avatar_image_path = cvars.avatar_image_path;

#else
						(void)cvars;
#endif
					}
				}

				break;
			}
			case settings_pane::GRAPHICS: {
				text_color("Rendering", yellow);

				ImGui::Separator();

				enum_combo("Default filtering", config.renderer.default_filtering);

				{
					auto scope = scoped_indent();

					text_disabled(
						"The nearest neighbor filtering will give you a nostalgic, pixel-art feeling.\n"
						"The linear filtering is a little easier on the eyes.\n"
						"Linear filtering is enabled automatically when the camera zooms out."
					);
				}

				
				revertable_enum("Interpolation method", config.interpolation.method);
				
				if (config.interpolation.method == interpolation_method::EXPONENTIAL) {
					auto scope = scoped_indent();

					revertable_slider("Speed", config.interpolation.speed, 50.f, 1000.f);
				}

				//revertable_checkbox("Highlight hovered world items", config.drawing.draw_aabb_highlighter);

				ImGui::Separator();

				text_color("General Effects", yellow);

				ImGui::Separator();

				revertable_checkbox("Draw bullet shadows", config.drawing.draw_bullet_shadows);

				{
					auto& scope_cfg = config.performance.special_effects;
					revertable_slider(SCOPE_CFG_NVP(particle_stream_amount), 0.f, 1.f);
					revertable_slider(SCOPE_CFG_NVP(particle_burst_amount), 0.f, 1.f);
				}

				ImGui::Separator();

				text_color("Explosion effects", yellow);

				ImGui::Separator();

				{
					auto& scope_cfg = config.performance.special_effects.explosions;

					revertable_slider(SCOPE_CFG_NVP(sparkle_amount), 0.f, 1.f);
					revertable_slider(SCOPE_CFG_NVP(thunder_amount), 0.f, 1.f);
					revertable_slider(SCOPE_CFG_NVP(smoke_amount), 0.f, 1.f);
				}

				break;
			}
			case settings_pane::AUDIO: {
				text_color("Volume", yellow);

				ImGui::Separator();

				{
					auto& scope_cfg = config.audio_volume;

					revertable_slider(SCOPE_CFG_NVP(master), 0.f, 1.f);
					revertable_slider(SCOPE_CFG_NVP(sound_effects), 0.f, 1.f);
					revertable_slider(SCOPE_CFG_NVP(music), 0.f, 1.f);

					revertable_checkbox("Mute Main menu background", config.audio.mute_main_menu_background);
				}

				text_disabled("\n");

				ImGui::Separator();

				text_color("Spatialization", yellow);

				ImGui::Separator();

				revertable_enum("Output mode", config.audio.output_mode);

				text("Disable this for speakers!\nFor headphones, HRTF is the best.");

				{
					auto scope = scoped_indent(); 

					const auto stat = audio.get_device().get_hrtf_status();
					text(" Device HRTF status:");
					ImGui::SameLine();

					const auto col = stat.success ? green : red;
					text_color(stat.message, col);
#if !PLATFORM_WEB
					const auto output_mode = audio.get_device().get_output_mode();
					text(" Device output status:");
					ImGui::SameLine();
					text_color(output_mode, gray4);
#endif
				}

#if !PLATFORM_WEB
				text_disabled("If you experience a drop in sound quality with HRTF,\ntry setting the sample rate of your audio device to 44.1 kHz,\nor consider providing your own presets in detail/hrtf.");
#endif

				{

					auto& scope_cfg = config.sound;

					revertable_enum_radio("Keep listener position at:", scope_cfg.listener_reference);
					revertable_checkbox("Make listener face the same direction as character", scope_cfg.set_listener_orientation_to_character_orientation);
				}

				text_disabled("\n");

				ImGui::Separator();
				text_color("Sound physics", yellow);
				ImGui::Separator();

				revertable_slider("Speed of sound (m/s)", config.audio.sound_meters_per_second, 50.f, 400.f);
				revertable_slider("Max object speed for doppler calculation", config.sound.max_speed_for_doppler_calculation, 0.f, 10000.f);

				text_disabled("\n");

				ImGui::Separator();
				text_color("Sound quality", yellow);

				ImGui::Separator();

				{
					auto& scope_cfg = config.sound;

#if WEB_SINGLETHREAD
					revertable_slider(SCOPE_CFG_NVP(max_audio_commands_per_frame_ms), 0.5f, 10.f);
#endif
					revertable_enum_radio(SCOPE_CFG_NVP(processing_frequency));

					if (scope_cfg.processing_frequency == sound_processing_frequency::PERIODIC) {
						auto indent = scoped_indent();

						revertable_slider("Hz", scope_cfg.custom_processing_frequency, 1, 60);
					}

					revertable_slider(SCOPE_CFG_NVP(max_simultaneous_bullet_trace_sounds), 0, 20);
					revertable_slider(SCOPE_CFG_NVP(max_short_sounds), 0, static_cast<int>(SOUNDS_SOURCES_IN_POOL));

#if 0
					revertable_slider(SCOPE_CFG_NVP(missile_impact_sound_cooldown_duration), 1.f, 100.f);
					revertable_slider(SCOPE_CFG_NVP(missile_impact_occurences_before_cooldown), 0, 10);
#endif
				}

				break;
			}

			case settings_pane::CONTROLS: {
				auto& scope_cfg = config.input;

				{
					auto& scope_cfg = config.input.character;

					if (separate_sensitivity_axes == std::nullopt) {
						separate_sensitivity_axes = scope_cfg.crosshair_sensitivity.x != scope_cfg.crosshair_sensitivity.y;
					}

					auto& separate_axes = *separate_sensitivity_axes;

					if (separate_axes) {
						revertable_slider("Horizontal sensitivity", scope_cfg.crosshair_sensitivity.x, 0.1f, 5.0f, "%.3f");
						revertable_slider("Vertical sensitivity", scope_cfg.crosshair_sensitivity.y, 0.1f, 5.0f, "%.3f");
					}
					else {
						if (revertable_slider("Mouse sensitivity", scope_cfg.crosshair_sensitivity.x, 0.1f, 5.0f, "%.3f")) {
							scope_cfg.crosshair_sensitivity.y = scope_cfg.crosshair_sensitivity.x;
						}
					}

					if (checkbox("Separate sensitivity x & y", separate_axes)) {
						if (separate_axes == false) {
							scope_cfg.crosshair_sensitivity.y = scope_cfg.crosshair_sensitivity.x;
						}
					}

					revertable_checkbox(SCOPE_CFG_NVP(forward_moves_towards_crosshair));
				}

				if (auto node = scoped_tree_node("Advanced")) {
					{
						auto& scope_cfg = config.hotbar;

						revertable_checkbox(SCOPE_CFG_NVP(hide_unassigned_hotbar_buttons));
						//revertable_checkbox(SCOPE_CFG_NVP(autocollapse_hotbar_buttons));
					}

					revertable_checkbox(SCOPE_CFG_NVP(swap_mouse_buttons_in_akimbo));

					{
						auto& scope_cfg = config.input.game_gui;
						revertable_checkbox(SCOPE_CFG_NVP(allow_switching_to_bare_hands_as_previous_wielded_weapon));
					}

					{
						auto& scope_cfg = config.game_gui;

						revertable_checkbox(SCOPE_CFG_NVP(autodrop_magazines_of_dropped_weapons));
						// revertable_checkbox(SCOPE_CFG_NVP(autodrop_holstered_armed_explosives));
					}
				}

				const auto binding_text_color = rgba(255, 255, 255, 255);
				const auto bindings_title_color = rgba(255, 255, 0, 255);

				int control_i = 0;
				int map_i = 0;

				auto do_bindings_map = [&](const auto& preffix, auto& m) {
					auto id = scoped_id(map_i++);
					using K = typename remove_cref<decltype(m)>::key_type;
					using A = typename remove_cref<decltype(m)>::mapped_type;

					thread_local std::unordered_map<A, std::vector<K>> action_to_keys;
					action_to_keys.clear();

					for (auto&& ka : m) {
						action_to_keys[ka.second].push_back(ka.first);
					}

					ImGui::Separator();
					text_color(preffix, bindings_title_color);
					ImGui::NextColumn();
					ImGui::NextColumn();
					ImGui::NextColumn();
					ImGui::Separator();
					(void)m;
					
					augs::for_each_enum_except_bounds([&](const A a) {
						auto id = scoped_id(static_cast<int>(a));

						auto find_already_assigned_action_in = [&](const auto& mm, const auto key) -> std::optional<std::string> {
							if (const auto found = mapped_or_nullptr(mm, key)) {
								return format_enum(*found);
							}

							return std::nullopt;
						};

						auto find_already_assigned_action = [&](const auto key) -> std::optional<std::string> {
							if (const auto found = find_already_assigned_action_in(config.app_controls, key)) {
								return *found;
							}

							if (const auto found = find_already_assigned_action_in(config.game_controls, key)) {
								return *found;
							}

							if (const auto found = find_already_assigned_action_in(config.general_gui_controls, key)) {
								return *found;
							}

							if (const auto found = find_already_assigned_action_in(config.inventory_gui_controls, key)) {
								return *found;
							}

							return std::nullopt;
						};

						const bool capturing_this_action = hijacking.for_idx == control_i;
						const auto label = [&]() {
							const auto custom_name = get_custom_binding_name(a);

							if (custom_name != "") {
								return custom_name;
							}

							return format_enum(a);
						}();

						{
							auto scope = scoped_style_color(ImGuiCol_Text, binding_text_color);
							text(label);
						}

						ImGui::NextColumn();

						auto& bindings = action_to_keys[a];

						const auto first_key = [&]() -> std::optional<K> {
							if (bindings.size() > 0) {
								return bindings[0];
							}

							return std::nullopt;
						}();

						const auto second_key = [&]() -> std::optional<K> {
							if (bindings.size() > 1) {
								return bindings[1];
							}

							return std::nullopt;
						}();

						auto do_button = [&](const auto key, bool is_secondary) {
							const bool captured = capturing_this_action && is_secondary == hijacking.for_secondary;

							const auto label = [&]() -> std::string {
								if (captured) {
									return "Press a key, ESC to abort, Enter to clear...";
								}

								return key ? key_to_string(*key) : "(Unassigned)";
							}();

							const auto captured_bg_col = rgba(255, 20, 0, 80);

							const auto text_color = [&]() {
								if (captured) {
									return rgba(255, 40, 0, 255);
								}

								if (key) {
									return white;
								}

								return rgba(255, 255, 255, 120);
							}();

							const auto colors = std::make_tuple(
								cond_scoped_style_color(captured, ImGuiCol_Header, captured_bg_col),
								cond_scoped_style_color(captured, ImGuiCol_HeaderHovered, captured_bg_col),
								cond_scoped_style_color(captured, ImGuiCol_HeaderActive, captured_bg_col),
								scoped_style_color(ImGuiCol_Text, text_color)
							);

							if (ImGui::Selectable(label.c_str(), captured)) {
								if (!should_hijack_key()) {
									hijacking.for_idx = control_i;
									hijacking.for_secondary = is_secondary;
								}
							}
						};

						do_button(first_key, false);
						ImGui::NextColumn();
						do_button(second_key, true);
						ImGui::NextColumn();

						const bool captured_successfully = hijacking.for_idx && hijacking.captured;

						if (capturing_this_action && captured_successfully) {
							const auto new_key = *hijacking.captured;

							if (new_key != augs::event::keys::key::ESC) {
								const bool should_clear = new_key == augs::event::keys::key::ENTER;
								const auto found = find_already_assigned_action(new_key);

								auto make_popup = [&]() {
									const auto key_name = key_to_string(new_key);
									const auto current_action_str = format_enum(a);

									const auto description = typesafe_sprintf(
										"\"%x\" is already assigned to \"%x\"!\n\nPress \"Reassign\" to clear \"%x\" from \"%x\"\nand assign \"%x\" to \"%x\".\n\n",
										key_name, *found, key_name, *found, key_name, current_action_str
									);

									reassignment_request = hijacking;

									already_bound_popup = simple_popup {
										"Failed",
										description,
										""
									};
								};

								if (hijacking.for_secondary) {
									if (!should_clear && found) {
										make_popup();
									}
									else {
										if (second_key) {
											m.erase(*second_key);
										}

										if (!should_clear) {
											m[new_key] = a;
										}
									}
								}
								else {
									if (!should_clear && found) {
										make_popup();
									}
									else {
										if (first_key) {
											m.erase(*first_key);
										}

										if (!should_clear) {
											m[new_key] = a;
										}
									}
								}
							}

							hijacking = {};
						}

						++control_i;
					});
				};

				ImGui::Columns(3);

				text_disabled("Action");
				ImGui::NextColumn();

				text_disabled("Key");
				ImGui::NextColumn();

				text_disabled("Alternative key");
				ImGui::NextColumn();

				do_bindings_map("Combat controls", config.game_controls);
				text_disabled("\n\n");
				do_bindings_map("GUI controls", config.general_gui_controls);
				text_disabled("\n\n");
				do_bindings_map("Inventory controls", config.inventory_gui_controls);
				text_disabled("\n\n");
				do_bindings_map("General application controls", config.app_controls);
				text_disabled("\n\n");

				ImGui::Columns(1);

				ImGui::Separator();

				text_disabled("\n");

				if (ImGui::Button("Restore all keybindings from factory defaults")) {
					config.app_controls = canon_config.app_controls;
					config.game_controls = canon_config.game_controls;
					config.general_gui_controls = canon_config.general_gui_controls;
					config.inventory_gui_controls = canon_config.inventory_gui_controls;
				}


				break;
			}

			case settings_pane::GAMEPLAY: {
				{
					auto& scope_cfg = config.arena_mode_gui;
					revertable_checkbox("Show contextual tips", scope_cfg.context_tip_settings.is_enabled);

#if 0
					if (scope_cfg.context_tip_settings.is_enabled) {
						auto& scope_cfg = config.arena_mode_gui.context_tip_settings.value;
						auto indent = scoped_indent();

						revertable_slider(SCOPE_CFG_NVP(tip_offset_mult), 0.f, 1.f);

						revertable_color_edit(SCOPE_CFG_NVP(tip_text_color));
						revertable_color_edit(SCOPE_CFG_NVP(bound_key_color));
					}
#endif
				}

#if 0
				{
					auto& scope_cfg = config;
					revertable_checkbox("Show HUD messages", scope_cfg.hud_messages.is_enabled);

					if (scope_cfg.hud_messages.is_enabled) {
						auto& scope_cfg = config.hud_messages.value;
						auto indent = scoped_indent();

						revertable_slider(SCOPE_CFG_NVP(offset_mult), 0.f, 1.f);

						revertable_color_edit(SCOPE_CFG_NVP(text_color));
						revertable_slider(SCOPE_CFG_NVP(message_lifetime_secs), 1.0f, 20.0f);
						revertable_slider(SCOPE_CFG_NVP(message_fading_secs), 0.0f, 2.f);
						revertable_slider(SCOPE_CFG_NVP(max_simultaneous_messages), 1, 10);
					}
				}
#endif

				if (auto node = scoped_tree_node("Camera")) {
					auto& scope_cfg = config.camera;

#if 0
					// Disabled to properly synchronize spectator experience across clients

					revertable_slider(SCOPE_CFG_NVP(edge_zoom_out_zone), 0.0f, 0.4f);
					revertable_slider(SCOPE_CFG_NVP(edge_zoom_in_zone_expansion), 1.0f, 6.0f);
					revertable_slider(SCOPE_CFG_NVP(edge_zoom_in_cutoff_mult), 0.0f, 1.0f);
#endif

					revertable_slider(SCOPE_CFG_NVP(look_bound_expand), 0.0f, 0.5f);
					revertable_slider(SCOPE_CFG_NVP(surfing_zoom_out), 1.0f, 2.5f);
					
					revertable_checkbox(SCOPE_CFG_NVP(enable_smoothing));

					if (scope_cfg.enable_smoothing) {
						auto indent = scoped_indent();

						revertable_slider(SCOPE_CFG_NVP(smoothing.averages_per_sec), 0.f, 100.f); 
						revertable_slider(SCOPE_CFG_NVP(smoothing.average_factor), 0.01f, 0.95f); 
					}
				}

				{
					auto& scope_cfg = config.drawing;

					if (auto node = scoped_tree_node("HUD")) {
						revertable_checkbox("Cinematic mode", config.drawing.cinematic_mode);

						revertable_checkbox("Draw enemy silhouettes in Spectator", config.drawing.draw_enemy_silhouettes_in_spectator);

						revertable_checkbox(SCOPE_CFG_NVP(draw_inventory));
						revertable_checkbox(SCOPE_CFG_NVP(draw_hotbar));

						revertable_checkbox("Draw Health bar and ammo bar", config.drawing.draw_hp_bar);
						revertable_checkbox("Draw Personal Electricity bar", config.drawing.draw_pe_bar);
						revertable_checkbox("Draw Consciousness bar", config.drawing.draw_cp_bar);
						revertable_checkbox("Draw character status", config.drawing.draw_character_status);
						revertable_checkbox("Draw remaining ammo", config.drawing.draw_remaining_ammo);
						revertable_checkbox("Draw damage indicators", config.drawing.draw_damage_indicators);

						revertable_checkbox(SCOPE_CFG_NVP(draw_weapon_laser));
						revertable_checkbox(SCOPE_CFG_NVP(draw_crosshairs));
						revertable_checkbox(SCOPE_CFG_NVP(draw_nicknames));
						revertable_checkbox(SCOPE_CFG_NVP(draw_health_numbers));
						revertable_checkbox(SCOPE_CFG_NVP(draw_small_health_bars));

						revertable_checkbox("Draw teammate indicators", config.drawing.draw_teammate_indicators.is_enabled);

						if (config.drawing.draw_teammate_indicators.is_enabled) {
							auto indent = scoped_indent();
							revertable_slider("Alpha##colors", config.drawing.draw_teammate_indicators.value, 0.f, 1.f);
						}

						revertable_checkbox("Draw tactical indicators", config.drawing.draw_tactical_indicators.is_enabled);
						text_disabled("(for example, the dropped bomb's location)");

						if (config.drawing.draw_tactical_indicators.is_enabled) {
							auto indent = scoped_indent();
							revertable_slider("Alpha##colors2", config.drawing.draw_tactical_indicators.value, 0.f, 1.f);
						}

						revertable_checkbox("Draw danger indicators", config.drawing.draw_danger_indicators.is_enabled);

						if (config.drawing.draw_danger_indicators.is_enabled) {
							auto indent = scoped_indent();
							revertable_color_edit("Color", config.drawing.draw_danger_indicators.value);
						}

						revertable_checkbox(SCOPE_CFG_NVP(draw_offscreen_indicators));

						if (scope_cfg.draw_offscreen_indicators) {
							auto indent = scoped_indent();

							revertable_enum_radio(SCOPE_CFG_NVP(offscreen_reference_mode));
							revertable_checkbox(SCOPE_CFG_NVP(draw_offscreen_callouts));

							revertable_slider(SCOPE_CFG_NVP(nickname_characters_for_offscreen_indicators), 0, static_cast<int>(max_nickname_length_v));

							revertable_slider(SCOPE_CFG_NVP(show_danger_indicator_for_seconds), 0.f, 20.f);
							revertable_slider(SCOPE_CFG_NVP(fade_danger_indicator_for_seconds), 0.f, 20.f);

							revertable_slider(SCOPE_CFG_NVP(show_death_indicator_for_seconds), 0.f, 20.f);
							revertable_slider(SCOPE_CFG_NVP(fade_death_indicator_for_seconds), 0.f, 20.f);
						}

						revertable_checkbox("Draw callout indicators", config.drawing.draw_callout_indicators.is_enabled);
						revertable_checkbox("Draw current character callout", config.drawing.print_current_character_callout);

						if (config.drawing.draw_callout_indicators.is_enabled) {
							auto indent = scoped_indent();
							revertable_slider("Alpha##callouts", config.drawing.draw_callout_indicators.value, 0.f, 1.f);
						}

						revertable_checkbox("Draw area markers", config.drawing.draw_area_markers.is_enabled);

						if (config.drawing.draw_area_markers.is_enabled) {
							auto indent = scoped_indent();
							revertable_slider("Alpha##markers", config.drawing.draw_area_markers.value, 0.f, 1.f);
						}

						//revertable_drag_vec2(SCOPE_CFG_NVP(radar_pos));
					}
				}

				if (auto node = scoped_tree_node("Fog of war")) {
					auto& scope_cfg = config.drawing.fog_of_war_appearance;

					revertable_color_edit("Field of view overlay color", scope_cfg.overlay_color);

					text("Overlay color on:");

					{
						auto indent = scoped_indent();

						auto& f = scope_cfg.overlay_color_on_visible;
						if (ImGui::RadioButton("Visible area", f)) {
							f = true;
						}

						if (ImGui::RadioButton("The occlusion fog", !f)) {
							f = false;
						}
						text("\n");
						revert(f);
					}
				}

				{
					auto& scope_cfg = config.drawing.crosshair;

					if (auto node = scoped_tree_node("Crosshair")) {
						revertable_slider(SCOPE_CFG_NVP(scale), 0, 20);
						revertable_slider(SCOPE_CFG_NVP(border_width), 0, 20);

						revertable_checkbox(SCOPE_CFG_NVP(show_dot));

						if (scope_cfg.show_dot)
						{
							revertable_slider(SCOPE_CFG_NVP(dot_size), 0.f, 20.f);
						}

						revertable_slider(SCOPE_CFG_NVP(segment_length), 0.f, 200.f);
						revertable_slider(SCOPE_CFG_NVP(recoil_expansion_base), 0.f, 200.f);
						revertable_slider(SCOPE_CFG_NVP(recoil_expansion_mult), 0.f, 100.f);

						revertable_color_edit(SCOPE_CFG_NVP(inside_color));
						revertable_color_edit(SCOPE_CFG_NVP(border_color));
					}
				}

				// revertable_checkbox("Draw gameplay GUI", config.drawing.draw_character_gui); revert(config.drawing.draw_character_gui);
				break;
			}
			case settings_pane::CLIENT: {
				auto& scope_cfg = config.client;

				{
#if PLATFORM_WEB
					const auto label = "Nickname (guest)";
#else
					const auto label = typesafe_sprintf("Nickname (%x-%x characters)", min_nickname_length_v, max_nickname_length_v);
#endif

					revertable_input_text(label, scope_cfg.nickname);
				}

				{
					//const auto clan_label = typesafe_sprintf("Clan (0-20 characters)", 0, max_clan_length_v);
					revertable_input_text("Clan", scope_cfg.clan);
					tooltip_on_hover("Displayed e.g. in scoreboard.\nWill be more useful in the future.\n\nUp to 20 characters.");
				}

				if (scope_cfg.nickname.length() < min_nickname_length_v) {
					scope_cfg.nickname = "Player";
				}

				{
					thread_local bool show = false;
					const auto flags = show ? 0 : ImGuiInputTextFlags_Password; 

					input_text(SCOPE_CFG_NVP(rcon_password), flags); ImGui::SameLine(); checkbox("Show", show); revert(scope_cfg.rcon_password);
				}

				revertable_checkbox("Record demo", scope_cfg.record_demo);

#if 0
				if (scope_cfg.demo_recording_path.is_enabled) {
					auto scope = scoped_indent();
					
					revertable_slider(SCOPE_CFG_NVP(flush_demo_to_disk_once_every_secs), 1u, 120u);
				}
#endif

#if 0
				{
					auto& scope_cfg = config.arena_mode_gui;
					revertable_checkbox(SCOPE_CFG_NVP(show_client_resyncing_notifier));
				}
#endif

#if 0
				if (auto node = scoped_tree_node("Advanced")) {
					revertable_enum_radio("Spectate:", scope_cfg.spectated_arena_type, true);
					revertable_slider(SCOPE_CFG_NVP(max_buffered_server_commands), 0u, 10000u);
					revertable_slider(SCOPE_CFG_NVP(max_predicted_client_commands), 0u, 3000u);
				}
#endif

				revertable_slider("Max direct file bandwidth (per second)", scope_cfg.max_direct_file_bandwidth, 0.0f, 2.f, "%.2f MB");

				tooltip_on_hover("If the external provider does not have the hosted map,\nthe client will download it directly through UDP as a fallback mechanism.\nNote this will always be the case with Editor playtesting.");

				ImGui::Separator();

				text_color("Lag compensation", yellow);

				ImGui::Separator();

				do_lag_simulator(config.client.network_simulator);

				{
					{
						auto& scope_cfg = config.simulation_receiver;
						revertable_slider(SCOPE_CFG_NVP(misprediction_smoothing_multiplier), 0.f, 3.f);
					}

					{
						auto& scope_cfg = config.lag_compensation;
						revertable_checkbox(SCOPE_CFG_NVP(confirm_local_character_death));

						augs::introspect(
							[&](const std::string& label, auto& field){
								revertable_checkbox(format_field_name(label), field);
							},
							scope_cfg.effect_prediction
						); 
					}
				}


				{
					auto& scope_cfg = config.lag_compensation;
					revertable_checkbox(SCOPE_CFG_NVP(simulate_decorative_organisms_during_reconciliation));
				}

				ImGui::Separator();

				text_color("Jitter compensation", yellow);

				ImGui::Separator();

				{
					auto& scope_cfg = config.client.net.jitter;

					revertable_slider(SCOPE_CFG_NVP(buffer_at_least_steps), 0u, 10u);
					revertable_slider(SCOPE_CFG_NVP(buffer_at_least_ms), 0u, 100u);
					revertable_slider(SCOPE_CFG_NVP(max_commands_to_squash_at_once), uint8_t(0), uint8_t(255));
				}

				break;
			}
#if !WEB_LOWEND
			case settings_pane::EDITOR: {
				if (auto node = scoped_tree_node("Autosave")) {
					revertable_checkbox("Autosave when window loses focus", config.editor.autosave.on_lost_focus);
					revertable_checkbox("Autosave periodically", config.editor.autosave.periodically);

					if (config.editor.autosave.periodically) {
						auto scope = scoped_indent();
						text("Autosave once per");
						ImGui::SameLine();
						revertable_drag("minutes", config.editor.autosave.once_every_min, 0.002, 0.05, 2000.0);
					}

					text_disabled("(Note that when you exit the editor with unsaved changes,\nyour work will ALWAYS be autosaved)");
					
#if TODO
					text("Remember last");
					ImGui::SameLine();

					revertable_drag("commands for undoing", config.editor.remember_last_n_commands, 1, 10, 2000);
#endif

					ImGui::Separator();

					text("If loaded autosave, show: ");
					ImGui::SameLine();
					revertable_enum("##WhenAutosave", config.editor.autosave.if_loaded_autosave_show);
					revertable_checkbox("Alert when loaded autosave", config.editor.autosave.alert_when_loaded_autosave);

					if (config.editor.autosave.if_loaded_autosave_show == editor_autosave_load_option::AUTOSAVED_VERSION) {
						tooltip_on_hover("Whenever you open a project that has autosaved changes,\nthis will show a popup that forces you to click OK so that\nyou never miss the fact you've opened an autosave.\n\nYou can safely untick it if the popup annoys you.");
					}
					else {
						tooltip_on_hover("Whenever you open a project that has autosaved changes,\nthis will show a popup that forces you to click OK so that\nyou never miss the fact autosave is available.\n\nYou can safely untick it if the popup annoys you.");
					}
				}	
				
				if (auto node = scoped_tree_node("Interface")) {
					revertable_checkbox("Warp cursor when moving nodes with T", config.editor.warp_cursor_when_moving_nodes);

					if (auto node = scoped_tree_node("Action notifications")) {
						auto& scope_cfg = config.editor.action_notification;

						revertable_checkbox(SCOPE_CFG_NVP(enabled));

						if (scope_cfg.enabled) {
							revertable_slider(SCOPE_CFG_NVP(show_for_ms), 0u, 20000u);
							revertable_drag(SCOPE_CFG_NVP(offset.y));

							revertable_color_edit(SCOPE_CFG_NVP(bg_color));
							revertable_color_edit(SCOPE_CFG_NVP(bg_border_color));

							revertable_slider(SCOPE_CFG_NVP(max_width), 10u, 1000u);
							revertable_drag_vec2(SCOPE_CFG_NVP(text_padding));
						}
					}

					if (auto node = scoped_tree_node("Grid")) {
						auto& scope_cfg = config.editor.grid.render;

						if (auto lines = scoped_tree_node("Line colors")) {
							for (std::size_t i = 0; i < scope_cfg.line_colors.size(); ++i) {
								revertable_color_edit(std::to_string(i), scope_cfg.line_colors[i]);
							}
						}

						{
							auto& po2 = scope_cfg.maximum_power_of_two;

							revertable_slider("Maximum power of 2", po2, 3u, 20u);
							ImGui::SameLine();
							text(typesafe_sprintf("(Max grid size: %x)", 1 << po2));
						}

						revertable_slider("Alpha", scope_cfg.alpha_multiplier, 0.f, 1.f);
						revertable_slider(SCOPE_CFG_NVP(hide_grids_smaller_than), 0u, 128u);
					}

					if (auto node = scoped_tree_node("Camera")) {
						revertable_drag("Panning speed", config.editor.camera.panning_speed, 0.001f, -10.f, 10.f);
					}

#if 0
					if (auto node = scoped_tree_node("\"Go to\" dialogs")) {
						revertable_slider("Width", config.editor.go_to.dialog_width, 30u, static_cast<unsigned>(screen_size.x));
						revertable_slider("Number of lines to show", config.editor.go_to.num_lines, 1u, 300u);
					}
#endif

					if (auto node = scoped_tree_node("Entity selections")) {
						auto& scope_cfg = config.editor;
						revertable_checkbox(SCOPE_CFG_NVP(keep_source_nodes_selected_on_mirroring));
					}

					if (auto node = scoped_tree_node("Entity highlights")) {
						{
							auto& scope_cfg = config.editor.entity_selector;

							revertable_color_edit(SCOPE_CFG_NVP(hovered_color));
							revertable_color_edit(SCOPE_CFG_NVP(hovered_dashed_line_color));

							revertable_color_edit(SCOPE_CFG_NVP(selected_color));
							revertable_color_edit(SCOPE_CFG_NVP(held_color));
						}

#if 0
						{
							auto& scope_cfg = config.editor;

							revertable_color_edit(SCOPE_CFG_NVP(controlled_entity_color));
							revertable_color_edit(SCOPE_CFG_NVP(matched_entity_color));
						}
#endif
					}

				}

#if 0
				if (auto node = scoped_tree_node("Appearance")) {
					if (auto node = scoped_tree_node("Property editor")) {
						auto& scope_cfg = config.editor.property_debugger;

						revertable_color_edit(SCOPE_CFG_NVP(different_values_frame_bg));
						revertable_color_edit(SCOPE_CFG_NVP(different_values_frame_hovered_bg));
						revertable_color_edit(SCOPE_CFG_NVP(different_values_frame_active_bg));
					}

					auto& scope_cfg = config.editor;

					revertable_color_edit(SCOPE_CFG_NVP(tutorial_text_color));
					revertable_color_edit(SCOPE_CFG_NVP(rectangular_selection_color));
					revertable_color_edit(SCOPE_CFG_NVP(rectangular_selection_border_color));
				}

				if (auto node = scoped_tree_node("Player")) {
					auto& scope_cfg = config.editor.player;

					revertable_slider(SCOPE_CFG_NVP(snapshot_interval_in_steps), 400u, 5000u);
				}

				if (auto node = scoped_tree_node("Debug")) {
					auto& scope_cfg = config.editor;

					revertable_checkbox(SCOPE_CFG_NVP(save_entropies_to_live_file));
				}
#endif

				if (auto node = scoped_tree_node("Uploading")) {
					auto& scope_cfg = config.editor;

					thread_local bool show = false;
					const auto flags = show ? 0 : ImGuiInputTextFlags_Password; 

					input_text("Upload API key", scope_cfg.upload_api_key, flags); ImGui::SameLine(); checkbox("Show", show); revert(scope_cfg.upload_api_key);

					revertable_input_text("Upload URL", scope_cfg.upload_url);
				}

				break;
			}
#endif

			case settings_pane::INTERFACE: {
				revertable_slider("UI scale", config.ui_scale, 1.0f, 4.0f);
				config.ui_scale = std::clamp(config.ui_scale, 1.0f, 4.0f);

				if (auto node = scoped_tree_node("Chat window")) {
					auto& scope_cfg = config.client.client_chat;

					revertable_slider(SCOPE_CFG_NVP(chat_window_width), 100u, 500u);
#if 0
					revertable_drag_rect_bounded_vec2i(SCOPE_CFG_NVP(chat_window_offset), 0.3f, -vec2i(screen_size), vec2i(screen_size));
#endif

					revertable_slider(SCOPE_CFG_NVP(show_recent_chat_messages_num), 0u, 30u);
					revertable_slider(SCOPE_CFG_NVP(keep_recent_chat_messages_for_seconds), 0.f, 30.f);
				}

				if (auto node = scoped_tree_node("Advanced")) {
					if (auto node = scoped_tree_node("In-game HUD skin")) {
						if (auto node = scoped_tree_node("Scoreboard")) {
							auto scope = scoped_indent();
							auto& scope_cfg = config.arena_mode_gui.scoreboard_settings;

							revertable_slider(SCOPE_CFG_NVP(cell_bg_alpha), 0.f, 1.f);
							
							revertable_drag_vec2(SCOPE_CFG_NVP(player_row_inner_padding), 1.f, 0, 20);

							revertable_color_edit(SCOPE_CFG_NVP(background_color));
							revertable_color_edit(SCOPE_CFG_NVP(border_color));

							revertable_slider(SCOPE_CFG_NVP(bg_lumi_mult), 0.f, 5.f);
							revertable_slider(SCOPE_CFG_NVP(text_lumi_mult), 0.f, 5.f);
							revertable_slider(SCOPE_CFG_NVP(current_player_bg_lumi_mult), 0.f, 5.f);
							revertable_slider(SCOPE_CFG_NVP(current_player_text_lumi_mult), 0.f, 5.f);
							revertable_slider(SCOPE_CFG_NVP(dead_player_bg_lumi_mult), 0.f, 1.f);
							revertable_slider(SCOPE_CFG_NVP(dead_player_bg_alpha_mult), 0.f, 1.f);
							revertable_slider(SCOPE_CFG_NVP(dead_player_text_alpha_mult), 0.f, 1.f);
							revertable_slider(SCOPE_CFG_NVP(dead_player_text_lumi_mult), 0.f, 1.f);

							revertable_slider(SCOPE_CFG_NVP(text_stroke_lumi_mult), 0.f, 1.f);
							revertable_slider(SCOPE_CFG_NVP(faction_logo_alpha), 0.f, 1.f);
							revertable_checkbox(SCOPE_CFG_NVP(dark_color_overlay_under_score));
						}

						if (auto node = scoped_tree_node("Buy menu")) {
							auto scope = scoped_indent();
							auto& scope_cfg = config.arena_mode_gui.buy_menu_settings;

							revertable_color_edit(SCOPE_CFG_NVP(disabled_bg));
							revertable_color_edit(SCOPE_CFG_NVP(disabled_active_bg));

							revertable_color_edit(SCOPE_CFG_NVP(already_owns_bg));
							revertable_color_edit(SCOPE_CFG_NVP(already_owns_active_bg));

							revertable_color_edit(SCOPE_CFG_NVP(already_owns_other_type_bg));
							revertable_color_edit(SCOPE_CFG_NVP(already_owns_other_type_active_bg));
						}

						auto& scope_cfg = config.arena_mode_gui;

						if (auto node = scoped_tree_node("Knockouts indicators")) {
							auto scope = scoped_indent();

							revertable_slider(SCOPE_CFG_NVP(between_knockout_boxes_pad), 0u, 20u);
							revertable_slider(SCOPE_CFG_NVP(inside_knockout_box_pad), 0u, 20u);
							revertable_slider(SCOPE_CFG_NVP(weapon_icon_horizontal_pad), 0u, 20u);
							revertable_slider(SCOPE_CFG_NVP(show_recent_knockouts_num), 0u, 20u);
							revertable_slider(SCOPE_CFG_NVP(keep_recent_knockouts_for_seconds), 0.f, 20.f);
							revertable_slider("Max weapon icon height (0 for no limit)", scope_cfg.max_weapon_icon_height, 0u, 100u);
						}

						if (auto node = scoped_tree_node("Money indicator")) {
							auto scope = scoped_indent();

							drag("Money indicator position X", scope_cfg.money_indicator_pos.x, 0.3f, -vec2i(screen_size).x, vec2i(screen_size).x);
							drag("Money indicator position Y", scope_cfg.money_indicator_pos.y, 0.3f, -vec2i(screen_size).y, vec2i(screen_size).y);

							revertable_color_edit("Money indicator color", scope_cfg.money_indicator_color);
							revertable_color_edit("Award indicator color", scope_cfg.award_indicator_color);
							revertable_slider(SCOPE_CFG_NVP(show_recent_awards_num), 0u, 20u);
							revertable_slider(SCOPE_CFG_NVP(keep_recent_awards_for_seconds), 0.f, 20.f);
						}
					}

					if (auto node = scoped_tree_node("ImGUI style editor")) {
						text(
							"This is the ImGUI-provided style tweaker.\n"
							"To save your changes to the local configuration file,\n"
							"You need to push Save Ref and only then Save settings at the bottom."
						);

						ImGui::Separator();
						ImGui::ShowStyleEditor(&config.gui_style);
					}
				}

				break;
			}

			case settings_pane::ADVANCED: {
#if !WEB_LOWEND
				if (auto node = scoped_tree_node("Server settings")) {
					do_server_settings();
				}
#endif

#if PLATFORM_UNIX
				revertable_checkbox("Map CAPS LOCK to ESC", config.window.map_caps_lock_to_esc);
#endif

				ImGui::Separator();

				text_color("Masterserver", yellow);

				ImGui::Separator();

				input_text("Server list provider", config.server_list_provider, ImGuiInputTextFlags_EnterReturnsTrue); revert(config.server_list_provider);
				input_text("WebRTC signalling server", config.webrtc_signalling_server_url, ImGuiInputTextFlags_EnterReturnsTrue); revert(config.webrtc_signalling_server_url);

#if 0
				input_text("Port probing host", config.nat_detection.port_probing.host.address, ImGuiInputTextFlags_EnterReturnsTrue); revert(config.nat_detection.port_probing.host.address);
#endif

#if !PLATFORM_WEB
				ImGui::Separator();

				text_color("Automatic updates", yellow);

				ImGui::Separator();

				{
					auto& scope_cfg = config.self_update;

					revertable_checkbox("Automatically update when the game starts", scope_cfg.update_on_launch);
					revertable_slider(SCOPE_CFG_NVP(update_connection_timeout_secs), 1, 3600);

					input_text<100>(SCOPE_CFG_NVP(update_host), ImGuiInputTextFlags_EnterReturnsTrue); revert(config.self_update.update_host);
					input_text<100>(SCOPE_CFG_NVP(update_path), ImGuiInputTextFlags_EnterReturnsTrue); revert(config.self_update.update_path);

				}

#endif
				ImGui::Separator();

				text_color("Rendering", yellow);

				ImGui::Separator();

				revertable_checkbox("Stencil before light pass", config.drawing.stencil_before_light_pass);
				tooltip_on_hover("If your field of view is glitched (disappearing enemies),\ntry toggling on and off.");

				{
					auto& scope_cfg = config.drawing;
					revertable_checkbox(SCOPE_CFG_NVP(occlude_neons_under_sentiences));
				}


				{
					auto& scope_cfg = config.performance;
					revertable_enum_radio(SCOPE_CFG_NVP(wall_light_drawing_precision));
				}

				ImGui::Separator();

#if !PLATFORM_WINDOWS
				{
					auto s = std::basic_string<char8_t>(u8"Test: いい товарищ żółćńźś");

					std::string u8str;

					for (auto c : s) {
						u8str += c;
					}

					text(u8str);
				}
#endif
				ImGui::Separator();

#if BUILD_NATIVE_SOCKETS
				{
					auto& scope_cfg = config.nat_traversal;
					auto& st = scope_cfg.short_ttl;
					revertable_checkbox("Enable short TTL for traversal packets", st.is_enabled);

					if (st.is_enabled) {
						auto indent = scoped_indent();
						revertable_slider("TTL", st.value, 1, 255);
					}

					revertable_slider("Num brute-force packets during NAT traversal", scope_cfg.num_brute_force_packets, 0, 25);
				}

				{
					auto& scope_cfg = config.debug;
					revertable_checkbox(SCOPE_CFG_NVP(log_solvable_hashes));
				}
#endif

				revertable_checkbox("Show performance", config.session.show_performance);
				revertable_checkbox("Show logs", config.session.show_logs);
				revertable_checkbox("Log keystrokes", config.window.log_keystrokes);
#if PLATFORM_WEB || !IS_PRODUCTION_BUILD
				revertable_checkbox("Suppress client webhooks", config.client.suppress_webhooks);
#endif
				revertable_slider("Camera query aabb mult", config.session.camera_query_aabb_mult, 0.10f, 5.f);
				
				revertable_checkbox("Draw debug lines", config.debug_drawing.enabled);

				if (config.debug_drawing.enabled) {
					auto indent = scoped_indent();

					augs::introspect(
						[&](const std::string& label, auto& field){
							if (label != "enabled") {
								revertable_checkbox(format_field_name(label), field);
							}
						},
						config.debug_drawing
					); 
				}

#if BUILD_TEST_SCENES
				text("Test scenes (%x)", "built-in");
#else
				text("Test scenes (%x)", "not built-in");
#endif

				{
					auto indent = scoped_indent();

					revertable_checkbox("Create minimal", config.test_scene.create_minimal);
					revertable_slider("Tickrate", config.test_scene.scene_tickrate, 10u, 300u);
				}

				{
					auto& scope_cfg = config.debug;
					revertable_checkbox(SCOPE_CFG_NVP(measure_atlas_uploading));
				}

#if !PLATFORM_WEB
				text("Content regeneration");

				{
					auto indent = scoped_indent();
					auto& scope_cfg = config.content_regeneration;
					revertable_checkbox(SCOPE_CFG_NVP(regenerate_every_time));
					revertable_checkbox(SCOPE_CFG_NVP(rescan_assets_on_window_focus));

					const auto concurrency = std::thread::hardware_concurrency();
					const auto t_max = concurrency * 2;

					text_disabled("(Value of 0 tells regenerators to not spawn any additional workers)");
					text_disabled(typesafe_sprintf("(std::thread::hardware_concurrency() = %x)", concurrency));

					revertable_slider(SCOPE_CFG_NVP(atlas_blitting_threads), 1u, t_max);
					revertable_slider(SCOPE_CFG_NVP(neon_regeneration_threads), 1u, t_max);
				}
#endif

#if !PLATFORM_WEB
				ImGui::Separator();

				text_color("Multithreading", yellow);

				ImGui::Separator();

				{
					auto& scope_cfg = config.performance;

					revertable_enum_radio(SCOPE_CFG_NVP(swap_window_buffers_when));

					const auto concurrency = static_cast<int>(std::thread::hardware_concurrency());

					{
						text("Concurrent hardware threads:");
						ImGui::SameLine();
						text_color(typesafe_sprintf("%x", concurrency), green);
					}

					{
						const auto default_n = performance_settings::get_default_num_pool_workers();
						text("Default number of thread pool workers:");
						ImGui::SameLine();
						text_color(typesafe_sprintf("%x\n\n", default_n), default_n == 0 ? red : green);
					}


					{
						auto& cn = scope_cfg.custom_num_pool_workers;
						revertable_checkbox("Custom number of thread pool workers", cn.is_enabled);

						if (cn.is_enabled) {
							auto indent = scoped_indent();
							revertable_slider("##ThreadCount", cn.value, 0, concurrency * 3);
						}
					}

					revertable_slider(SCOPE_CFG_NVP(max_particles_in_single_job), 1000, 20000);
				}

				ImGui::Separator();

				text_color("Info", yellow);

				ImGui::Separator();

				const auto cwd = augs::get_current_working_directory();
				text("Working directory: %x", cwd, std::filesystem::absolute(cwd));
				text("Cache folder location: %x (%x)", CACHE_DIR, std::filesystem::absolute(CACHE_DIR));
				text("User folder location: %x (%x)", USER_DIR, std::filesystem::absolute(USER_DIR));

				if (ImGui::Button("Dump debug log")) {
					get_dumped_log_path();
					
					const auto logs = program_log::get_current().get_complete();
					auto failure_log_path = augs::path_type(get_dumped_log_path());

					augs::save_as_text(failure_log_path, logs);

					augs::open_text_editor(failure_log_path.string());
					augs::open_text_editor(failure_log_path.replace_filename("").string());
				}

				ImGui::SameLine();
#endif

#if BUILD_NATIVE_SOCKETS
				if (ImGui::Button("Open STUN manager")) {
					stun_manager.open();
				}
#endif

				break;
			}
			default: {
				ensure(false && "Unknown settings pane type");
				break;
			}
		}
	}

	{
		auto scope = scoped_child("save revert");

		ImGui::Separator();

		if (config != last_saved_config) {
			if (ImGui::Button("Save changes")) {
				augs::timer save_timer;
				last_saved_config = config;
				config.save_patch(canon_config_with_confd, config_path_for_saving, true);
				LOG("Saved new config in: %x ms", save_timer.get<std::chrono::milliseconds>());
			}

			ImGui::SameLine();

			if (ImGui::Button("Undo changes")) {
				config = last_saved_config;
				ImGui::GetStyle() = config.gui_style;
			}
		}
	}
}

#include "application/gui/arena_chooser.h"
#include "application/arena/arena_paths.h"
#include "application/gui/config_nvp.h"
#include "application/setups/editor/resources/editor_game_mode_resource.h"

bool perform_game_mode_chooser(game_mode_name_type& current_arena, const std::string& caption = "Game mode");

bool perform_game_mode_chooser(
	game_mode_name_type& current_mode,
	const std::string& caption
) {
	const auto displayed_str = current_mode.empty() ? std::string("(Map default)") : std::string(current_mode);

	bool chosen = false;

	if (auto combo = augs::imgui::scoped_combo(caption.c_str(), displayed_str.c_str(), ImGuiComboFlags_HeightLargest)) {
		auto do_entry = [&](const std::string& value, const std::string& displayed_value) {
			const bool is_current = value == current_mode;

			if (ImGui::Selectable(displayed_value.c_str(), is_current)) {
				ImGui::CloseCurrentPopup();

				current_mode = value;
				chosen = true;

				LOG("Mode selected: %x", displayed_value);
			}
		};

		do_entry("", "(Map default)");

		for_each_type_in_list<editor_all_game_modes>([&]<typename M>(M) {
			do_entry(M::get_identifier(), M::get_display_name());
		});
	}

	return chosen;
}

bool perform_arena_chooser(
	arena_identifier& current_arena,
	const server_runtime_info* info
) {
	thread_local arena_chooser chooser;

	bool changed = false;

	chooser.perform(
		"Arena",
		current_arena,
		OFFICIAL_ARENAS_DIR,
		DOWNLOADED_ARENAS_DIR,
		EDITOR_PROJECTS_DIR,
		info,
		[&](const auto& chosen_arena_path) {
			current_arena = chosen_arena_path.filename().string();
			changed = true;
		}
	);

	return changed;
}

void do_server_vars(
	server_vars& vars,
	server_vars& last_saved_vars,
	rcon_pane pane,
	const server_runtime_info* runtime_info
) {
	using namespace augs::imgui;

	auto revert = make_revert_button_lambda(vars, last_saved_vars);

	auto revertable_enum = [&](auto l, auto& f, auto&&... args) {
		enum_combo(l, f, std::forward<decltype(args)>(args)...);
		revert(f);
	};

	auto revertable_checkbox = [&](auto l, auto& f, auto&&... args) {
		checkbox(l, f, std::forward<decltype(args)>(args)...);
		revert(f);
	};

	auto revertable_slider = [&](auto l, auto& f, auto&&... args) {
		slider(l, f, std::forward<decltype(args)>(args)...);
		revert(f);
	};

	if (pane == rcon_pane::ARENAS) {
		if (perform_arena_chooser(vars.arena, runtime_info)) {
			vars.game_mode = "";
		}

		revert(vars.arena);

		perform_game_mode_chooser(vars.game_mode);
		revert(vars.game_mode);

		text("\n");

		ImGui::Separator();
		text_color("Cycle", yellow);
		ImGui::Separator();

		revertable_enum("Cycle type", vars.cycle);

		if (vars.cycle == arena_cycle_type::LIST || vars.cycle == arena_cycle_type::ALL_ON_DISK) {
			text("Force a specific game mode when cycling");
			ImGui::SameLine();
			perform_game_mode_chooser(vars.cycle_always_game_mode, "##ForceChooser");

			revert(vars.cycle_always_game_mode);

			revertable_checkbox("Randomize order", vars.cycle_randomize_order);
			revertable_slider("When idle, change map once every minutes", vars.when_idle_change_maps_once_every_mins, float(0.0f), float(120.0f));
			tooltip_on_hover("0 disables changing on idle.");
		}

		if (vars.cycle == arena_cycle_type::LIST) {
			ImGui::Separator();
			text_color("Cycle arena list", yellow);
			ImGui::Separator();

			std::optional<std::size_t> to_remove;

			if (ImGui::Button("Add arena to list")) {
				vars.cycle_list.push_back("de_cyberaqua");
			}

			ImGui::SameLine();

			if (ImGui::Button("Clear list")) {
				vars.cycle_list.clear();
			}

			for (std::size_t i = 0; i < vars.cycle_list.size(); ++i) {
				auto scope = scoped_id(i);

				auto& entry = vars.cycle_list[i];

				if (ImGui::Button("-")) {
					to_remove = i;
				}

				ImGui::SameLine();

				bool modified = false;

				auto arena = arena_identifier(::get_first_word(entry));
				auto mode = game_mode_name_type(::get_second_word(entry));

				if (perform_arena_chooser(arena, runtime_info)) {
					mode = "";

					modified = true;
				}

				ImGui::SameLine();

				if (perform_game_mode_chooser(mode)) {
					modified = true;
				}

				if (modified) {
					if (mode.empty()) {
						entry = arena.operator std::string();
					}
					else {
						entry = typesafe_sprintf("%x %x", arena, mode);
					}
				}
			}

			if (to_remove.has_value()) {
				vars.cycle_list.erase(vars.cycle_list.begin() + *to_remove);
			}
		}

		text_disabled("\n");
	}
	else if (pane == rcon_pane::VARS) {
		int field_id = 999998;

		auto& scope_cfg = vars;

		auto revertable_input_text = [&](auto l, auto& f, auto&&... args) {
			input_text(l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};

		auto revertable_checkbox = [&](auto l, auto& f, auto&&... args) {
			checkbox(l, f, std::forward<decltype(args)>(args)...);
			revert(f);
		};

		if (auto node = scoped_tree_node("Ranked")) {
			revertable_enum("Start ranked when...", vars.ranked.autostart_when);
			revertable_slider("Countdown time", vars.ranked.countdown_time, uint16_t(3u), uint16_t(300u));
			revertable_slider("Rejoin time limit", vars.ranked.rejoin_time_limit, uint16_t(0u), uint16_t(600u));
			revertable_slider("Max rejoins", vars.ranked.max_rejoins, uint8_t(0u), uint8_t(20u));

			revertable_checkbox("Freeze match on disconnect", vars.ranked.freeze_match_on_disconnect);
		}

		ImGui::Separator();

		text_color("General", yellow);

		ImGui::Separator();

		revertable_checkbox("Friendly fire", vars.friendly_fire);

		revertable_checkbox("Show on server list", vars.show_on_server_list);
		revertable_input_text(SCOPE_CFG_NVP(notified_server_list));

#if !IS_PRODUCTION_BUILD
		revertable_slider("Heartbeat interval (secs)", vars.send_heartbeat_to_server_list_once_every_secs, 1u, 50u);
#endif
		revertable_input_text(SCOPE_CFG_NVP(server_name));

		revertable_checkbox(SCOPE_CFG_NVP(daily_autoupdate));

		if (scope_cfg.daily_autoupdate) {
			auto scope = scoped_indent();
			revertable_input_text(SCOPE_CFG_NVP(daily_autoupdate_hour));
		}

		revertable_checkbox("I'm behind router", scope_cfg.allow_nat_traversal);

		revertable_input_text(SCOPE_CFG_NVP(external_arena_files_provider));
		tooltip_on_hover("Clients will first try to download missing files from this URL.\nIf for any reason the download fails or the files are out of date,\nthe clients will request a direct UDP transfer.");

		revertable_slider("Max direct file bandwidth (per second)", scope_cfg.max_direct_file_bandwidth, 0.0f, 2.f, "%.2f MB");
		tooltip_on_hover("If the external provider does not have the hosted map,\nclients will download it directly through UDP as a fallback mechanism.\nNote this will always be the case with Editor playtesting.");

		if (auto node = scoped_tree_node("Time limits")) {
			revertable_slider(SCOPE_CFG_NVP(move_to_spectators_if_afk_for_secs), 10u, 6000u);
			revertable_slider(SCOPE_CFG_NVP(kick_if_afk_for_secs), 10u, 2 * 3600u);
			revertable_slider(SCOPE_CFG_NVP(client_network_timeout_secs), 0.2f, 10.0f);
			revertable_slider("Client network timeout secs (WEB clients)", scope_cfg.web_client_network_timeout_secs, 0.2f, 10.0f);
			revertable_slider("Client network timeout secs (RANKED)", scope_cfg.ranked.client_network_timeout_secs, 0.2f, 10.0f);
			revertable_slider(SCOPE_CFG_NVP(time_limit_to_enter_game_since_connection), 5u, 300u);
		}

		text_disabled("\n");

		ImGui::Separator();

		text_color("Dedicated server", yellow);

		ImGui::Separator();

		revertable_slider(SCOPE_CFG_NVP(sleep_mult), 0.0f, 0.9f);
		revertable_checkbox(SCOPE_CFG_NVP(sync_all_external_arenas_on_startup));
	}
}

#undef CONFIG_NVP
