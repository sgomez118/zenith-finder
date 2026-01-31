add_executable(zenith-finder 
    main.cpp
    windows_location_provider.cpp
    logger.cpp
)

target_link_libraries(zenith-finder PRIVATE
    engine
    CLI11::CLI11
)

if(WIN32)
    target_link_libraries(zenith-finder PRIVATE LocationAPI)
endif()
