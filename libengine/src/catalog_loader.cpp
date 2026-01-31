#include "catalog_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace engine {

std::vector<Star> CatalogLoader::LoadFromCSV(const std::filesystem::path& path) {
    std::vector<Star> stars;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        return stars;
    }

    std::string line;
    // Skip header if exists (assume name,ra,dec)
    if (std::getline(file, line)) {
        // Simple check if first line is header
        if (line.find("name") == std::string::npos && line.find(",") != std::string::npos) {
            // Not a header, rewind or handle? 
            // For now, assume there IS a header.
        }
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string name, ra_str, dec_str;
        
        if (std::getline(ss, name, ',') &&
            std::getline(ss, ra_str, ',') &&
            std::getline(ss, dec_str, ',')) {
            try {
                stars.push_back({
                    name,
                    std::stod(ra_str),
                    std::stod(dec_str)
                });
            } catch (...) {
                // Skip malformed lines
            }
        }
    }

    return stars;
}

} // namespace engine
