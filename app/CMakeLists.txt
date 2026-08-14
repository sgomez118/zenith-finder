configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
    @ONLY
)

add_executable(zenith-finder 
    main.cpp
    app_controller.cpp
    logger.cpp
    config_manager.cpp
    ui/zenith_ui.cpp
)

target_include_directories(zenith-finder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(zenith-finder PRIVATE
    engine
    CLI11::CLI11
    ftxui::screen
    ftxui::dom
    ftxui::component
    tomlplusplus::tomlplusplus
)

if(WIN32)
    target_sources(zenith-finder PRIVATE windows_location_provider.cpp)
    target_link_libraries(zenith-finder PRIVATE LocationAPI)
endif()
