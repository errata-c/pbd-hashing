#pragma once
#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/BBox.hpp>
#include <cinttypes>
#include <vector>
#include <array>

namespace pbd {
	/*
	Table of my own design, sorts along all the axes of the vectors,
	Then generates a list of overlaps.
	
	This table is meant as a means to implement a broad phase pass of a collision engine.
	It is not meant to be very efficient at queries.

	Can store the sorted lists from one step to the next to take advantage of the nearly sorted nature of them.
	*/
	class SortTable {
	public:
		using scalar_t = float;
		using index_t = int32_t;
		static constexpr glm::length_t Dims = 3;
		using grid_t = Grid<index_t, Dims, scalar_t>;
		using bbox_t = BBox<Dims, scalar_t>;
		using vec_t = grid_t::vec_t;
		using ivec_t = grid_t::ivec_t;

		static_assert(Dims > 1 && Dims < 4, "pbd::SortTable expects 2 or 3 dimensional elements!");

		void build(const index_t* const _ids, const bbox_t* const _bounds, size_t count) {
			// Copy over all the bounds.
			// Sort them by their minimum.
			
			// Once the axes have been sorted begin the process of generating the list of overlaps.
			// Every overlap is a pairing of objects, in order to prevent generating duplicate pairings only generate the pairing
			//  where the first id is less than the second id.
			// In order to reduce the total data used to define those pairings,
			//  put the first id, then follow it by all the other ids it overlaps with (first id < second id)

			/*
			First test:
				Add an index to be sorted for the min and max of each bbox.
				
			*/
			const bbox_t* bit = _bounds;
			const bbox_t* const bend = _bounds + count;
			const index_t* idit = _ids;
			const index_t* const idend = _ids + count;

			for (int i = 0; i < Dims; ++i) {
				auto & sdim = sdims[i];
				sdim.clear();
				sdim.reserve(count * 2);

				for (bit = _bounds, idit = _ids; bit != bend; ++bit) {
					sdim.push_back(Element(*idit, bit->min[i]));
					sdim.push_back(Element(*idit, bit->max[i]));
				}
			}

			for (int i = 0; i < Dims; ++i) {
				std::sort(sdims[i].begin(), sdims[i].end());
			}
		}

	private:
		std::vector<bbox_t> bounds;

		struct Element {
			index_t id;
			scalar_t pos;

			bool operator<(const Element& other) const noexcept {
				return pos < other.pos;
			}
		};
		std::array<std::vector<Element>, Dims> sdims;


	};
}