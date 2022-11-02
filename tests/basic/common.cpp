#include <array>

#include <pbd/hashing/BBox.hpp>
#include <pbd/hashing/common.hpp>
#include <pbd/hashing/Grid.hpp>

#include <catch2/catch_all.hpp>


using namespace pbd;

using Catch::Approx;

TEST_CASE("BBox", "[common]") {
	using bbox_t = BBox<2, float>;
	using vec_t = bbox_t::vec_t;

	SECTION("Constructors") {
		// Default
		bbox_t test0;

		REQUIRE(test0.area() == Approx(0.f));
		REQUIRE(test0.min.x == Approx(0.f));
		REQUIRE(test0.min.y == Approx(0.f));
		REQUIRE(test0.max.x == Approx(0.f));
		REQUIRE(test0.max.y == Approx(0.f));

		REQUIRE(test0.isValid() == false);


		// Min max
		bbox_t test1(vec_t(0), vec_t(1));
		REQUIRE(test1.area() == Approx(1.f));
		
		REQUIRE(test1.min.x == Approx(0.f));
		REQUIRE(test1.min.y == Approx(0.f));
		REQUIRE(test1.max.x == Approx(1.f));
		REQUIRE(test1.max.y == Approx(1.f));

		REQUIRE(test1.isValid() == true);
	}

	SECTION("Translation") {
		bbox_t test0(vec_t(0), vec_t(1));
		bbox_t test1 = test0;

		test0.translate(vec_t(1));
		
		test1.centerTo(vec_t(1.5f));

		REQUIRE(test0.min.x == Approx(test1.min.x));
		REQUIRE(test0.min.y == Approx(test1.min.y));
		REQUIRE(test0.max.x == Approx(test1.max.x));
		REQUIRE(test0.max.y == Approx(test1.max.y));

		REQUIRE(test0 == test1);
	}

	SECTION("Scale") {
		bbox_t test0(vec_t(0), vec_t(1));

		REQUIRE(test0.scaled(2.f) == bbox_t(vec_t(0.f), vec_t(2.f)));
		REQUIRE(test0.scaled(vec_t(2.f, 3.f)) == bbox_t(vec_t(0.f), vec_t(2.f, 3.f)));
		REQUIRE(test0.scaled(vec_t(0.5f), vec_t(2.f)) == bbox_t(vec_t(-0.5f), vec_t(1.5f)));
	}
	SECTION("Expand") {
		bbox_t test0(vec_t(0), vec_t(1));

		REQUIRE( test0.expanded(1.f) == bbox_t(vec_t(-1), vec_t(2)) );
		REQUIRE( test0.expanded(vec_t(1, 2)) == bbox_t(vec_t(-1, -2), vec_t(2, 3)) );
		REQUIRE( test0.expanded(vec_t(1, 2), vec_t(3.f, 4.f)) == bbox_t(vec_t(-1, -2), vec_t(4, 5)) );
	}
	SECTION("Shrink") {
		bbox_t test0(vec_t(0), vec_t(10));

		REQUIRE(test0.shrinked(1) == bbox_t(vec_t(1), vec_t(9)));
		REQUIRE(test0.shrinked(vec_t(1, 2)) == bbox_t(vec_t(1, 2), vec_t(9, 8)));
		REQUIRE(test0.shrinked(vec_t(1, 2), vec_t(3, 4)) == bbox_t(vec_t(1, 2), vec_t(7, 6)));

		REQUIRE_FALSE(test0.shrinked(1) != bbox_t(vec_t(1), vec_t(9)));
		REQUIRE_FALSE(test0.shrinked(vec_t(1, 2)) != bbox_t(vec_t(1, 2), vec_t(9, 8)));
		REQUIRE_FALSE(test0.shrinked(vec_t(1, 2), vec_t(3, 4)) != bbox_t(vec_t(1, 2), vec_t(7, 6)));
	}
	SECTION("Merge") {
		bbox_t test0(vec_t(0), vec_t(1));

		bbox_t test1(vec_t(1), vec_t(2));
		bbox_t test2(vec_t(-1), vec_t(0.5));
		bbox_t test3(vec_t(-3), vec_t(-2));

		REQUIRE(test0.merged(test1) == bbox_t(vec_t(0), vec_t(2)));
		REQUIRE(test0.merged(test2) == bbox_t(vec_t(-1), vec_t(1)));
		REQUIRE(test0.merged(test3) == bbox_t(vec_t(-3), vec_t(1)));

		REQUIRE_FALSE(test0.merged(test1) != bbox_t(vec_t(0), vec_t(2)));
		REQUIRE_FALSE(test0.merged(test2) != bbox_t(vec_t(-1), vec_t(1)));
		REQUIRE_FALSE(test0.merged(test3) != bbox_t(vec_t(-3), vec_t(1)));
	}

	SECTION("Contains") {
		bbox_t test0(vec_t(-1), vec_t(1));

		bbox_t test1(vec_t(0), vec_t(1));
		bbox_t test2(vec_t(-0.5), vec_t(0.5));
		bbox_t test3(vec_t(-3), vec_t(-2));

		REQUIRE(test0.contains(test1));
		REQUIRE(test0.contains(test2));
		REQUIRE(!test0.contains(test3));
	}

	SECTION("Overlaps") {
		bbox_t test0(vec_t(-1), vec_t(1));

		bbox_t 
			test1(vec_t(0), vec_t(1)),
			test2(vec_t(-0.5), vec_t(0.5)),
			test3(vec_t(-3), vec_t(-2)),
			test4(vec_t(0), vec_t(2));

		REQUIRE(test0.overlaps(test1));
		REQUIRE(test0.overlaps(test2));
		REQUIRE_FALSE(test0.overlaps(test3));
		REQUIRE(test0.overlaps(test4));
	}
};

TEST_CASE("applyAllCells", "[common]") {
	using ivec_t = glm::vec<3, int>;

	auto && [first, last] = GENERATE(
		std::make_pair(ivec_t(0), ivec_t(0)), 
		std::make_pair(ivec_t(0), ivec_t(2)),
		std::make_pair(ivec_t(1), ivec_t(3)),
		std::make_pair(ivec_t(5), ivec_t(10))
	);

	std::vector<int> passes;

	auto resize = [](std::vector<int> & passes, const ivec_t & _cells) {
		ivec_t cells = _cells + ivec_t(1);
		size_t count = cells[0] * cells[1] * cells[2];
		passes.clear();
		passes.resize(count, 0);
	};
	
	resize(passes, last - first);

	size_t i = 0;
	applyAllCells(first, last, [&](const ivec_t & loc){
		passes[i] = 1;
		++i;
	});

	INFO("first: [" << first.x << ", " << first.y << ", " << first.z << "]");
	INFO("last:  [" << last.x << ", " << last.y << ", " << last.z << "]");

	REQUIRE(i == passes.size());
	for (int v : passes) {
		REQUIRE(v == 1);
	}
}