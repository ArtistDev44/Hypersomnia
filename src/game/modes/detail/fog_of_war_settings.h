#pragma once
#include "augs/math/vec2.h"

struct fog_of_war_settings {
	// GEN INTROSPECTOR struct fog_of_war_settings
	real32 angle = 165.f;
	vec2 size = vec2(1920, 1080);
	// END GEN INTROSPECTOR

	bool operator==(const fog_of_war_settings& b) const = default;

	bool is_enabled() const {
		return angle > 0.f && angle < 360.f;
	}

	auto get_real_size() const {
		return size * 2;
	}
};
