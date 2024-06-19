#pragma once

struct content_regeneration_settings {
	// GEN INTROSPECTOR struct content_regeneration_settings
	bool regenerate_every_time = false;
	bool rescan_assets_on_window_focus = true;

	unsigned atlas_blitting_threads = 2;
	unsigned neon_regeneration_threads = 2;
	// END GEN INTROSPECTOR

	bool operator==(const content_regeneration_settings& b) const = default;
};