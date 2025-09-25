add_library(MemoryOperation STATIC)

target_sources(MemoryOperation PUBLIC
       
       "src/PatternScanner.cpp"
       "src/Patch.cpp" "Include/WinDetour.h" "src/WinDetour.cpp" "src/MemoryOperation.cpp" "Include/Memory.h" "src/Memory.cpp")

target_include_directories(MemoryOperation PUBLIC
    "Include"
)


add_library(MemoryOperation::MemoryOperation ALIAS MemoryOperation)
