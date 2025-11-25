cmake_minimum_required(VERSION 3.24)

# Use the prebuilt Detours library instead of building from source
set(DETOURS_PREBUILT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt")
set(DETOURS_INCLUDE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Set the library file based on architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(DETOURS_LIB "${DETOURS_PREBUILT_DIR}/x64/detours.lib")
else()
    set(DETOURS_LIB "${DETOURS_PREBUILT_DIR}/detours.lib")
endif()

# Verify the library exists
if(NOT EXISTS "${DETOURS_LIB}")
    message(FATAL_ERROR "Detours library not found at: ${DETOURS_LIB}")
endif()

# Create imported target
add_library(Detours STATIC IMPORTED GLOBAL)
set_target_properties(Detours PROPERTIES
    IMPORTED_LOCATION             "${DETOURS_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${DETOURS_INCLUDE_DIR}"
)

# Nice namespaced alias
add_library(Detours::Detours ALIAS Detours)

message(STATUS "Using prebuilt Detours library: ${DETOURS_LIB}")
