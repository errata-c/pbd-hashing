# Find all dependencies

if(NOT TARGET "glm::glm")
	find_dependency(glm CONFIG)
endif()