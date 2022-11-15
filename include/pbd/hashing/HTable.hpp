#pragma once
#include <cinttypes>
#include <vector>

#include <pbd/hashing/util.hpp>
#include <parallel_hashmap/phmap.h>

#include <pbd/hashing/Grid.hpp>
#include <pbd/common/BBox.hpp>
#include <pbd/hashing/OverlapList.hpp>

namespace pbd {
	// Heirarchical hash table, multiple size tiers for objects to be inserted.
	// Uses the smallest tier that an object will fit into to minimize the number of entries that are created.
	template<typename Scalar, typename Index, glm::length_t L, size_t MaxTiers = 64>
	class HTable {
	public:
		using scalar_t = Scalar;
		using index_t = Index;
		static constexpr glm::length_t Dims = L;
		using grid_t = Grid<index_t, Dims, scalar_t>;
		using bbox_t = BBox<Dims, scalar_t>;
		using vec_t = typename grid_t::vec_t;
		using ivec_t = typename grid_t::ivec_t;
		static constexpr size_t max_tiers = MaxTiers;
	
		struct Cell {
			index_t tier;
			ivec_t index;

			bool operator==(const Cell& other) const noexcept {
				return index == other.index && tier == other.tier;
			}
			bool operator!=(const Cell& other) const noexcept {
				return index != other.index || tier == other.tier;
			}

			friend size_t hash_value(const Cell& cell) noexcept {
				size_t seed = std::hash<ivec_t>{}(cell.index);
				size_t hash = std::hash<size_t>{}(cell.tier);
				hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= hash;
				return seed;
			}
		};

		using map_t = phmap::parallel_flat_hash_map<Cell, index_t>;
		using map_iter_t = typename map_t::const_iterator;
		using entries_t = std::vector<index_t>;

		static constexpr ptrdiff_t MaxIndex = std::numeric_limits<index_t>::max();
		static constexpr ptrdiff_t MinIndex = std::numeric_limits<index_t>::lowest();

		class CellRange;
		class const_iterator;
	private:
		// Linear search
		static index_t msb1(index_t val) {
			for (int i = 1, c = sizeof(index_t) * 8; i < c; ++i) {
				if (val < ((index_t(1) << (i)) - 1)) {
					return i - 1;
				}
			}
			return sizeof(index_t) * 8 - 1;
		}
		// Binary search
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
		HTable()
			: tier_limit(0)
		{}
		HTable(const vec_t& _cell_size, size_t ntiers)
		{
			initialize(_cell_size, ntiers);
		}

		void initialize(const vec_t & _cell_size, size_t ntiers) {
			initialize(grid_t(_cell_size), ntiers);
		}
		void initialize(const grid_t& _grid, size_t ntiers) {
			grid = _grid;

			assert(ntiers < maxTiers());
			ntiers = std::min(maxTiers(), ntiers);

			tier_limit = ntiers;

			cell_map.clear();
			cell_entries.clear();
		}

		bool isInitialized() const noexcept {
			return tier_limit > 0;
		}
		explicit operator bool() const noexcept {
			return isInitialized();
		}

		void clear() {
			grid = grid_t();
			tier_limit = 0;

			cell_map.clear();
			cell_entries.clear();
		}

		const grid_t& getGrid() const {
			return grid;
		}
		size_t numTiers() const {
			return tier_limit;
		}
		static size_t maxTiers() {
			return max_tiers;
		}
		size_t numCells() const {
			return cell_map.size();
		}

		void build(const bbox_t* const bounds, size_t count) {
			if (tier_limit == 0) {
				return;
			}

			// Iterate over all the bounds
			// Classify the tier each one fits best.
			// Insert into said tier.
			
			// This may not perform well when one of the bound dimensions is much smaller than the others.

			size_t element_count = 0;
			cell_map.clear();
			cell_entries.clear();

			ClassifiedTier ctier;

			const bbox_t* boxit = bounds;
			const bbox_t* boxend = bounds + count;
			for (; boxit != boxend; ++boxit) {
				ctier = classify(*boxit);
				applyAllCells(ctier.b0, ctier.b1, [&](const ivec_t & vec){
					auto it = cell_map.find(Cell{ctier.msb, vec});
					if (it == cell_map.end()) {
						// We start at 2, to reserve a place for the entry count.
						// We put it in the entry list so the elements in the cell map are as small as possible.
						cell_map.insert(it, { Cell{ctier.msb, vec}, 2});
						element_count += 2;
					}
					else {
						++element_count;
						++it->second;
					}
				});
			}

			prepareCellEntries(element_count);

			boxit = bounds;
			for (size_t i = 0; i < count; ++i, ++boxit) {
				ctier = classify(*boxit);

				insert(static_cast<index_t>(i), ctier.msb, ctier.b0, ctier.b1);
			}
		}

