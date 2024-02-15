#pragma once
#include <unordered_set>
#include <optional>

#include "augs/misc/date_time.h"
#include "augs/math/camera_cone.h"
#include "augs/math/snapping_grid.h"

#include "augs/templates/container_templates.h"

#include "game/cosmos/entity_id.h"
#include "game/cosmos/per_entity_type.h"
#include "game/detail/render_layer_filter.h"
#include "game/modes/mode_player_id.h"

#include "application/setups/debugger/debugger_selection_groups.h"
#include "application/setups/debugger/gui/debugger_rect_select_type.h"
#include "application/setups/debugger/debugger_marks.h"

using current_selections_type = std::unordered_set<entity_id>;

struct debugger_view_ids {
	// GEN INTROSPECTOR struct debugger_view_ids
	debugger_selection_groups selection_groups;
	current_selections_type selected_entities;
	// END GEN INTROSPECTOR

	template <template <class> class Mod>
	void select(const per_entity_type_container<Mod>& new_selections) {
		selected_entities.clear();

		new_selections.for_each([&](const auto id) {
			selected_entities.emplace(id);
		});
	}
};

struct debugger_view {
	// GEN INTROSPECTOR struct debugger_view
	snapping_grid grid;
	bool show_grid = true;
	bool snapping_enabled = true;
	bool ignore_groups = false;
	debugger_rect_select_type rect_select_mode = debugger_rect_select_type::SAME_LAYER;

	maybe_layer_filter selecting_filter = maybe_layer_filter(render_layer_filter::all(), true);
	maybe_layer_filter viewing_filter = maybe_layer_filter(render_layer_filter::all(), true);

	std::optional<camera_eye> panned_camera;
	debugger_marks marks;

	mode_player_id local_player_id;
	entity_id overridden_viewed;
	// END GEN INTROSPECTOR

	void reset_zoom();
	void reset_panning();
	void reset_zoom_at(vec2);
	void center_at(vec2);

	void toggle_snapping();
	void toggle_ignore_groups();
	void toggle_grid();

	maybe_layer_filter get_effective_selecting_filter() const;
};
