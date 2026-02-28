add_library(engine 
    src/engine.cpp
    src/catalog_loader.cpp
)

target_include_directories(engine PUBLIC include)

target_link_libraries(engine PRIVATE
    supernovas::core
    supernovas::solsys-calceph
    nlohmann_json::nlohmann_json
)
