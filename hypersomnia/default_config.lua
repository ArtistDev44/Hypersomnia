return {
  streamer_mode = false,
  launch_at_startup = "LAST_ACTIVITY", -- or "MAIN_MENU"

  log_timestamp_format = "[%H:%M:%S] ",
  --log_timestamp_format = "[%d-%m-%y %H:%M:%S] ",
  --log_timestamp_format = "",

  server_list_provider = "masterserver.hypersomnia.xyz:8420",
  webrtc_signalling_server_url = "wss://masterserver.hypersomnia.xyz:8000",

  nat_detection = {
	  packet_interval_ms = 5,

	  port_probing = {
	    num_available = 30,
	    num_probed_for_detection = 3,

	    host = {
	      address = "masterserver.hypersomnia.xyz",
	      default_port = 8430
	    }
	  },

	  num_stun_hosts_used_for_detection = 2,

	  nat_translation_entry_timeout_secs = 30,

	  stun_server_list = "detail/web/stun_server_list.txt"
  },

  nat_traversal = {
	  num_brute_force_packets = 1,
	  traversal_attempt_timeout_secs = 2
  },

  masterserver = {
	ip = "0.0.0.0",
	ssl_cert_path = "",
	ssl_private_key_path = "",
	signalling_ssl_cert_path = "",
	signalling_ssl_private_key_path = "",
	suppress_community_server_webhooks_after_launch_for_secs = 20,
	server_entry_timeout_secs = 65,
	signalling_peer_timeout_secs = 25,

	signalling_server_port = 8000,
	first_udp_command_port = 8430,
	num_udp_command_ports = 30,

	sleep_ms = 8,
	server_list_port = 8420,

	cert_pem_path = "",
	key_pem_path = ""
  },

  float_consistency_test = {
	  passes = 5000
	  , report_filename = ""
	  --, report_filename = "report.bin"
  },

  http_client = {
    update_on_launch = true,
	update_connection_timeout_secs = 2,
	self_update_host = "https://hypersomnia.xyz",
	self_update_path = "/builds/latest"
  },

  official_arena_servers = {
	"arena.hypersomnia.xyz",
	"arena-us.hypersomnia.xyz",
	"arena-de.hypersomnia.xyz",
	"arena-au.hypersomnia.xyz",
	"arena-ru.hypersomnia.xyz",
	"arena-ch.hypersomnia.xyz"
  },

  app_controls = {
    F1 = "SHOW_PERFORMANCE",
    --F9 = "TOGGLE_STREAMER_MODE",
    --F10 = "TOGGLE_CINEMATIC_MODE",
    F2 = "SHOW_LOGS"
  },

  game_controls = {
    W = "MOVE_FORWARD",
    S = "MOVE_BACKWARD",
    A = "MOVE_LEFT",
    D = "MOVE_RIGHT",

    LeftShift = "SPRINT",
    -- LeftShift = "TOGGLE_SPRINT",

    Space = "DASH",

    LeftMouseButton = "SHOOT",
    RightMouseButton = "SHOOT_SECONDARY",

    LeftControl = "WALK_SILENTLY",
    -- LeftControl = "TOGGLE_WALK_SILENTLY",

    G = "DROP",

    MiddleMouseButton = "THROW_FORCE_GRENADE",
    MouseButton5 = "THROW_FLASHBANG",
    MouseButton4 = "THROW_PED_GRENADE",
    J = "THROW_INTERFERENCE_GRENADE",

    WheelUp = "THROW_TWO_KNIVES",
    WheelDown = "THROW_KNIFE",

    E = "INTERACT",
	R = "RELOAD",

	C = "WIELD_BOMB",
    Backspace = "SWITCH_CAMERA_MODE"
  },

  general_gui_controls = {
    Tilde = "TOGGLE_MOUSE_CURSOR",
    F = "TOGGLE_WEAPON_LASER",
	Tab = "SCOREBOARD",
	B = "BUY_MENU",
	M = "CHOOSE_TEAM",

	Y = "CHAT",
	Slash = "CHAT_COMMAND",
	U = "TEAM_CHAT",

	F8 = "SERVER_ADMIN_PANEL",
	F10 = "EXECUTE_RCON_GAME_COMMANDS",

	RightMouseButton = "SPECTATE_PREVIOUS",
	LeftMouseButton = "SPECTATE_NEXT"
  },

  inventory_gui_controls = {
    ["0"] = "HOTBAR_9",
    ["1"] = "HOTBAR_0",
    ["2"] = "HOTBAR_1",
    ["3"] = "HOTBAR_2",
    ["4"] = "HOTBAR_3",
    ["5"] = "HOTBAR_4",
    ["6"] = "HOTBAR_5",
    ["7"] = "HOTBAR_6",
    ["8"] = "HOTBAR_7",
    ["9"] = "HOTBAR_8",

    Z = "SPECIAL_ACTION_BUTTON_1",
    X = "SPECIAL_ACTION_BUTTON_2",
    V = "SPECIAL_ACTION_BUTTON_3",
    N = "SPECIAL_ACTION_BUTTON_4",
    O = "SPECIAL_ACTION_BUTTON_5",
    P = "SPECIAL_ACTION_BUTTON_6",

    H = "HOLSTER",
    Q = "LAST_USED_WEAPON"
  },
  enabled_hud_messages = {
    offset_mult = 0.82,
    text_color = "255 255 255 255",
    background_color = "255 255 255 10",
    background_border_color = "255 255 255 40",
    message_lifetime_secs = 4,
    max_simultaneous_messages = 3,
    message_fading_secs = 0.5,
    box_padding = { x = 32, y = 6 },
    box_separation = 1.05
  },
  renderer = {
    default_filtering = "NEAREST_NEIGHBOR" -- alternatively: "LINEAR"
  },
  audio = {
    output_mode = "STEREO_HRTF",
    mute_main_menu_background = true,
    max_number_of_sound_sources = 4200,
    output_device_name = "",
	sound_meters_per_second = 150
  },
  audio_volume = {
    master = 1,
    sound_effects = 0.75,
    music = 0.55
  },
  sound = {
    processing_frequency = "EVERY_SIMULATION_STEP",
	sync_sounds_longer_than_secs = 5,
treat_as_music_sounds_longer_than_secs = 5,
    max_speed_for_doppler_calculation = 5000,
	missile_impact_sound_cooldown_duration = 60,
	missile_impact_occurences_before_cooldown = 1,
    max_short_sounds = 80,
    max_simultaneous_bullet_trace_sounds = 5,
	gain_threshold_for_bullet_trace_sounds = 0.012,
	max_divergence_before_sync_secs = 1,
	set_listener_orientation_to_character_orientation = false,
	listener_reference = "CHARACTER_POSITION"
  },
  simulation_receiver = {
    misprediction_smoothing_multiplier = 1.2000000476837158
  },
  lag_compensation = {
    confirm_local_character_death = true,
	simulate_decorative_organisms_during_reconciliation = false,

	effect_prediction = {
	  predict_death_particles = true,
	  predict_death_sounds = false
	}
  },
  camera = {
    edge_zoom_out_zone = 0.005,
    edge_zoom_in_zone_expansion = 1.0,
    edge_zoom_in_cutoff_mult = 0.2,

    additional_position_smoothing = {
      average_factor = 0.5,
      averages_per_sec = 0
    },
    surfing_zoom_out = 2,
    angled_look_length = 100,
    enable_smoothing = true,
    look_bound_expand = 0.5,
    smoothing = {
      average_factor = 0.5,
      averages_per_sec = 30
    }
  },
  content_regeneration = {
    regenerate_every_time = false,
	rescan_assets_on_window_focus = true,
	atlas_blitting_threads = 3,
	neon_regeneration_threads = 3
  },
  debug = {
    determinism_test_cloned_cosmoi_count = 0,
    input_recording_mode = "DISABLED"
  },
  debug_drawing = {
    draw_cast_rays = false,
    draw_collinearization = false,
    draw_discontinuities = false,
    draw_explosion_forces = false,
    draw_forces = false,
    draw_friction_field_collisions_of_entering = false,
    draw_memorised_walls = false,
    draw_triangle_edges = false,
    draw_undiscovered_locations = false,
    draw_visible_walls = false,
    enabled = false
  },
  drawing = {
    --custom_zoom = 4,
    snap_zoom_to_fov_size = true,
    snap_zoom_to_multiple_if_different_by_pixels = 100,

    crosshair = {
      scale = 3,
      border_width = 1,
      show_dot = true,
      dot_size = 1,
      segment_length = 10,
      recoil_expansion_base = 5,
      recoil_expansion_mult = 1
    },
    draw_character_gui = true,
    draw_crosshairs = true,
    draw_weapon_laser = true,
	enabled_draw_area_markers = 1.0,
	disabled_draw_callout_indicators = 1.0,
	draw_nicknames = true,
	draw_health_numbers = true,
	draw_damage_indicators = true,
	occlude_neons_under_sentiences = true,
    draw_offscreen_indicators = true,
    print_current_character_callout = true,
    nickname_characters_for_offscreen_indicators = 4,

    show_death_indicator_for_seconds = 5,
    fade_death_indicator_for_seconds = 2,

    show_danger_indicator_for_seconds = 10,
    fade_danger_indicator_for_seconds = 2,

    draw_offscreen_callouts = true,
	offscreen_reference_mode = "CHARACTER_POSITION",
	enabled_draw_teammate_indicators = 1,
	enabled_draw_tactical_indicators = 1,
	enabled_draw_danger_indicators = "255 50 50 255",

    fog_of_war_appearance = {
      overlay_color = "0 0 0 40",
      overlay_color_on_visible = false
    },
	fog_of_war = {
	  enabled = true,
	  angle = 165
    },
	enemy_hud_mode = "SMALL_HEALTH_BAR",
	draw_hp_bar = true,
	draw_cp_bar = true,
	draw_pe_bar = false,
	draw_remaining_ammo = true,
    radar_pos = {
		x = -201,
		y = 84
	}
  },
  editor = {
    grid = {
      render = {
        alpha_multiplier = 0.5,
        hide_grids_smaller_than = 16,
        line_colors = {
          "255 0 0 255",
          "255 0 255 255",
          "255 165 0 255",
          "255 255 0 100",
          "0 255 0 100",
          "0 255 255 100",
          "255 255 255 100",
          "177 177 177 100",
          "75 74 74 100",
          "54 54 54 100"
        },
        maximum_power_of_two = 12
      }
    },

    autosave = {
      enabled = true,
      once_every_min = 2,
      if_loaded_autosave_show = "AUTOSAVED_VERSION",
      alert_when_loaded_autosave = true
    },

    camera = {
      panning_speed = 1
    },

    entity_selector = {
      hovered_color = "255 255 255 0",
      selected_color = "65 131 196 3",
      held_color = "65 131 196 6",
      hovered_dashed_line_color = "255 255 255 140"
    },

    warp_cursor_when_moving_nodes = false,
    matched_entity_color = "0 255 0 80",

    rectangular_selection_color = "65 131 196 60",
    rectangular_selection_border_color = "65 131 196 120",

    action_notification = {
      enabled = true,

      bg_color = "0 0 0 200",
      bg_border_color = "255 255 255 15",

      max_width = 1000,
      show_for_ms = 3000,
      text_padding = { x = 10, y = 10 },
      offset = { y = 80 },
    },

    upload_url = "https://hypersomnia.xyz/upload",
    upload_api_key = ""
  },

  debugger = {
	player = {
		snapshot_interval_in_steps = 800
	},
    grid = {
      render = {
        alpha_multiplier = 0.5,
        hide_grids_smaller_than = 16,
        line_colors = {
          "255 0 0 255",
          "255 0 255 255",
          "255 165 0 255",
          "255 255 0 100",
          "0 255 0 100",
          "0 255 255 100",
          "255 255 255 100",
          "177 177 177 100",
          "75 74 74 100",
          "54 54 54 100"
        },
        maximum_power_of_two = 12
      }
    },
    autosave = {
      enabled = true,
      once_every_min = 1
    },
    go_to = {
      dialog_width = 400,
      num_lines = 15,
    },
    test_scene = {
      scene_tickrate = 60,
      start_arena_mode = true
    },
    camera = {
      panning_speed = 1
    },
    entity_selector = {
      hovered_color = "255 255 255 0",
      selected_color = "65 131 196 3",
      held_color = "65 131 196 6",
      hovered_dashed_line_color = "255 255 255 140"
    },
	tutorial_text_color = "220 220 220 255",
    controlled_entity_color = "255 255 0 120",
    matched_entity_color = "0 255 0 80",

    rectangular_selection_color = "65 131 196 60",
    rectangular_selection_border_color = "65 131 196 120",

    action_notification = {
	    bg_color = "0 0 0 180",
	    bg_border_color = "255 255 255 15",

	    max_width = 300,
	    show_for_ms = 3000,
	    text_padding = { x = 10, y = 10 },
	    offset = { x = 80, y = 80 },
    }
  },
  input = {
	character = {
	  crosshair_sensitivity = {
	    x = 3,
	    y = 3
	  },

      forward_moves_towards_crosshair = false
	},

	swap_mouse_buttons_in_akimbo = false,

	game_gui = {
      allow_switching_to_bare_hands_as_previous_wielded_weapon = true
	}
  },
  gui_fonts = {
	  gui = {
		unicode_ranges = {
		  { 0x0020, 0x00FF }, -- Basic Latin + Latin Supplement
		  { 0x0100, 0x017F }  -- Latin Extended-A
		},
		size_in_pixels = 22,
		add_japanese_ranges = "NEVER",
		add_cyrillic_ranges = "ONLY_IN_PRODUCTION",
		settings = {},
		source_font_path = "content/fonts/LiberationSans-Regular.ttf"
	  },

	  larger_gui = {
		unicode_ranges = {
		  { 0x0020, 0x00FF }, -- Basic Latin + Latin Supplement
		  { 0x0100, 0x017F }  -- Latin Extended-A
		},
		size_in_pixels = 32,
		add_japanese_ranges = "NEVER",
		add_cyrillic_ranges = "ONLY_IN_PRODUCTION",
		settings = {},
		source_font_path = "content/fonts/LiberationSans-Regular.ttf"
	  },

	  medium_numbers = {
		unicode_ranges = {
		  { 0x0030, 0x0039 }, -- Only numbers
		},
		size_in_pixels = 42,
		add_japanese_ranges = "NEVER",
		add_cyrillic_ranges = "NEVER",
		settings = {},
		source_font_path = "content/fonts/LiberationSans-Regular.ttf"
	  },

	  large_numbers = {
		unicode_ranges = {
		  { 0x0030, 0x0039 }, -- Only numbers
		},
		size_in_pixels = 52,
		add_japanese_ranges = "NEVER",
		add_cyrillic_ranges = "NEVER",
		settings = {},
		source_font_path = "content/fonts/LiberationSans-Regular.ttf"
	  },

	  very_large_numbers = {
		unicode_ranges = {
		  { 0x0030, 0x0039 }, -- Only numbers
		},
		size_in_pixels = 64,
		add_japanese_ranges = "NEVER",
		add_cyrillic_ranges = "NEVER",
		settings = {},
		source_font_path = "content/fonts/LiberationSans-Regular.ttf"
	  }
  },
  gui_style = {
    Alpha = 1,
    AntiAliasedLines = true,
    AntiAliasedShapes = true,
    ButtonTextAlign = {
      x = 0.5,
      y = 0.5
    },
    ChildBorderSize = 1,
    ChildRounding = 0,
    ColumnsMinSpacing = 6,
    CurveTessellationTol = 1.25,
    DisplaySafeAreaPadding = {
      x = 4,
      y = 4
    },
    DisplayWindowPadding = {
      x = 22,
      y = 22
    },
    FrameBorderSize = 0,
    FramePadding = {
      x = 4,
      y = 3
    },
    FrameRounding = 0,
    GrabMinSize = 10,
    GrabRounding = 0,
    IndentSpacing = 21,
    ItemInnerSpacing = {
      x = 4,
      y = 4
    },
    ItemSpacing = {
      x = 8,
      y = 4
    },
    PopupBorderSize = 1,
    PopupRounding = 0,
    ScrollbarRounding = 0,
    ScrollbarSize = 16,
    TouchExtraPadding = {
      x = 0,
      y = 0
    },
    WindowBorderSize = 1,
    WindowMinSize = {
      x = 32,
      y = 32
    },
    WindowPadding = {
      x = 8,
      y = 8
    },
    WindowRounding = 0,
    WindowTitleAlign = {
      x = 0,
      y = 0.5
    }
  },
  arena_mode_gui = {
    show_client_resyncing_notifier = true,
    death_summary_offset_mult = 0.5,

    between_knockout_boxes_pad = 8,
    inside_knockout_box_pad = 6,
    weapon_icon_horizontal_pad = 14,
    show_recent_knockouts_num = 7,
    keep_recent_knockouts_for_seconds = 8,

	money_indicator_color = "255 249 133 255",
	award_indicator_color = "255 255 0 255",

	show_recent_awards_num = 8,
	keep_recent_awards_for_seconds = 3,

	money_indicator_pos = {
		x = -50,
		y = 20 + 16 * 8
	},

	buy_menu_settings = {
		disabled_bg = "75 0 0 180",
		disabled_active_bg = "90 0 0 220",
		already_owns_active_bg = "0 130 0 150",
		already_owns_bg = "0 76 0 91",
		already_owns_other_type_bg = "94 12 255 91",
		already_owns_other_type_active_bg = "94 101 255 120"
	},

    scoreboard_settings = {
	  dark_color_overlay_under_score = true,
      background_color = "15 15 15 200",
      bg_lumi_mult = 0.69999998807907104,
      border_color = "109 109 127 100",
      current_player_bg_lumi_mult = 1.5000000238418579,
      current_player_text_lumi_mult = 1.7000000476837158,
      dead_player_bg_alpha_mult = 0.30000001192092896,
      dead_player_bg_lumi_mult = 0.5,
      dead_player_text_alpha_mult = 0.60000002384185791,
      dead_player_text_lumi_mult = 0.45000001192092896,
      cell_bg_alpha = 0.56000000238418579,
      faction_logo_alpha = 0.80000001192092896,
      avatar_alpha = 1,
	  icon_alpha = 0.8,
      player_row_inner_padding = {
        x = 4,
        y = 4
      },
      text_lumi_mult = 1.7000000476837158,
      text_stroke_lumi_mult = 0.5,
      window_padding = {
        x = 4,
        y = 8
      }
    },

    enabled_context_tip_settings = {
      tip_offset_mult = 0.75,
      tip_text_color = "255 255 255 255",
      bound_key_color = "255 255 0 255",
      item_name_color = "255 255 0 255"
	}
  },
  hotbar = {
    autocollapse_hotbar_buttons = true,
    hide_unassigned_hotbar_buttons = true,

    colorize_inside_when_selected = true,
    increase_inside_alpha_when_selected = false,
    primary_selected_color = "0 255 255 255",
    secondary_selected_color = "86 156 214 255"
  },
  game_gui = {
    autodrop_holstered_armed_explosives = true,
    autodrop_magazines_of_dropped_weapons = true,
    autocollapse_hotbar_buttons = true
  },
  interpolation = {
    method = "LINEAR",
    speed = 1000 -- only for EXPONENTIAL method
  },
  main_menu = {
    leaderboards_provider_url = "https://hypersomnia.xyz/leaderboards",
    menu_intro_scene_entropy_path = "abc",
    menu_background_arena_path = "content/menu/menu",
    menu_theme_path = "",
    rewind_intro_scene_by_secs = 3.5,
    skip_credits = true,
    start_menu_music_at_secs = 0
  },
  session = {
    hide_settings_ingame = true,
    show_performance = false,
    show_logs = false,
    camera_query_aabb_mult = 1.0
  },
  test_scene = {
    create_minimal = false,
    scene_tickrate = 60
  },
  unit_tests = {
    break_on_failure = true,
    log_successful = false,
    redirect_log_to_path = "",
    run = false
  },
  window = {
    app_icon_path = "content/gfx/necessary/app.ico",
    border = true,
    bpp = 32,
    fullscreen = true,
    name = "Hypersomnia",
    position = {
      x = 40,
      y = 40
    },
    draw_own_cursor_in_fullscreen = false,
    log_keystrokes = false,
    size = {
      x = 1200,
      y = 700
    },
    vsync_mode = "OFF",
    enabled_max_fps = 400,
    max_fps_method = "YIELD",

    -- Looks like GLFW disables X mappings.
    -- This is for Linux users (like me)
    -- who are likely to have bound CAPS LOCK to ESC through e.g. xmodmap -e "keysym Caps_Lock = Escape"
    -- GLFW will not recognize these mappings.
    map_caps_lock_to_esc = false
  },
  faction_view = {
	colors = {
      SPECTATOR = {
		  standard = "200 200 200 255",
		  current_player_text = "215 215 215 255",
		  background_dark = "0 0 0 0"
	  },
      METROPOLIS = {
		  standard = "40 230 255 255",
		  current_player_text = "70 240  255 255",
		  background_dark = "0 20 75 255"
	  },
      ATLANTIS = {
		  standard = "0 255 0 255"
	  },
      RESISTANCE = {
		  standard = "255 37 0 255",
		  current_player_text = "255 80 0 255",
		  background_dark = "47 10 0 255"
	  }
	}
  },

  damage_indication = {
    enabled_numbers_accumulation_speed = 1000,
    critical_color = "255 255 60 255",
    friendly_damage_border_color = "255 40 40 255",
    indicator_fading_duration_secs = 0.5,
    single_indicator_lifetime_secs = 1,
    accumulative_indicator_idle_lifetime_secs = 0.0,
    indicator_rising_speed = 50,
    small_damage_threshold = 25,
    medium_damage_threshold = 61,

    single_indicator_offsets = {
      { x = 0, y = 0 },
      { x = -25, y = 25 },
      { x = 45, y = -25 },
      { x = -45, y = -55 },
      { x = 15, y = -75 },
      { x = 0, y = 25 },
      { x = 35, y = 25 }
    },

    accumulative_indicator_offset = {
      x = 0, y = -80
    },

    white_damage_highlight_secs = 0.25,
    character_silhouette_damage_highlight_secs = 0.12
  },

  -- Private vars aren't known to any clients.

  server_start = {
	ip = "127.0.0.1",
	port = 0,
	slots = 10
  },

  server_private = {
    master_rcon_password = "",
    rcon_password = "",
    discord_webhook_url = "",
    telegram_webhook_url = "",
    telegram_channel_id = "@hypersomnia_monitor",

    -- For authenticating users through Steam tickets.
    steam_web_api_key = "",

    -- Ranked servers only.
    -- This secret key allows reporting match results to the leaderboard.
    report_ranked_match_api_key = "",
    report_ranked_match_url = "https://hypersomnia.xyz/report_match",

    report_ranked_match_aux_endpoints = {
    -- { url = "example.com", api_key = "example" }
    },

    check_ban_url = ""
    --check_ban_url = "https://hypersomnia.xyz/check_ban"
  },

  server = {
    server_name = "${MY_NICKNAME}'s server",

    -- If > 0, kicks players who do not provide either a Steam or Discord (TODO) session ticket upon connection.
    -- Will happen on all ranked servers by default.
    -- Note you'll need to setup your own steam_web_api_key in order to authenticate players.
    kick_if_unauthenticated_for_secs = 6.0,

    ranked = {
      -- Note you can always host a "ranked" match locally -
      -- even if you don't report winners/losers to the MMR database.
      -- 
      -- It will still make the match behave like a proper tournament match.
      -- For example:
      --    - The server becomes unjoinable after starting the match.
      --    - Match will be frozen for a while when someone disonnects mid-way, allowing them to rejoin.
      --
      -- You may absolutely use it on some LAN torunaments.

      -- Enables ranked match logic depending on team composition,
      -- With "SERVER_FULL", use the server_start.slots variable to determine if the server is meant for
      -- 1v1 (slots=2), 2v2 (slots=4) or other configurations.
      -- ALWAYS will allow asymmetrical teams liks 1v3.
      -- NEVER is the default for casual servers.
      autostart_when = "NEVER", -- "| TEAMS_EQUAL | SERVER_FULL | SERVER_FULL_EQUAL_TEAMS | ALWAYS",
      -- Only "NEVER" and "ALWAYS" are implemented right now.

      -- The "RANKED COUNTDOWN" timer.
      countdown_time = 81,

      -- Keep this relatively tight so the match can instantly freeze
      -- in case the game crashes in a critical situation.
      client_network_timeout_secs = 1.5,

      -- Max: 65535
      rejoin_time_limit = 180,

      -- After this many rejoins,
      -- you will no longer be able to join this match after disconnect.
      --
      -- On official ranked servers, this results in a ban.
      -- Feel free to set it to a high value for matches between trusted friends.
      -- Max: 255
      max_rejoins = 5,

      -- When all players finally rejoin,
      -- match will remain frozen for this many more seconds 
      -- for everyone to get ready.
      -- Max: 255
      match_unfreezes_in_secs = 5,

      -- Freeze the entire game completely if a client disconnects.
      -- This could be due to a game crash or network failure.
      --
      -- This effectively stops time.
      -- If false, will just lengthen the freeze time on the next round.
      --
      -- THe characters will end up with their buttons released (except Use button).
      -- The match will resume once all players are back.
      -- Use for tournaments between trusted friends only.
      -- "false" option IS NOT SUPPORTED YET!!!
      freeze_match_on_disconnect = true
    },

    -- For unranked games, can be reasonably high.
    client_network_timeout_secs = 5,

    arena = "de_cyberaqua",
    game_mode = "",

    friendly_fire = true, -- not implemented yet

    cycle = "REPEAT_CURRENT", -- | "LIST" | "ALL_ON_DISK"

    -- Just add "arena mode" or "arena" for the map-default mode.
    -- "mode" can be bomb_defusal, gun_game or quick_test.
    cycle_list = {
      "de_cyberaqua",
      "fy_minilab bomb_defusal",
      "de_silo"
    }, 

    -- Will go to the next map in the cycle when there are no players on the server.
    when_idle_change_maps_once_every_mins = 15,

    -- Convenience var to override the game mode
    -- for all maps on the list or on the disk.
    cycle_always_game_mode = "",

    -- Applies when cycle is "LIST" or "ALL_ON_DISK".
    cycle_randomize_order = true,

    daily_autoupdate = false,
    daily_autoupdate_hour = "03:00",

    external_arena_files_provider = "https://hypersomnia.xyz/arenas",
    sync_all_external_arenas_on_startup = false,

    allow_nat_traversal = true,

    notified_server_list = "masterserver.hypersomnia.xyz:8430",

    send_heartbeat_to_server_list_once_every_secs = 10,
    resolve_server_list_address_once_every_secs = 3600,
    sleep_mult = 0.1,
    log_performance_once_every_secs = 0,

    move_to_spectators_if_afk_for_secs = 120,
    kick_if_afk_for_secs = 2 * 3600,
    time_limit_to_enter_game_since_connection = 15,

    send_packets_once_every_tick = 1,
    reset_resync_timer_once_every_secs = 4,
    max_client_resyncs = 30,

    disabled_network_simulator = {
      latency_ms = 50,
      jitter_ms = 10,
      loss_percent = 1,
      duplicates_percent = 1,
    },

    max_buffered_client_commands = 1280,
    state_hash_once_every_tick = 1,
    send_net_statistics_update_once_every_secs = 0.5,

    auto_authorize_loopback_for_rcon = true,
    auto_authorize_internal_for_rcon = false,
    max_unauthorized_rcon_commands = 100,
    max_bots = 0,
    max_direct_file_bandwidth = 6
  },

  client_connect = "127.0.0.1",

  client = {
	nickname = "",
	rcon_password = "",

	net = {
	  jitter = {
	  	buffer_at_least_steps = 3,
		buffer_at_least_ms = 20,
		max_commands_to_squash_at_once = 255
	  }
	},

	disabled_network_simulator = {
      latency_ms = 70,
      jitter_ms = 15,
      loss_percent = 1,
	  duplicates_percent = 1,
	},

	max_buffered_server_commands = 10000,
	max_predicted_client_commands = 1500,
    flush_demo_to_disk_once_every_secs = 10,
    spectated_arena_type = "REFERENTIAL",

	client_chat = {
      background_color = "15 15 15 200",
      border_color = "109 109 127 100",
      chat_window_width = 600,
	  chat_window_offset = {
		  x = 10,
		  y = 300
	  },
      show_recent_chat_messages_num = 30,
      keep_recent_chat_messages_for_seconds = 22,
	},
      max_direct_file_bandwidth = 2
  },

  performance = {
	max_particles_in_single_job = 2500,
	swap_buffers_when = "AFTER_GL_COMMANDS",

    special_effects = {
	  explosions = {
	    sparkle_amount = 1,
	    thunder_amount = 1,
	    smoke_amount = 0.7
	  },
	  particle_stream_amount = 1,
	  particle_burst_amount = 1
    }
  }
}
