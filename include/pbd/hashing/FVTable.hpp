#pragma once
#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/BBox.hpp>
#include <cinttypes>
#include <vector>

namespace pbd {
	// Fixed size vector hash table.
	// Based on this paper: DOI:10.1145/2663806.2663862
	class FVTable {
	public:
		using scalar_t = float;
		using index_t = int32_t;
		static constexpr glm::length_t Dims = 3;
		using grid_t = Grid<index_t, Dims, scalar_t>;
		using bbox_t = BBox<Dims, scalar_t>;
		using vec_t = grid_t::vec_t;
		using ivec_t = grid_t::ivec_t;
		
		const grid_t& getGrid() const {
			return grid;
		}
		void setGrid(const grid_t& _grid) {
			grid = _grid;
		}
		void setGrid(const vec_t& _min, const vec_t& _max, const ivec_t& _cells) {
			grid = grid_t(_min, _max, _cells);
		}

		// Build from a set of bounding boxes
		/// Fixed vector table cannot do this!
		//void build(const index_t * const ids, const bbox_t * const bounds, size_t count) {
			
		//}

		// Build from a set of points
		void build(const index_t* const ids, const vec_t* const points, size_t count) {
			
		}

		void resize(size_t _count) {
			
		}
		
		size_t numCells() const {
			return used.size();
		}
		size_t numObjects() const {
			return objectIndices.size();
		}
	private:
		grid_t grid;

		struct Pivot {
			index_t first, last;
		};
		std::vector<index_t> used;
		std::vector<Pivot> pivots;
		std::vector<index_t> objectIndices;
	};
}