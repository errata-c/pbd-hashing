#pragma once
#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/BBox.hpp>
#include <pbd/hashing/BaseTable.hpp>

namespace pbd {
	// Dynamically sized vector hash table.
	// This is a true hash table, not just a grid method.
	// Grid based methods are essentially just a fancy radix sort.
	class DVTable {
	public:
		using scalar_t = float;
		using index_t = int32_t;
		static constexpr glm::length_t Dims = 3;
		using grid_t = Grid<index_t, Dims, scalar_t>;
		using bbox_t = BBox<Dims, scalar_t>;
		using vec_t = grid_t::vec_t;
		using ivec_t = grid_t::ivec_t;

		using subtable_t = BaseTable;

		void initialize(const grid_t& _grid) {
			grid = _grid;
		}
		void initialize(const vec_t& _min, const vec_t& _max, const ivec_t& _cells) {
			grid = grid_t(_min, _max, _cells);
		}

		const grid_t& getGrid() const {
			return grid;
		}

		void clear() {
			grid = grid_t();
			table.clear();
		}


		// Build from a set of bounding boxes
		void build(const index_t* const ids, const bbox_t* const bounds, size_t count) {
			table.clear();

			int64_t totalEntries = 0;
			const bbox_t* boxit = bounds;
			const bbox_t* boxend = bounds + count;
			for (; boxit != boxend; ++boxit) {
				ivec_t b0 = grid.calcCell(boxit->min);
				ivec_t b1 = grid.calcCell(boxit->max);
				table.count(b0, b1, totalEntries);
			}

			table.prepareCellEntries(totalEntries);

			boxit = bounds;
			for (const index_t* idit = ids; boxit != boxend; ++boxit, ++idit) {
				ivec_t b0 = grid.calcCell(boxit->min);
				ivec_t b1 = grid.calcCell(boxit->max);
				table.insert(*idit, b0, b1);
			}
		}

		// Build from a set of points
		void build(const index_t* const ids, const vec_t* const points, size_t count) {
			clear();

			int64_t totalEntries = 0;
			const vec_t* pit = points;
			const vec_t* pend = points + count;
			for (; pit != pend; ++pit) {
				ivec_t vec = grid.calcCell(*pit);
				table.count(vec, totalEntries);
			}

			table.prepareCellEntries(totalEntries);

			pit = points;
			for (const index_t* idit = ids; pit != pend; ++pit, ++idit) {
				ivec_t vec = grid.calcCell(*pit);
				table.insert(*idit, vec);
			}
		}

		size_t numCells() const {
			return table.numCells();
		}
		size_t numEntries() const {
			return table.numEntries();
		}
	private:
		grid_t grid;
		subtable_t table;
	};
}