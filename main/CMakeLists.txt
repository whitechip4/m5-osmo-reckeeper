idf_component_register(
    SRCS "main.c" "ble_simple.c" "m5_wrapper.cpp"
         "dji_protocol.c" "storage.c" "gps_module.c"
         "ui/ui_layout.c" "ui/ui.c" "ui/ui_state.c" "ui/ui_renderer.c"
         "display/display_helpers.c"
         "handlers/button_handlers.c" "handlers/status_updater.c" "handlers/system_init.c"
         "protocol/dji_protocol_parser.c"
         "protocol/crc16.c" "protocol/crc32.c"
    INCLUDE_DIRS "." "protocol" "ui" "handlers" "display"
    PRIV_REQUIRES M5Unified bt nvs_flash driver
)

# Ensure C++ wrapper file is compiled as C++
set_source_files_properties(m5_wrapper.cpp PROPERTIES LANGUAGE CXX)
