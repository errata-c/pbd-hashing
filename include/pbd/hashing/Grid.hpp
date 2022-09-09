#pragma once
#include <cassert>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <pbd/hashing/common.hpp>

namespace pbd {
	template<typename index_t, glm::length_t L, typename scalar_t>
	class Grid {
	public:
		using vec_t = glm::vec<L, scalar_t>;
		using ivec_t = glm::vec<L, index_t>;
		Grid()
			: morigin(0)
			, mscale(1)
			, mcells(0)
		{}
		Grid(const vec_t& _min, const vec_t& _max, const ivec_t& _cells)
			: morigin(glm::min(_min, _max))
			, mcells(glm::abs(_cells))
		{
			vec_t region = glm::max(_min, _max) - morigin;
			assert(glm::all(glm::greaterThan(region, vec_t(scalar_t(1e-5)))));

			for (glm::length_t i = 0; i < L; ++i) {
				mscale[i] = static_cast<scalar_t>(mcells[i]) / region[i];
			}
		}

		ivec_t calcCell(const vec_t& vec) const {
			vec_t offset = (vec - morigin) * mscale;
			ivec_t ioffset;
			for (glm::length_t i = 0; i < L; ++i) {
				ioffset[i] = static_cast<index_t>(offset[i]);
			}
			return ioffset;
		}
		index_t calcCellIndex(const vec_t& vec) const {
			ivec_t ioffset = calcCell(vec);

			index_t result = ioffset[0];
			for (glm::length_t i = 1; i < L; ++i) {
				result += ioffset[i] * capacity[i - 1];
			}

			return result;
		}
		index_t calcHash(const vec_t& vec) const {
			return hash(calcCell(vec));
		}

		const ivec_t& cells() const noexcept {
			return mcells;
		}
		const vec_t& origin() const noexcept {
			return morigin;
		}
		const vec_t& scale() const noexcept {
			return mscale;
		}
	private:
		vec_t morigin, mscale;
		ivec_t mcells;
	};

	template<typename index_t, glm::length_t L, typename scalar_t>
	class StrictGrid {
	public:
		using vec_t = glm::vec<L, scalar_t>;
		using ivec_t = glm::vec<L, index_t>;

		StrictGrid()
			: morigin(0)
			, mscale(1)
			, mcells(0)
		{}
		StrictGrid(scalar_t _min, scalar_t _max, index_t _cells)
			: morigin(std::min(_min, _max))
			, mcells(std::abs(_cells))
		{
			vec_t region = glm::max(_min, _max) - morigin;
			assert(glm::all(glm::greaterThan(region, vec_t(scalar_t(1e-5)))));

			for (glm::length_t i = 0; i < L; ++i) {
				mscale[i] = static_cast<scalar_t>(mcells[i]) / region[i];
			}
		}

		ivec_t calcCell(const vec_t& vec) const {
			vec_t offset = (vec - morigin) * mscale;
			ivec_t ioffset;
			for (glm::length_t i = 0; i < L; ++i) {
				ioffset[i] = static_cast<index_t>(offset[i]);
			}
			return ioffset;
		}
		index_t calcCellIndex(const vec_t& vec) const {
			ivec_t ioffset = calcCell(vec);

			index_t result = ioffset[0];
			for (glm::length_t i = 1; i < L; ++i) {
				result += ioffset[i] * capacity[i - 1];
			}

			return result;
		}
		index_t calcHash(const vec_t& vec) const {
			return hash(calcCell(vec));
		}

		const ivec_t& cells() const noexcept {
			return mcells;
		}
		const vec_t& origin() const noexcept {
			return morigin;
		}
		const vec_t& scale() const noexcept {
			return mscale;
		}
	private:
		vec_t morigin, mscale;
		ivec_t mcells;
	};
}