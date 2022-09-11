project(PBD_HASHING_TESTS)

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

add_executable(basic_test
	"common.cpp"
	"overlaps.cpp"
	"base_table.cpp"
	"htable.cpp"
)
target_link_libraries(basic_test PRIVATE
	pbd::hashing
	Catch2::Catch2WithMain
)

catch_discover_tests(basic_test)