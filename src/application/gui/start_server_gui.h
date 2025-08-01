#pragma once
#include "application/setups/server/server_listen_input.h"
#include "augs/math/vec2.h"
#include "augs/misc/imgui/standard_window_mixin.h"
#include "application/setups/server/dedicated_or_integrated.h"
#include "augs/misc/timing/timer.h"
#include "application/arena/synced_dynamic_vars.h"
#include "application/setups/server/server_temp_var_overrides.h"

struct server_vars;
struct server_public_vars;
class nat_detection_session;

class start_server_gui_state : public standard_window_mixin<start_server_gui_state> {
public:
	using base = standard_window_mixin<start_server_gui_state>;
	using base::base;

	port_type previous_chosen_port = DEFAULT_GAME_PORT_V;

	dedicated_or_integrated type = dedicated_or_integrated::INTEGRATED;

	bool is_steam_client = false;
	bool show_help = false;
	bool show_nat_details = false;

	bool enable_bots = true;
	difficulty_type bot_difficulty = difficulty_type::HARD;

	int bot_allies = 2;
	int bot_enemies = 5;

	server_temp_var_overrides get_initial_overrides() const;

	bool perform(
		server_listen_input& into,
		server_vars&,

		const nat_detection_session*,
		const port_type currently_bound_port
	);
};
