#pragma once
#include <optional>
#include "augs/templates/traits/is_comparable.h"
#include "augs/pad_bytes.h"

namespace augs {
	struct introspection_access;

	template <class T, bool SerializeDisabled = true>
	struct maybe {
		static constexpr bool serialize_disabled = SerializeDisabled;

		// GEN INTROSPECTOR struct augs::maybe class T bool S
		T value;
		bool is_enabled = false;
		pad_bytes<3> pad;
		// END GEN INTROSPECTOR

		static auto enabled(const T& value) {
			return maybe(value, true);
		}

		static auto disabled(const T& value) {
			return maybe(value, false);
		}

		maybe() = default;
		maybe(const T& value) : value(value), is_enabled(true) {}
		maybe(const T& value, const bool is_enabled) : value(value), is_enabled(is_enabled) {}

		void set(const T& a) {
			value = a;
			is_enabled = true;
		}

		template <class A>
		void emplace(A&& a) {
			value = std::forward<A>(a);
			is_enabled = true;
		}

		void reset() {
			is_enabled = false;
		}

		auto& operator=(const T& t) {
			emplace(t);
			return *this;
		}

		auto& operator=(T&& t) {
			emplace(std::move(t));
			return *this;
		}

		explicit operator bool() const {
			return is_enabled;
		}

		operator maybe<T, !SerializeDisabled>() const {
			return { value, is_enabled };
		}

		friend bool operator==(const maybe<T, SerializeDisabled>&, const maybe<T, SerializeDisabled>&) = default;
	};
}

