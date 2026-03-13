idf_component_register(
    SRCS "main.c" "ble_simple.c" "m5_wrapper.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES M5Unified bt nvs_flash
)

# Ensure C++ wrapper file is compiled as C++
set_source_files_properties(m5_wrapper.cpp PROPERTIES LANGUAGE CXX)
