#pragma once
#include <vector>
#include <string>

#include <filesystem>

#include "augs/filesystem/path.h"
#include "augs/enums/callback_result.h"

namespace augs {
	bool natural_order(const std::string& a, const std::string& b);

	bool create_directory(const path_type& dir_path);
	bool create_directories(const path_type& dir_path);

	bool create_directories_for(path_type path);

	path_type get_current_working_directory();

	template <class D, class F>
	void for_each_in_directory_recursive(
		const path_type& dir_path,
		D directory_callback,
		F file_callback
	) {
		using namespace std::filesystem;

		for (recursive_directory_iterator i(dir_path), end; i != end; ++i) {
			const auto p = make_windows_friendly(i->path());

			if (is_directory(p)) {
				directory_callback(p);
			}
			else {
				file_callback(p);
			}
		}
	}

	template <class D, class F>
	bool for_each_in_directory(
		const path_type& dir_path,
		D directory_callback,
		F file_callback
	) {
		using namespace std::filesystem;

		for (directory_iterator i(dir_path), end; i != end; ++i) {
			const auto p = make_windows_friendly(i->path());

			if (is_directory(p)) {
				if (directory_callback(p) == callback_result::ABORT) {
					return false;
				}
			}
			else {
				if (file_callback(p) == callback_result::ABORT) {
					return false;
				}
			}
		}
		
		return true;
	}

	template <class D, class F>
	bool for_each_in_directory_sorted(
		const path_type& dir_path,
		D directory_callback,
		F file_callback
	) {
		using namespace std::filesystem;

		std::vector<path_type> entries;

		for (const auto& entry : directory_iterator(dir_path)) {
			entries.push_back(make_windows_friendly(entry.path()));
		}

		std::sort(entries.begin(), entries.end(), [](const path_type& a, const path_type& b) {
			return natural_order(a.string(), b.string());
		});

		for (const auto& p : entries) {
			if (is_directory(p)) {
				if (directory_callback(p) == callback_result::ABORT) {
					return false;
				}
			}
			else {
				if (file_callback(p) == callback_result::ABORT) {
					return false;
				}
			}
		}

		return true;
	}

	template <class D>
	bool for_each_directory_in_directory(
		const path_type& dir_path,
		D&& directory_callback
	) {
		auto skip_file = [](auto&&...) { return callback_result::CONTINUE; };

		return for_each_in_directory(dir_path, std::forward<D>(directory_callback), skip_file);
	}
}