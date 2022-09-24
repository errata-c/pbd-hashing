
# Add in the local phmap installation
find_path(PHMAP_HEADERS "parallel_hashmap/phmap.h")
if("${PHMAP_HEADERS}" STREQUAL "PHMAP_HEADERS-NOTFOUND")
	message(FATAL_ERROR "Failed to find the parallel_hashmap headers!")
endif()

set_property(
	TARGET pbd::hashing 
	APPEND
	PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PHMAP_HEADERS}
)