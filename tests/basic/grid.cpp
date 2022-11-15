#include <array>

#include <pbd/common/BBox.hpp>
#include <pbd/hashing/util.hpp>
#include <pbd/hashing/Grid.hpp>

#include <catch2/catch_all.hpp>

using namespace pbd;

using Catch::Approx;

TEST_CASE("grid") {
	using grid_t = Grid<int, 3, float>;
	using vec_t = grid_t::vec_t;
	using ivec_t = grid_t::ivec_t;

	grid_t grid(vec_t(1.0));

	ivec_t p0 = grid.calcCell(vec_t(0.5));
	REQUIRE(p0 == ivec_t(0));

	ivec_t p1 = grid.calcCell(vec_t(1.5));
	REQUIRE(p1 == ivec_t(1));

	grid = grid_t(vec_t(0.01));

	p0 = grid.calcCell(vec_t(0.025));
	p1 = grid.calcCell(vec_t(0.035));
	REQUIRE(p0 == ivec_t(2));
	REQUIRE(p1 == ivec_t(3));
}