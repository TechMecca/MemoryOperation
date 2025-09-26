add_library(MemoryOperation STATIC)

target_sources(MemoryOperation PUBLIC
       
       "src/PatternScanner.cpp"
       "src/Patch.cpp" 
       "src/WinDetour.cpp" 
       "src/MemoryOperation.cpp"
       "src/Memory.cpp"
       "src/MemoryOperator.cpp"
       "src/WinConsole.cpp")

target_include_directories(MemoryOperation PUBLIC
    "Include"
)


add_library(MemoryOperation::MemoryOperation ALIAS MemoryOperation)
