#pragma once
#include <cassert>
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtx/hash.hpp>

namespace pbd {
	template<glm::length_t L, typename index_t>
	static index_t hash(const glm::vec<L, index_t>& vec) {
		static constexpr index_t co[3] = { 92837111, 689287499, 283923481 };
		index_t result = vec[0] * co[0];
		for (glm::length_t i = 1; i < L; ++i) {
			result ^= (vec[i] * co[i]);
		}
		return result;
	}

	template<glm::length_t L, typename index_t, typename Func>
	void applyAllCells(const glm::vec<L, index_t>& b0, const glm::vec<L, index_t>& b1, Func&& func) {
		glm::vec<L, index_t> vec = b0;
		for (vec[0] = b0[0]; vec[0] <= b1[0]; ++vec[0]) {
			for (vec[1] = b0[1]; vec[1] <= b1[1]; ++vec[1]) {
				if constexpr (L == 2) {
					func(vec);
				}
				else if constexpr (L == 3) {
					for (vec[2] = b0[2]; vec[2] <= b1[2]; ++vec[2]) {
						func(vec);
					}
				}
			}
		}
	}
}