#pragma once
#include <algorithm>
#include "augs/math/vec2.h"
#include "augs/math/transform.h"
#include "augs/math/rects.h"
#include "augs/math/matrix.h"
#include "augs/math/make_rect_points.h"

template <class T>
struct basic_camera_eye {
	using vec2_type = basic_vec2<T>;
	using ltrb = basic_ltrb<T>;

	using zoom_type = T;

	// GEN INTROSPECTOR struct basic_camera_eye class T
	basic_transform<T> transform;
	zoom_type zoom;
	// END GEN INTROSPECTOR

	basic_camera_eye(
		const basic_transform<T>& transform = basic_transform<T>(),
		const T zoom = 1.f
	) : 
		transform(transform),
		zoom(zoom)
	{}
};

using camera_eye = basic_camera_eye<float>;

struct camera_cone {
	using vec2_type = camera_eye::vec2_type;
	using transform_type = transformr;

	camera_eye eye;
	vec2 screen_size;

	camera_cone(
		const camera_eye eye,
		const vec2i screen_size
	) : 
		eye(eye),
		screen_size(screen_size)
	{}

	template <class E>
	static auto from_aabb(const E& handle) {
		const auto tr = handle.get_logic_transform();
		return camera_cone(camera_eye(tr.pos, 1.f), handle.get_aabb(tr).get_size());
	}

	vec2_type to_screen_space(const vec2_type world_pos) const {
		// TODO: support rotation
		return eye.zoom * (world_pos - eye.transform.pos) + screen_size / 2;
	}

	transform_type to_screen_space(transform_type world) const {
		world.pos = to_screen_space(world.pos);
		return world;
	}

	ltrb to_screen_space(const ltrb r) const {
		// TODO: support rotation
		return { to_screen_space(r.get_position()), r.get_size() * eye.zoom };
	}

	auto get_visible_world_area() const {
		return vec2_type(screen_size) / eye.zoom;
	}

	ltrb get_visible_world_rect_aabb() const {
		const auto visible_world_area = get_visible_world_area();

		const auto rotated_rect_verts = augs::make_rect_points(
			eye.transform.pos, 
			visible_world_area, 
			eye.transform.rotation
		);

		return augs::calc_vertices_aabb(rotated_rect_verts);
	}
	
	auto get_projection_matrix() const {
		// TODO: support rotation
		return augs::orthographic_projection(get_visible_world_rect_aabb());
	}

	vec2_type to_world_space(const vec2_type cursor_pos) const {
		// TODO: support rotation
		return eye.transform.pos + (cursor_pos - screen_size / 2) / eye.zoom;
	}

	auto get_world_screen_center() const {
		return to_world_space(screen_size / 2);
	}
};

using zoom_type = camera_eye::zoom_type;
