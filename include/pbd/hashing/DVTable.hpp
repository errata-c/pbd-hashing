#pragma once
#include <pbd/hashing/Grid.hpp>
#include <pbd/common/BBox.hpp>
#include <pbd/hashing/BaseTable.hpp>

namespace pbd {
	// Dynamically sized vector hash table.
	// This is a true hash table, not just a grid method.
	// Grid based methods are essentially just a fancy radix sort.
	template<typename Scalar, typename Index, glm::length_t L>
	class DVTable {
	public:
		using scalar_t = Scalar;
		using index_t = Index;
		static constexpr glm::length_t Dims = L;
		using grid_t = Grid<index_t, Dims, scalar_t>;
		using bbox_t = BBox<Dims, scalar_t>;
		using vec_t = typename grid_t::vec_t;
		using ivec_t = typename grid_t::ivec_t;

		using subtable_t = BaseTable<scalar_t, index_t, Dims>;
		using const_iterator = typename subtable_t::const_iterator;
		using CellRange = typename subtable_t::CellRange;

		void initialize(const grid_t& _grid) {
			grid = _grid;
		}
		void initialize(const vec_t& _cell_size) {
			grid = grid_t(_cell_size);
		}

		const grid_t& getGrid() const {
			return grid;
		}

		void clear() {
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
		void build(const bbox_t* const bounds, size_t count) {
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

			int i = 0;
			boxit = bounds;
			while (boxit != boxend) {
				ivec_t b0 = grid.calcCell(boxit->min);
				ivec_t b1 = grid.calcCell(boxit->max);
				table.insert(i, b0, b1);

				++boxit;
				++i;
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
			const index_t* idit = ids;
			while(pit != pend) {
				ivec_t vec = grid.calcCell(*pit);
				table.insert(*idit, vec);

				++pit;
				++idit;
			}
		}
		void build(const vec_t* const points, size_t count) {
			clear();

			int64_t totalEntries = 0;
			const vec_t* pit = points;
			const vec_t* pend = points + count;
			for (; pit != pend; ++pit) {
				ivec_t vec = grid.calcCell(*pit);
				table.count(vec, totalEntries);
			}

			table.prepareCellEntries(totalEntries);

			int i = 0;
			pit = points;
			while (pit != pend) {
				ivec_t vec = grid.calcCell(*pit);
				table.insert(i, vec);

				++pit;
				++i;
			}
		}

		CellRange find(const vec_t& point) const {
			return table.find(grid.calcCell(point));
		}

		const_iterator begin() const {
			return table.begin();
		}
		const_iterator end() const {
			return table.end();
		}

		size_t numCells() const {
			return table.numCells();
		}
	private:
		grid_t grid;
		subtable_t table;
	};
}