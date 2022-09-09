#pragma once
#include <cinttypes>
#include <vector>
#include <limits>
#include <cassert>
#include <pbd/hashing/common.hpp>
#include <parallel_hashmap/phmap.h>

namespace pbd {
	// Dynamically sized vector hash table.
	// This is a true hash table, not just a grid method.
	// Grid based methods are essentially just a fancy radix sort.
	class BaseTable {
	public:
		using scalar_t = float;
		using index_t = int32_t;
		static constexpr glm::length_t Dims = 3;
		using ivec_t = glm::vec<Dims, index_t>;

		using map_t = phmap::parallel_flat_hash_map<ivec_t, index_t>;
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
		size_t numEntries() const {
			return cellEntries.size();
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
				++it->second;
				++totalEntries;
			}
		}
		void count(const ivec_t& b0, const ivec_t& b1, int64_t & totalEntries) {
			applyAllCells(b0, b1, [&](const ivec_t& vec) {
				count(vec, totalEntries);
			});
		}
		void prepareCellEntries(int64_t totalEntries) {
			cellEntries.resize(totalEntries, 0);

			int64_t tot = 0;
			for (auto& kv : cellMap) {
				int64_t tmp = kv.second;
				assert(tot < MaxIndex);
				kv.second = static_cast<index_t>(tot);
				tot += tmp;

				// First entry is the number of ids in the cell.
				assert(tmp < MaxIndex);
				cellEntries[kv.second] = static_cast<index_t>(tmp);

				// Put the offset to the end of this cell's entry list at the first position in list.
				// We will use this in the next step to insert properly.
				cellEntries[kv.second + 1] = static_cast<index_t>(tmp - 1);
			}
		}
		void insert(index_t id, const ivec_t& vec) {
			index_t start = cellMap[vec];

			index_t& offset = cellEntries[start + 1];
			cellEntries[start + offset + 1] = id;
			--offset;
		}
		void insert(index_t id, const ivec_t& b0, const ivec_t& b1) {
			applyAllCells(b0, b1, [&](const ivec_t& vec) {
				insert(id, vec);
			});
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
				: first(nullptr)
				, last(nullptr)
			{}
			CellRange(const entries_t& list, index_t index)
			{
				first = list.data() + index;
				last = first + (*first);

				++first;
				++last;
			}

			explicit operator bool() const noexcept {
				return !empty();
			}

			size_t size() const noexcept {
				return last - first;	
			}
			bool empty() const noexcept {
				return first == last;
			}

			const index_t& operator[](int32_t i) const noexcept {
				assert(i >= 0 && i < size());
				return first[i];
			}

			const index_t* begin() const noexcept {
				return first;
			}
			const index_t* end() const noexcept {
				return last;
			}
		private:
			const index_t* first, * last;
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
				return CellRange(*entries, it->second);
			}
			CellRange operator->() const {
				return CellRange(*entries, it->second);
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
		private:
			map_iter_t it;
			const entries_t* entries;
		};
	private:
		map_t cellMap;
		entries_t cellEntries;
	};
}