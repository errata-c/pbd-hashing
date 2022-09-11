#include <glm/gtx/io.hpp>
#include <array>

#include <pbd/hashing/BBox.hpp>
#include <pbd/hashing/common.hpp>
#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/HTable.hpp>

#include <catch2/catch_all.hpp>

using namespace pbd;

using Table = HTable<float, int32_t, 3>;

class TestTable : public Table {
public:
	using ClassifiedTier = Table::ClassifiedTier;
	ClassifiedTier test_classify(const bbox_t& bbox) {
		return classify(bbox);
	}
};

TEST_CASE("HTable") {
	using bbox_t = Table::bbox_t;
	using index_t = Table::index_t;
	using vec_t = Table::vec_t;
	using ivec_t = Table::ivec_t;
	using grid_t = Table::grid_t;

	Table table;

	REQUIRE_FALSE(table.isInitialized());
	REQUIRE(table.numCells() == 0);
	REQUIRE(table.numTiers() == 0);

	// Cells cover a 1x1x1 area, with 4 tiers in the heirarchy. (2x2x2, 4x4x4, and 8x8x8)
	table.initialize(0.f, 1.f, 1, 4);
	REQUIRE(table.isInitialized());
	REQUIRE(table.numTiers() == 4);
	REQUIRE(table.numCells() == 0);


	std::vector<bbox_t> bounds;
	std::vector<index_t> ids;
	OverlapList overlapList;

	// set to check group requirements.
	phmap::flat_hash_set<index_t> group;

	SECTION("Classify") {
		using ClassifiedTier = TestTable::ClassifiedTier;
		TestTable & ttable = static_cast<TestTable&>(table);
		const grid_t& grid = ttable.getGrid();

		bbox_t test(vec_t(0.1), ivec_t(1.9));
		
		ivec_t
			loc0 = grid.calcCell(test.min),
			loc1 = grid.calcCell(test.max);

		REQUIRE(loc0 == ivec_t(0, 0, 0));
		REQUIRE(loc1 == ivec_t(1, 1, 1));

		ClassifiedTier ctier = ttable.test_classify(test);
		REQUIRE(ctier.msb == 1);

		test = bbox_t(vec_t(0.1), vec_t(3.9));
		ctier = ttable.test_classify(test);
		REQUIRE(ctier.msb == 2);

		test = bbox_t(vec_t(0.1), vec_t(7.9));
		ctier = ttable.test_classify(test);
		REQUIRE(ctier.msb == 3);
	}

	auto add = [&](const vec_t & min, const vec_t & max, index_t id) {
		bounds.push_back(bbox_t::Between(min, max));
		ids.push_back(id);
	};
	auto prep = [&]() {
		table.build(bounds.data(), bounds.size());
		table.findOverlaps(ids.data(), bounds.data(), bounds.size(), overlapList);
	};

	SECTION("Single cell first tier") {
		add(vec_t(0.1), vec_t(0.9), 1);
		
		prep();
		
		REQUIRE(overlapList.size() == 0);
		REQUIRE(table.numCellsTier(0) == 1);
		REQUIRE(table.numCells() == 1);
	}

	SECTION("Single cell first tier, far from origin") {
		add(vec_t(10.1), vec_t(10.9), 1);

		prep();

		REQUIRE(overlapList.size() == 0);
		REQUIRE(table.numCells() == 1);
		REQUIRE(table.numCellsTier(0) == 1);
		REQUIRE(table.numCellsTier(1) == 0);
		REQUIRE(table.numCellsTier(2) == 0);
		REQUIRE(table.numCellsTier(3) == 0);
	}

	SECTION("Bound by itself, fitting in the second tier") {
		add(vec_t(0.1), vec_t(1.9), 1);

		prep();

		REQUIRE(overlapList.size() == 0);
		REQUIRE(table.numCells() == 1);
		REQUIRE(table.numCellsTier(0) == 0);
		REQUIRE(table.numCellsTier(1) == 1);
		REQUIRE(table.numCellsTier(2) == 0);
		REQUIRE(table.numCellsTier(3) == 0);
	}
	SECTION("Bound by itself, fitting in the third tier") {
		add(vec_t(0.1), vec_t(3.9), 1);

		prep();

		REQUIRE(overlapList.size() == 0);
		REQUIRE(table.numCells() == 1);
		REQUIRE(table.numCellsTier(0) == 0);
		REQUIRE(table.numCellsTier(1) == 0);
		REQUIRE(table.numCellsTier(2) == 1);
		REQUIRE(table.numCellsTier(3) == 0);
	}
	SECTION("Bound by itself, fitting in the fourth tier") {
		add(vec_t(0.1), vec_t(7.9), 1);

		prep();

		REQUIRE(overlapList.size() == 0);
		REQUIRE(table.numCells() == 1);
		REQUIRE(table.numCellsTier(0) == 0);
		REQUIRE(table.numCellsTier(1) == 0);
		REQUIRE(table.numCellsTier(2) == 0);
		REQUIRE(table.numCellsTier(3) == 1);
	}


	SECTION("Bound overlapping a single other bound, both fit in the first tier") {
		add(vec_t(0.1), vec_t(0.5), 0);
		add(vec_t(0.4), vec_t(0.9), 1);

		prep();

		REQUIRE(overlapList.size() == 1);
		auto it = overlapList.begin();
		auto end = overlapList.end();
		REQUIRE(it != end);

		auto overlaps = *it;
		REQUIRE(overlaps.size() == 2);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		REQUIRE(group.contains(0));
		REQUIRE(group.contains(1));

		REQUIRE(table.numCells() == 1);
		REQUIRE(table.numCellsTier(0) == 1);
		REQUIRE(table.numCellsTier(1) == 0);
		REQUIRE(table.numCellsTier(2) == 0);
		REQUIRE(table.numCellsTier(3) == 0);
	}
	SECTION("Bound overlapping a single other bound, both fit in the second tier") {
		add(vec_t(0.1), vec_t(1.5), 0);
		add(vec_t(0.4), vec_t(1.9), 1);

		prep();

		REQUIRE(overlapList.size() == 1);
		auto it = overlapList.begin();
		auto end = overlapList.end();
		REQUIRE(it != end);

		auto overlaps = *it;
		REQUIRE(overlaps.size() == 2);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		REQUIRE(group.contains(0));
		REQUIRE(group.contains(1));

		REQUIRE(table.numCells() == 1);
		REQUIRE(table.numCellsTier(0) == 0);
		REQUIRE(table.numCellsTier(1) == 1);
		REQUIRE(table.numCellsTier(2) == 0);
		REQUIRE(table.numCellsTier(3) == 0);
	}
	SECTION("Bound overlapping a single other bound, both fit in the third tier") {
		add(vec_t(0.1), vec_t(3.5), 0);
		add(vec_t(0.4), vec_t(3.9), 1);

		prep();

		REQUIRE(overlapList.size() == 1);
		auto it = overlapList.begin();
		auto end = overlapList.end();
		REQUIRE(it != end);

		auto overlaps = *it;
		REQUIRE(overlaps.size() == 2);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		REQUIRE(group.contains(0));
		REQUIRE(group.contains(1));

		REQUIRE(table.numCells() == 1);
		REQUIRE(table.numCellsTier(0) == 0);
		REQUIRE(table.numCellsTier(1) == 0);
		REQUIRE(table.numCellsTier(2) == 1);
		REQUIRE(table.numCellsTier(3) == 0);
	}
	SECTION("Bound overlapping a single other bound, both fit in the fourth tier") {
		add(vec_t(0.1), vec_t(7.5), 0);
		add(vec_t(0.4), vec_t(7.9), 1);

		prep();

		REQUIRE(overlapList.size() == 1);
		auto it = overlapList.begin();
		auto end = overlapList.end();
		REQUIRE(it != end);

		auto overlaps = *it;
		REQUIRE(overlaps.size() == 2);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		REQUIRE(group.contains(0));
		REQUIRE(group.contains(1));

		REQUIRE(table.numCells() == 1);
		REQUIRE(table.numCellsTier(0) == 0);
		REQUIRE(table.numCellsTier(1) == 0);
		REQUIRE(table.numCellsTier(2) == 0);
		REQUIRE(table.numCellsTier(3) == 1);
	}

	// Test a bound overlapping a single other bound, one in first tier, one in second tier.
	SECTION("Bound overlap, first tier and second tier") {
		add(vec_t(0.1), vec_t(0.9), 0);
		add(vec_t(0.4), vec_t(1.9), 1);

		prep();

		REQUIRE(overlapList.size() == 1);
		auto it = overlapList.begin();
		auto end = overlapList.end();
		REQUIRE(it != end);

		auto overlaps = *it;
		REQUIRE(overlaps.size() == 2);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		REQUIRE(group.contains(0));
		REQUIRE(group.contains(1));

		REQUIRE(table.numCells() == 2);
		REQUIRE(table.numCellsTier(0) == 1);
		REQUIRE(table.numCellsTier(1) == 1);
		REQUIRE(table.numCellsTier(2) == 0);
		REQUIRE(table.numCellsTier(3) == 0);
	}

	SECTION("Bound overlap, first and third tier") {
		add(vec_t(0.1), vec_t(0.9), 0);
		add(vec_t(0.4), vec_t(3.9), 1);

		prep();

		REQUIRE(overlapList.size() == 1);
		auto it = overlapList.begin();
		auto end = overlapList.end();
		REQUIRE(it != end);

		auto overlaps = *it;
		REQUIRE(overlaps.size() == 2);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		REQUIRE(group.contains(0));
		REQUIRE(group.contains(1));

		REQUIRE(table.numCells() == 2);
		REQUIRE(table.numCellsTier(0) == 1);
		REQUIRE(table.numCellsTier(1) == 0);
		REQUIRE(table.numCellsTier(2) == 1);
		REQUIRE(table.numCellsTier(3) == 0);
	}

	SECTION("Bound overlap, first and fourth tier") {
		add(vec_t(0.1), vec_t(0.9), 0);
		add(vec_t(0.4), vec_t(7.9), 1);

		prep();

		REQUIRE(overlapList.size() == 1);
		auto it = overlapList.begin();
		auto end = overlapList.end();
		REQUIRE(it != end);

		auto overlaps = *it;
		REQUIRE(overlaps.size() == 2);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		REQUIRE(group.contains(0));
		REQUIRE(group.contains(1));

		REQUIRE(table.numCells() == 2);
		REQUIRE(table.numCellsTier(0) == 1);
		REQUIRE(table.numCellsTier(1) == 0);
		REQUIRE(table.numCellsTier(2) == 0);
		REQUIRE(table.numCellsTier(3) == 1);
	}

	SECTION("Bound overlap, all tiers") {
		add(vec_t(0.1), vec_t(0.9), 0);
		add(vec_t(0.1), vec_t(1.9), 1);
		add(vec_t(0.1), vec_t(3.9), 2);
		add(vec_t(0.1), vec_t(7.9), 3);

		prep();

		REQUIRE(table.numCells() == 4);
		REQUIRE(table.numCellsTier(0) == 1);
		REQUIRE(table.numCellsTier(1) == 1);
		REQUIRE(table.numCellsTier(2) == 1);
		REQUIRE(table.numCellsTier(3) == 1);

		// Overlaps generated:
		// 0, 1, 2, 3
		// 1, 2, 3
		// 2, 3
		
		REQUIRE(overlapList.size() == 3);
		auto it = overlapList.begin();
		auto end = overlapList.end();
		REQUIRE(it != end);

		auto overlaps = *it;
		REQUIRE(overlaps.size() == 4);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		group.insert(overlaps[2]);
		group.insert(overlaps[3]);
		REQUIRE(group.contains(0));
		REQUIRE(group.contains(1));
		REQUIRE(group.contains(2));
		REQUIRE(group.contains(3));

		++it;
		REQUIRE(it != end);

		overlaps = *it;
		REQUIRE(overlaps.size() == 3);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		group.insert(overlaps[2]);
		REQUIRE(group.contains(1));
		REQUIRE(group.contains(2));
		REQUIRE(group.contains(3));

		++it;
		REQUIRE(it != end);

		overlaps = *it;
		REQUIRE(overlaps.size() == 2);
		group.insert(overlaps[0]);
		group.insert(overlaps[1]);
		REQUIRE(group.contains(2));
		REQUIRE(group.contains(3));
	}
}