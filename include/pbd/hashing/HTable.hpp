#pragma once
#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/BBox.hpp>
#include <pbd/hashing/BaseTable.hpp>

namespace pbd {
	// Heirarchical hash table, multiple size tiers for objects to be inserted.
	// Uses the smallest tier that an object will fit into to minimize the number of entries that are created.
	// The downside is that each tier requires its own map, and each map must be consulted to check for collisions.
	// 
	// To check for overlaps, start by iterating over all the elements in the lowest tier.
	// check to see if they overlap with neighbors in the same tier.
	// Then check to see if they overlap with potential neighbors in the next up tier, repeat upwards.
	// Then restart the process on the next tier. Continue until all tiers have been processed.
	class HTable {
	public:
		using scalar_t = float;
		using index_t = int32_t;
		static constexpr glm::length_t Dims = 3;
		using grid_t = StrictGrid<index_t, Dims, scalar_t>;
		using bbox_t = BBox<Dims, scalar_t>;
		using vec_t = grid_t::vec_t;
		using ivec_t = grid_t::ivec_t;

		using subtable_t = BaseTable;

	private:
		static index_t msb1(index_t val) {
			for (int i = 1, c = sizeof(index_t) * 8; i < c; ++i) {
				if (val < ((index_t(1) << (i)) - 1)) {
					return i - 1;
				}
			}
			return sizeof(index_t) * 8 - 1;
		}
		static index_t msb2(index_t val) {
			index_t mask = ~index_t(0);
			index_t shift = (sizeof(index_t) * 8) >> 1;

			index_t loc = 0;
			while (shift != 0) {
				mask = (mask >> shift) << shift;
				if (val & mask) {
					loc += shift;
				}
				else {
					mask = mask >> shift;
				}
				shift = shift >> 1;
			}
			return loc;
		}
	public:
		void initialize(scalar_t _min, scalar_t _max, index_t _cells, size_t ntiers) {
			initialize(grid_t(_min, _max, _cells), ntiers);
		}
		void initialize(const grid_t& _grid, size_t ntiers) {
			grid = _grid;

			assert(ntiers < maxTiers());
			ntiers = std::min(maxTiers(), ntiers);

			// Get the size of the grid,  then scale it in powers of two.
			index_t cells = grid.cells().x;
			for (size_t i = 0; i < ntiers; ++i) {
				cells /= 2;
				cells = std::max(cells, index_t(1));

				if (cells == 1) {
					ntiers = i;
					break;
				}
			}

			tiers.resize(ntiers);
			tcounts.resize(ntiers, 0);
		}

		void clear() {
			grid = grid_t();
			tiers.clear();
			tcounts.clear();
		}

		const grid_t& getGrid() const {
			return grid;
		}
		size_t numTiers() const {
			return tiers.size();
		}
		static size_t maxTiers() {
			return 64;
		}

		void build(const index_t* const ids, const bbox_t* const bounds, size_t count) {
			if (tiers.size() == 0) {
				return;
			}

			// Iterate over all the bounds
			// Classify the tier each one fits best.
			// Insert into said tier.
			
			// This may not perform well when one of the bound dimensions is much smaller than the others.

			std::fill(tcounts.begin(), tcounts.end(), 0);
			for (size_t i = 0; i < tiers.size(); ++i) {
				tiers[i].clear();
			}

			index_t ntiers = static_cast<index_t>(tiers.size());

			const bbox_t* boxit = bounds;
			const bbox_t* boxend = bounds + count;
			for (; boxit != boxend; ++boxit) {
				ivec_t b0 = grid.calcCell(boxit->min);
				ivec_t b1 = grid.calcCell(boxit->max);
				ivec_t size = b1 - b0;
				index_t l = 0;
				for (int i = 0; i < Dims; ++i) {
					l = std::max(l, size[i]);
				}
				l += 1;

				index_t msb = msb1(l);
				msb = std::min(msb, ntiers) - 1;

				tiers[msb].count(b0 >> msb, b1 >> msb, tcounts[msb]);
			}

			for (size_t i =0 ; i < tiers.size(); ++i) {
				tiers[i].prepareCellEntries(tcounts[i]);
			}

			boxit = bounds;
			for (const index_t* idit = ids; boxit != boxend; ++boxit, ++idit) {
				ivec_t b0 = grid.calcCell(boxit->min);
				ivec_t b1 = grid.calcCell(boxit->max);

				ivec_t size = b1 - b0;
				index_t l = 0;
				for (int i = 0; i < Dims; ++i) {
					l = std::max(l, size[i]);
				}
				l += 1;

				index_t msb = msb1(l);
				msb = std::min(msb, ntiers) - 1;

				tiers[msb].insert(*idit, b0 >> msb, b1 >> msb);
			}
		}

	private:
		grid_t grid;
		std::vector<int64_t> tcounts;
		std::vector<subtable_t> tiers;
	};
}