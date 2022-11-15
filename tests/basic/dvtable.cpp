#include <glm/gtx/io.hpp>
#include <array>

#include <pbd/common/BBox.hpp>
#include <pbd/hashing/util.hpp>
#include <pbd/hashing/Grid.hpp>
#include <pbd/hashing/DVTable.hpp>

#include <catch2/catch_all.hpp>

using namespace pbd;

using Table = DVTable<float, int32_t, 3>;

TEST_CASE("dvtable 1") {
	using bbox_t = Table::bbox_t;
	using index_t = Table::index_t;
	using vec_t = Table::vec_t;
	using ivec_t = Table::ivec_t;
	using grid_t = Table::grid_t;
	using CellRange = Table::CellRange;

	Table table;
	REQUIRE(table.numCells() == 0);
	
	table.initialize(vec_t(0.0001f));
	REQUIRE(table.numCells() == 0);

	std::vector<vec_t> points;
	std::vector<index_t> ids;

	points.push_back(vec_t(0.1f));
	points.push_back(vec_t(0.2f));
	points.push_back(vec_t(0.3f));
	points.push_back(vec_t(0.4f));
	ids.push_back(0);
	ids.push_back(1);
	ids.push_back(2);
	ids.push_back(3);

	grid_t grid = table.getGrid();

	ivec_t
		c0 = grid.calcCell(points[0]),
		c1 = grid.calcCell(points[1]),
		c2 = grid.calcCell(points[2]),
		c3 = grid.calcCell(points[3]);

	REQUIRE(c0 != c1);
	REQUIRE(c0 != c2);
	REQUIRE(c0 != c3);
	REQUIRE(c1 != c2);
	REQUIRE(c1 != c3);
	REQUIRE(c2 != c3);

	table.build(ids.data(), points.data(), points.size());

	CellRange cells; 

	cells = table.find(points[0]);
	REQUIRE(cells);
	REQUIRE(cells.size() == 1);

	cells = table.find(points[1]);
	REQUIRE(cells);
	REQUIRE(cells.size() == 1);

	cells = table.find(points[2]);
	REQUIRE(cells);
	REQUIRE(cells.size() == 1);

	cells = table.find(points[3]);
	REQUIRE(cells);
	REQUIRE(cells.size() == 1);
}

TEST_CASE("dvtable 2") {
	using bbox_t = Table::bbox_t;
	using index_t = Table::index_t;
	using vec_t = Table::vec_t;
	using ivec_t = Table::ivec_t;
	using grid_t = Table::grid_t;
	using CellRange = Table::CellRange;

	Table table;
	REQUIRE(table.numCells() == 0);

	table.initialize(vec_t(0.0001f));
	REQUIRE(table.numCells() == 0);

	std::vector<vec_t> points;

	points.push_back(vec_t(0.1f));
	points.push_back(vec_t(0.2f));
	points.push_back(vec_t(0.3f));
	points.push_back(vec_t(0.4f));

	grid_t grid = table.getGrid();

	ivec_t
		c0 = grid.calcCell(points[0]),
		c1 = grid.calcCell(points[1]),
		c2 = grid.calcCell(points[2]),
		c3 = grid.calcCell(points[3]);

	REQUIRE(c0 != c1);
	REQUIRE(c0 != c2);
	REQUIRE(c0 != c3);
	REQUIRE(c1 != c2);
	REQUIRE(c1 != c3);
	REQUIRE(c2 != c3);

	table.build(points.data(), points.size());

	CellRange cells;

	cells = table.find(points[0]);
	REQUIRE(cells);
	REQUIRE(cells.size() == 1);

	cells = table.find(points[1]);
	REQUIRE(cells);
	REQUIRE(cells.size() == 1);

	cells = table.find(points[2]);
	REQUIRE(cells);
	REQUIRE(cells.size() == 1);

	cells = table.find(points[3]);
	REQUIRE(cells);
	REQUIRE(cells.size() == 1);
}