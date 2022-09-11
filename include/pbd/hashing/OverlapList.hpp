#pragma once
#include <cinttypes>
#include <vector>
#include <cassert>
#include <parallel_hashmap/phmap.h>

namespace pbd {
	class OverlapList {
	public:
		using index_t = int32_t;
		using container_t = std::vector<index_t>;
		using set_t = phmap::flat_hash_set<index_t>;

		class Overlaps;
		class const_iterator;

		OverlapList(const OverlapList&) = default;
		OverlapList(OverlapList&&) noexcept = default;
		OverlapList& operator=(OverlapList&&) noexcept = default;
		OverlapList& operator=(const OverlapList&)  = default;
		~OverlapList() = default;

		OverlapList()
			: count(0)
		{
#ifndef NDEBUG
			inGroup = false;
#endif
		}

		void clear() {
			count = 0;
			list.clear();
		}
		bool empty() const noexcept {
			return count == 0;
		}
		size_t size() const noexcept {
			return count;
		}

		void group() {
			assert(!inGroup);
#ifndef NDEBUG
			inGroup = true;
#endif
			list.push_back(0);
			list.push_back(1);
		}
		void push(index_t idx) {
			assert(inGroup);
			auto result = gset.insert(idx);
			if(result.second) {
				index_t offset = list.back();
				list.back() = idx;
				list.push_back(offset + 1);
			}
		}
		void ungroup() {
			assert(inGroup);
#ifndef NDEBUG
			inGroup = false;
#endif

			index_t offset = list.back();
			
			if (offset <= 2) {
				// Empty group
				list.pop_back();
				list.pop_back();
				if (offset == 2) {
					// Single element group, discard.
					list.pop_back();
				}
			}
			else {
				size_t index = list.size() - offset - 1;

				// Store the number of elements in the first index.
				// This means that its length encoded, first index is the count, followed by count elements. Then the next group. etc...
				list[index] = offset - 1;

				list.pop_back();
				++count;
			}

			gset.clear();
		}

		const_iterator begin() const noexcept {
			return const_iterator(list.begin());
		}
		const_iterator end() const noexcept {
			return const_iterator(list.end());
		}


		class Overlaps {
		public:
			Overlaps(const Overlaps&) = default;
			Overlaps& operator=(const Overlaps&) = default;

			Overlaps()
				: mfirst(nullptr)
				, mlast(nullptr)
			{}
			Overlaps(const index_t * _first)
			{
				mfirst = _first;
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

			const_iterator(const container_t::const_iterator& ref)
				: it(ref)
			{}

			Overlaps operator*() const {
				return range();
			}
			Overlaps operator->() const {
				return range();
			}

			const_iterator operator++(int) {
				const_iterator copy = *this;
				++(*this);
				return copy;
			}
			const_iterator& operator++() {
				it += (*it) + 1;

				return *this;
			}

			bool operator!=(const const_iterator& other) const noexcept {
				return it != other.it;
			}
			bool operator==(const const_iterator& other) const noexcept {
				return it == other.it;
			}

			Overlaps range() const {
				return Overlaps(&*it);
			}
		private:
			container_t::const_iterator it;
		};
	private:
		size_t count;
		container_t list;
		// We may want to add a set to prevent duplicates from being added to the list.
		set_t gset;

#ifndef NDEBUG
		bool inGroup;
#endif
	};
}