		void findOverlaps(const index_t* const ids, const bbox_t* const bounds, size_t count, OverlapList& list) {
			/*
			Iterate over all the bounding boxes. Classify the box tier.
			For each cell it it occupies in its tier, check for overlap with the other bboxes in the cell.
			Note, we skip pairings where the other bbox has higher index to make sure we don't include a pairing twice.
			Its guaranteed we will add the reverse configuration if they overlap.

			Then we check all the bboxes in the tiers above it. 
			Note that we don't need to skip any pairings this time, because we don't compare downward.
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
					// For each cell the bound occupies, find it in the table
					applyAllCells(ctier.b0, ctier.b1, [&](const ivec_t& loc) {
						// Find the cell 'loc' in the table.
						CellRange cell = find(ctier.msb, loc);

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

				for (index_t tier = ctier.msb+1, ntiers = tier_limit; tier < ntiers; ++tier) {
					// For each cell the bound occupies, find it in the table
					applyAllCells(ctier.b0, ctier.b1, [&](const ivec_t& loc) {
						// Find the cell 'loc' in the table.
						CellRange cell = find(tier, loc);

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
		map_t cell_map;
		entries_t cell_entries;
		size_t tier_limit;

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
			index_t cap = tier_limit - 1;

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

		void count(index_t tier, const ivec_t& vec, int64_t& totalEntries) {
			auto it = cellMap.find(vec);
			if (it == cellMap.end()) {
				// We start at 2, to reserve a place for the entry count.
				// We put it in the entry list so the elements in the cell map are as small as possible.
				cell_map.insert(it, { tier, vec, 2 });
				totalEntries += 2;
			}
			else {
				++totalEntries;
				++it->second;
			}
		}
		void count(const ivec_t& b0, const ivec_t& b1, int64_t& totalEntries) {
			applyAllCells(b0, b1, [&](const ivec_t& vec) {
				count(vec, totalEntries);
			});
		}
		void prepareCellEntries(int64_t totalEntries) {
			cell_entries.resize(totalEntries, 0);

			// Early out.
			if (totalEntries == 0) {
				return;
			}

			int64_t tot = 0;
			for (auto& kv : cell_map) {
				// ecount is the number of entries this cell is going to use, including the first entry that holds ecount-1.
				index_t ecount = kv.second;

				// Remap the cell to the current index in the entry list.
				kv.second = static_cast<index_t>(tot);

				// Move to the next open position in the entry list.
				tot += ecount;
				// Make sure we aren't over the limit.
				assert(tot < MaxIndex);

				// First entry is the number of ids in the cell.
				cell_entries[kv.second] = ecount - 1;

				// We use this value in the next step to make sure we insert everything correctly.
				cell_entries[kv.second + 1] = ecount - 1;
			}
		}
		CellRange find(index_t tier, const ivec_t& vec) const {
			auto it = cell_map.find(Cell{tier, vec});
			if (it != cell_map.end()) {
				return CellRange(cell_entries, it->second);
			}
			else {
				return {};
			}
		}
		void insert(index_t id, index_t tier, const ivec_t& vec) {
			assert(cell_map.contains(Cell{tier, vec}));

			// Grab the index associated with this vec
			index_t start = cell_map[Cell{tier, vec}];

			// The offset to the end of the cell entries range.
			index_t& old_offset = cell_entries[start + 1];
			index_t offset = old_offset;

			// We HAVE to do this before the next statement.
			// Otherwise we can end up modifying the id value we just inserted.
			--old_offset;

			// We insert the ids in REVERSE order.
			cell_entries[start + offset] = id;
		}
		void insert(index_t id, index_t tier, const ivec_t& b0, const ivec_t& b1) {
			applyAllCells(b0, b1, [&](const ivec_t& vec) {
				insert(id, tier, vec);
			});
		}

	public:
		class CellRange {
		public:
			CellRange(const CellRange&) = default;
			CellRange& operator=(const CellRange&) = default;

			CellRange()
				: mfirst(nullptr)
				, mlast(nullptr)
			{}
			CellRange(const entries_t& list, index_t index)
			{
				mfirst = list.data() + index;
				mlast = mfirst + (*mfirst);

				++mfirst;
				++mlast;
			}

			explicit operator bool() const noexcept {
				return !empty();
			}

			size_t size() const noexcept {
				return mlast - mfirst;
			}
			bool empty() const noexcept {
				return mfirst == mlast;
			}

			const index_t& operator[](int32_t i) const noexcept {
				assert(i >= 0 && i < size());
				return mfirst[i];
			}

			const index_t* begin() const noexcept {
				return mfirst;
			}
			const index_t* end() const noexcept {
				return mlast;
			}

			const index_t& front() const noexcept {
				assert(!empty());
				return *mfirst;
			}
			const index_t& back() const noexcept {
				assert(!empty());
				return *(mlast - 1);
			}
		private:
			const index_t* mfirst, * mlast;
		};

		class const_iterator {
		public:
			const_iterator() = default;
			const_iterator(const const_iterator&) = default;
			const_iterator& operator=(const const_iterator&) = default;

			const_iterator(const map_iter_t& ref, const entries_t& e)
				: it(ref)
				, entries(&e)
			{}

			CellRange operator*() const {
				return range();
			}
			CellRange operator->() const {
				return range();
			}

			const_iterator operator++(int) {
				const_iterator copy = *this;
				++(*this);
				return copy;
			}
			const_iterator& operator++() {
				++it;
				return *this;
			}

			bool operator!=(const const_iterator& other) const noexcept {
				return it != other.it;
			}
			bool operator==(const const_iterator& other) const noexcept {
				return it == other.it;
			}

			const Cell& cell() const {
				return it->first;
			}
			CellRange range() const {
				return CellRange(*entries, it->second);
			}
		private:
			map_iter_t it;
			const entries_t* entries;
		};
	};
}