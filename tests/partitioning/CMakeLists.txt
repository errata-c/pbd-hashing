
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(blend2d CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(partitioning_test
	"main.cpp"
	"Context.cpp"
	"Renderer.cpp"
)
target_compile_features(partitioning_test PRIVATE cxx_std_17)
target_link_libraries(partitioning_test 
PRIVATE
	imgui::imgui
	ImGui-SFML::ImGui-SFML
	sfml-system
	sfml-window
	sfml-graphics
	blend2d::blend2d
	fmt::fmt
	glm::glm
	GLEW::GLEW

	pbd::hashing
)