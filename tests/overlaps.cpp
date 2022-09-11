#include <array>

#include <pbd/hashing/BBox.hpp>
#include <pbd/hashing/common.hpp>
#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/OverlapList.hpp>

#include <catch2/catch_all.hpp>

using namespace pbd;

TEST_CASE("overlaps") {
	using index_t = OverlapList::index_t;

	OverlapList list;

	REQUIRE(list.size() == 0);
	REQUIRE(list.empty());

	// Test to see that empty and single element groups are discarded.
	list.group();
	list.ungroup();

	REQUIRE(list.size() == 0);

	list.group();
	list.push(1);
	list.ungroup();

	REQUIRE(list.size() == 0);

	auto it = list.begin();
	auto end = list.end();
	REQUIRE(it == end);

	list.group();
	list.push(1);
	list.push(2);
	list.push(3);
	list.ungroup();

	REQUIRE(list.size() == 1);

	it = list.begin();
	end = list.end();
	REQUIRE(it != end);

	auto overlaps = *it;
	REQUIRE(overlaps.size() == 3);
	REQUIRE(overlaps[0] == 1);
	REQUIRE(overlaps[1] == 2);
	REQUIRE(overlaps[2] == 3);

	list.group();
	list.push(4);
	list.push(5);
	list.push(6);
	list.ungroup();

	REQUIRE(list.size() == 2);
	it = list.begin();
	end = list.end();
	REQUIRE(it != end);

	++it;
	REQUIRE(it != end);
	overlaps = *it;
	REQUIRE(overlaps.size() == 3);
	REQUIRE(overlaps[0] == 4);
	REQUIRE(overlaps[1] == 5);
	REQUIRE(overlaps[2] == 6);

	list.clear();
	REQUIRE(list.size() == 0);
	REQUIRE(list.empty());

	it = list.begin();
	end = list.end();
	REQUIRE(it == end);
}