#include <array>

#include <pbd/common/BBox.hpp>
#include <pbd/hashing/util.hpp>
#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/BaseTable.hpp>

#include <catch2/catch_all.hpp>

using namespace pbd;

TEST_CASE("BaseTable") {
	using Table = BaseTable<float, int32_t, 3>;
	using index_t = Table::index_t;
	using ivec_t = Table::ivec_t;
	using CellRange = Table::CellRange;

	Table table;

	REQUIRE(table.numCells() == 0);

	// BaseTable is prepared in three steps:
	//	Count all the entries needed for the cells.
	//	Prepare the entries.
	//	Insert the indices into the prepared entries.

	SECTION("Single cell bound") {
		int64_t totentries = 0;

		ivec_t loc(1,1,1);

		table.count(loc, totentries);
		table.prepareCellEntries(totentries);
		table.insert(1, loc);

		REQUIRE(table.numCells() == 1);

		CellRange entries = table.find(loc);
		REQUIRE(entries.size() == 1);
		REQUIRE(entries.front() == 1);
	}

	SECTION("Multiple cell bound") {
		int64_t totentries = 0;

		// An 8 cell bound.
		ivec_t 
			loc0(1, 1, 1),
			loc1(2, 2, 2);

		table.count(loc0, loc1, totentries);
		table.prepareCellEntries(totentries);
		table.insert(1, loc0, loc1);

		REQUIRE(table.numCells() == 8);

		for (int i = 0; i < 8; ++i) {
			ivec_t loc(1,1,1);
			for (int j = 0; j < 3; ++j) {
				if (i & (1 << j)) {
					loc[j] += 1;
				}
			}

			INFO("loc: [" << loc[0] << ", " << loc[1] << ", " << loc[2] << "]");
			
			CellRange entries = table.find(loc);
			REQUIRE(entries.size() == 1);
			REQUIRE(entries.front() == 1);
		}
	}

	SECTION("Single cell range") {
		int64_t totentries = 0;

		ivec_t loc(1, 1, 1);

		table.count(loc, loc, totentries);
		table.prepareCellEntries(totentries);
		table.insert(1, loc, loc);

		REQUIRE(table.numCells() == 1);

		CellRange entries = table.find(loc);
		REQUIRE(entries.size() == 1);
		REQUIRE(entries.front() == 1);
	}

	SECTION("Overlapping cells") {
		int64_t totentries = 0;

		// An 8 cell bound.
		ivec_t
			loc0(1, 1, 1),
			loc1(2, 2, 2);

		// Single cell bound
		ivec_t loc2(1,2,2);

		// A 2 cell bound
		ivec_t
			loc3(2, 2, 1),
			loc4(2, 2, 2);
		
		table.count(loc0, loc1, totentries);
		table.count(loc2, loc2, totentries);
		table.count(loc3, loc4, totentries);

		table.prepareCellEntries(totentries);

		table.insert(1, loc0, loc1);
		table.insert(2, loc2, loc2);
		table.insert(3, loc3, loc4);

		REQUIRE(table.numCells() == 8);

		CellRange entries = table.find(ivec_t(1,1,1));
		REQUIRE(entries.size() == 1);
		REQUIRE(entries[0] == 1);

		entries = table.find(ivec_t(1, 1, 2));
		REQUIRE(entries.size() == 1);
		REQUIRE(entries[0] == 1);

		entries = table.find(ivec_t(1, 2, 1));
		REQUIRE(entries.size() == 1);
		REQUIRE(entries[0] == 1);

		entries = table.find(ivec_t(1, 2, 2));
		REQUIRE(entries.size() == 2);
		REQUIRE(entries[0] == 2);
		REQUIRE(entries[1] == 1);


		entries = table.find(ivec_t(2, 1, 1));
		REQUIRE(entries.size() == 1);
		REQUIRE(entries[0] == 1);

		entries = table.find(ivec_t(2, 1, 2));
		REQUIRE(entries.size() == 1);
		REQUIRE(entries[0] == 1);

		entries = table.find(ivec_t(2, 2, 1));
		REQUIRE(entries.size() == 2);
		REQUIRE(entries[0] == 3);
		REQUIRE(entries[1] == 1);

		entries = table.find(ivec_t(2, 2, 2));
		REQUIRE(entries.size() == 2);
		REQUIRE(entries[0] == 3);
		REQUIRE(entries[1] == 1);
	}
}