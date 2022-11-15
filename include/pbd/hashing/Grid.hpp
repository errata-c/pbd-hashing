#pragma once
#include <cassert>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <pbd/hashing/util.hpp>

namespace pbd {
	template<typename index_t, glm::length_t L, typename scalar_t>
	class Grid {
	public:
		using vec_t = glm::vec<L, scalar_t>;
		using ivec_t = glm::vec<L, index_t>;

		Grid()
			: mscale(1)
			, mcell(1)
		{}
		Grid(const vec_t& _cell_size)
			: mcell(_cell_size)
		{
			assert(glm::all(glm::greaterThan(mcell, vec_t(scalar_t(1e-5)))));
			mscale = vec_t(1) / mcell;
		}

		ivec_t calcCell(const vec_t& vec) const {
			vec_t offset = vec * mscale;
			ivec_t ioffset;
			for (glm::length_t i = 0; i < L; ++i) {
				ioffset[i] = static_cast<index_t>(offset[i]);
			}
			return ioffset;
		}

		const vec_t& cell() const noexcept {
			return mcell;
		}
		const vec_t& scale() const noexcept {
			return mscale;
		}
	private:
		vec_t mscale, mcell;
	};

	template<typename index_t, glm::length_t L, typename scalar_t>
	class UniformGrid {
	public:
		using vec_t = glm::vec<L, scalar_t>;
		using ivec_t = glm::vec<L, index_t>;

		UniformGrid()
			: mcell(1)
			, mscale(1)
		{}
		UniformGrid(scalar_t _cell_size)
			: mcell(_cell_size)
		{
			mscale = scalar_t(1) / mcell;
		}

		ivec_t calcCell(const vec_t& vec) const {
			vec_t offset = vec * vec_t(mscale);
			ivec_t ioffset;
			for (glm::length_t i = 0; i < L; ++i) {
				ioffset[i] = static_cast<index_t>(offset[i]);
			}
			return ioffset;
		}

		const scalar_t& cell() const noexcept {
			return mcells;
		}
		const scalar_t& scale() const noexcept {
			return mscale;
		}
	private:
		scalar_t mscale, mcell;
	};
}