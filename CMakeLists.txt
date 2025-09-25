cmake_minimum_required(VERSION 3.24)
project(MemoryOperationProject VERSION 0.4.33 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)



add_subdirectory(External/Detours)  # defines target: detours       (and alias Detours::Detours) (IMPORTED GLOBAL) + detours_ep
add_subdirectory(MemoryOperation)   # defines target: MemoryOperation (and alias MemoryOperation::MemoryOperation) (STATIC)

add_dependencies(MemoryOperation Detours)

target_link_libraries(MemoryOperation PUBLIC Detours::Detours)


function(set_output_names target_name)
    set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME_DEBUG           "${target_name}-${ARCH_SUFFIX}-Debug"
            OUTPUT_NAME_RELEASE         "${target_name}-${ARCH_SUFFIX}"
            OUTPUT_NAME_RELWITHDEBINFO  "${target_name}-${ARCH_SUFFIX}-RelWithDebInfo"
            OUTPUT_NAME_MINSIZEREL      "${target_name}-${ARCH_SUFFIX}-MinSizeRel"
    )
endfunction()

set_output_names(MemoryOperation)

# --- Install the libraries ---
include(GNUInstallDirs)

# NOTE:
# - ARCHIVE for .lib/.a (static libs)
# - LIBRARY for .so/.dylib (shared libs)
# - RUNTIME for .dll/.exe on Windows
install(TARGETS MemoryOperation
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)


install(DIRECTORY MemoryOperation/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MemoryOperation
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")