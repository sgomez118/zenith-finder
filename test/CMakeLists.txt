add_executable(unit_tests 
    test_catalog.cpp
    test_engine.cpp
    test_location.cpp
    test_julian.cpp
)


target_link_libraries(unit_tests PRIVATE
    engine
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
