add_executable(zenith-finder main.cpp)

target_link_libraries(zenith-finder PRIVATE
    engine
    CLI11::CLI11
)
