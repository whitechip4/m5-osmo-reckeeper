idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES M5Unified
)

# Ensure C++ files are compiled as C++
set_source_files_properties(main.cpp PROPERTIES LANGUAGE CXX)
