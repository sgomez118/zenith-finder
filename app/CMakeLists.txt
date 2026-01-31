add_executable(zenith-finder 
    main.cpp
    windows_location_provider.cpp
    logger.cpp
    config_manager.cpp
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

    target_link_libraries(zenith-finder PRIVATE LocationAPI)

endif()
