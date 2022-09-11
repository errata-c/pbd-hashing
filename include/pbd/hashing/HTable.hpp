#pragma once
#include <cinttypes>
#include <vector>

#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/BBox.hpp>
#include <pbd/hashing/BaseTable.hpp>

#include <pbd/hashing/OverlapList.hpp>

namespace pbd {
	// Heirarchical hash table, multiple size tiers for objects to be inserted.
	// Uses the smallest tier that an object will fit into to minimize the number of entries that are created.
	// The downside is that each tier requires its own map, and each map must be consulted to check for collisions.
	template<typename Scalar, typename Index, glm::length_t L>
	class HTable {
	public:
		using scalar_t = Scalar;
		using index_t = Index;
		static constexpr glm::length_t Dims = L;
		using grid_t = StrictGrid<index_t, Dims, scalar_t>;
		using bbox_t = BBox<Dims, scalar_t>;
		using vec_t = typename grid_t::vec_t;
		using ivec_t = typename grid_t::ivec_t;

		using subtable_t = BaseTable<scalar_t, index_t, Dims>;

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

			tiers.resize(ntiers);
			tcounts.resize(ntiers, 0);
		}

		bool isInitialized() const noexcept {
			return tiers.size() > 0;
		}
		explicit operator bool() const noexcept {
			return isInitialized();
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
		size_t numCells() const {
			size_t count = 0;
			for (const subtable_t & table : tiers) {
				count += table.numCells();
			}
			return count;
		}
		size_t numCellsTier(size_t i) const {
			assert(i < numTiers());
			return tiers[i].numCells();
		}

		void build(const bbox_t* const bounds, size_t count) {
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
			ClassifiedTier ctier;

			const bbox_t* boxit = bounds;
			const bbox_t* boxend = bounds + count;
			for (; boxit != boxend; ++boxit) {
				ctier = classify(*boxit);
				tiers[ctier.msb].count(ctier.b0, ctier.b1, tcounts[ctier.msb]);
			}

			for (size_t i = 0 ; i < tiers.size(); ++i) {
				tiers[i].prepareCellEntries(tcounts[i]);
			}

			boxit = bounds;
			for (size_t i = 0; i < count; ++i, ++boxit) {
				ctier = classify(*boxit);

				tiers[ctier.msb].insert(static_cast<index_t>(i), ctier.b0, ctier.b1);
			}
		}

		void findOverlaps(const index_t* const ids, const bbox_t* const bounds, size_t count, OverlapList& list) {
			using CellRange = typename subtable_t::CellRange;
			/*
			Iterate over all the bounding boxes. Classify the box tier.
			For each cell it it occupies in its tier, one-to-one compare all its neighbors.
			Then check all the cells it occupies in the next up tier.
			*/
			list.clear();


			// Iterate the bounds
			const bbox_t * boxit = bounds;
			const bbox_t* const boxend = bounds + count;
			size_t bidx = 0;
			ClassifiedTier ctier;
			for (; boxit != boxend; ++boxit, ++bidx) {
				const bbox_t& bbox = *boxit;
				ctier = classify(*boxit);

				list.group();
				list.push(ids[bidx]);

				// For the first tier the bound is in:
				{
					subtable_t& table = tiers[ctier.msb];

					// For each cell the bound occupies, find it in the table
					applyAllCells(ctier.b0, ctier.b1, [&](const ivec_t& loc) {
							// Find the cell 'loc' in the table.
							CellRange cell = table.find(loc);

							// For each element in the range, check if it overlaps.
							for (index_t cid : cell) {
								// Ignore any ids greater than the box id, to make sure we only add a pairing once.
								if (cid >= bidx) {
									continue;
								}

								const bbox_t& other = bounds[cid];
								if (bbox.overlaps(other)) {
									// Add to the list.
									list.push(ids[cid]);
								}
							}
						});

					ctier.b0 /= 2;
					ctier.b1 /= 2;
				}

				for (index_t tier = ctier.msb+1, ntiers = static_cast<index_t>(tiers.size()); tier < ntiers; ++tier) {
					subtable_t& table = tiers[tier];

					// For each cell the bound occupies, find it in the table
					applyAllCells(ctier.b0, ctier.b1, [&](const ivec_t& loc) {
							// Find the cell 'loc' in the table.
							CellRange cell = table.find(loc);

							// For each element in the range, check if it overlaps.
							for (index_t cid : cell) {
								// No longer have to ignore any ids.

								const bbox_t& other = bounds[cid];
								if (bbox.overlaps(other)) {
									// Add to the list.
									list.push(ids[cid]);
								}
							}
						});

					ctier.b0 /= 2;
					ctier.b1 /= 2;
				}
				list.ungroup();
			}

			// Done
		}

	protected:
		grid_t grid;
		std::vector<int64_t> tcounts;
		std::vector<subtable_t> tiers;

		struct ClassifiedTier {
			ivec_t b0, b1;
			index_t msb;
		};
		ClassifiedTier classify(const bbox_t & bbox) {
			ClassifiedTier result;
			result.b0 = grid.calcCell(bbox.min);
			result.b1 = grid.calcCell(bbox.max);

			ivec_t size = result.b1 - result.b0;
			index_t l = 0;
			for (int i = 0; i < Dims; ++i) {
				l = std::max(l, size[i]);
			}
			l += 1;

			index_t tier = 0;
			index_t cap = static_cast<index_t>(tiers.size()) - 1;

			while ((l > 1) && (tier < cap)) {
				l = l / 2;
				++tier;
			}

			result.msb = tier;
			index_t factor = 1 << result.msb;
			for (int i = 0; i < Dims; ++i) {
				result.b0[i] /= factor;
				result.b1[i] /= factor;
			}

			return result;
		}
	};
}