#pragma once
#include <cinttypes>
#include <vector>
#include <limits>
#include <cassert>
#include <pbd/hashing/util.hpp>
#include <parallel_hashmap/phmap.h>

namespace pbd {
	template<typename Scalar, typename Index, glm::length_t L, typename Hasher = std::hash<glm::vec<L, Index>>>
	class BaseTable {
	public:
		using scalar_t = Scalar;
		using index_t = Index;
		static constexpr glm::length_t Dims = L;
		using ivec_t = glm::vec<Dims, index_t>;
		
		using map_t = phmap::parallel_flat_hash_map<ivec_t, index_t, Hasher>;
		using map_iter_t = typename map_t::const_iterator;
		using entries_t = std::vector<index_t>;

		static constexpr ptrdiff_t MaxIndex = std::numeric_limits<index_t>::max();
		static constexpr ptrdiff_t MinIndex = std::numeric_limits<index_t>::lowest();

		class CellRange;
		class const_iterator;

		void clear() {
			cellMap.clear();
			cellEntries.clear();
		}

		size_t numCells() const {
			return cellMap.size();
		}

		void count(const ivec_t& vec, int64_t& totalEntries) {
			auto it = cellMap.find(vec);
			if (it == cellMap.end()) {
				// We start at 2, to reserve a place for the entry count.
				// We put it in the entry list so the elements in the cell map are as small as possible.
				cellMap.insert(it, { vec, 2 });
				totalEntries += 2;
			}
			else {
				++totalEntries;
				++it->second;
			}
		}
		void count(const ivec_t& b0, const ivec_t& b1, int64_t & totalEntries) {
			applyAllCells(b0, b1, [&](const ivec_t& vec) {
				count(vec, totalEntries);
			});
		}
		void prepareCellEntries(int64_t totalEntries) {
			cellEntries.resize(totalEntries, 0);

			// Early out.
			if (totalEntries == 0) {
				return;
			}

			int64_t tot = 0;
			for (auto& kv : cellMap) {
				// ecount is the number of entries this cell is going to use, including the first entry that holds ecount-1.
				index_t ecount = kv.second;

				// Remap the cell to the current index in the entry list.
				kv.second = static_cast<index_t>(tot);

				// Move to the next open position in the entry list.
				tot += ecount;
				// Make sure we aren't over the limit.
				assert(tot < MaxIndex);

				// First entry is the number of ids in the cell.
				cellEntries[kv.second] = ecount - 1;

				// We use this value in the next step to make sure we insert everything correctly.
				cellEntries[kv.second + 1] = ecount - 1;
			}
		}
		void insert(index_t id, const ivec_t& vec) {
			assert(cellMap.contains(vec));

			// Grab the index associated with this vec
			index_t start = cellMap[vec];

			// The offset to the end of the cell entries range.
			index_t& old_offset = cellEntries[start + 1];
			index_t offset = old_offset;

			// We HAVE to do this before the next statement.
			// Otherwise we can end up modifying the id value we just inserted.
			--old_offset;

			// We insert the ids in REVERSE order.
			cellEntries[start + offset] = id;
		}
		void insert(index_t id, const ivec_t& b0, const ivec_t& b1) {
			applyAllCells(b0, b1, [&](const ivec_t& vec) {
				insert(id, vec);
			});
		}

		CellRange find(const ivec_t& vec) const {
			auto it = cellMap.find(vec);
			if (it != cellMap.end()) {
				return CellRange(cellEntries, it->second);
			}
			else {
				return {};
			}
		}


		const_iterator begin() const noexcept {
			return const_iterator(cellMap.begin(), cellEntries);
		}
		const_iterator end() const noexcept {
			return const_iterator(cellMap.end(), cellEntries);
		}
		

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
				return *(mlast-1);
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

			const ivec_t& cell() const {
				return it->first;
			}
			CellRange range() const {
				return CellRange(*entries, it->second);
			}
		private:
			map_iter_t it;
			const entries_t* entries;
		};
	private:
		map_t cellMap;
		entries_t cellEntries;
	};
}