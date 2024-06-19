#pragma once
#include "augs/math/vec2.h"
#include "augs/templates/maybe.h"

struct game_gui_settings {
	// GEN INTROSPECTOR struct game_gui_settings
	bool autodrop_holstered_armed_explosives = true;
	bool autodrop_magazines_of_dropped_weapons = true;
	// END GEN INTROSPECTOR

	bool operator==(const game_gui_settings& b) const = default;
};