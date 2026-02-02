#include "FontData.h"
#include "FontLoader.h"
#include "..//Assets/data/json.hpp"
#include <fstream>

using json = nlohmann::json;

#include <filesystem>

namespace DRAW
{
    // Resolves an asset path relative to the project root
    std::string ResolveAssetPath(const std::string& relativePath)
    {
        namespace fs = std::filesystem;

        // Start from the current working directory (usually /build/Debug/)
        fs::path cwd = fs::current_path();

        // Walk up until we find the project root (contains "Source" folder)
        fs::path candidate = cwd;
        while (!candidate.empty() && !fs::exists(candidate / "Assets"))
            candidate = candidate.parent_path();

        if (candidate.empty()) {
            std::cerr << "Error: Could not locate project root from " << cwd << std::endl;
            return relativePath; // fallback to whatever was passed
        }

        // Return absolute path to the requested file
        fs::path fullPath = candidate / relativePath;
        return fullPath.string();
    }

    Font LoadFont(const std::string& jsonPath, const std::string& texturePath)
    {
        Font font;
        font.texturePath = texturePath;

        std::ifstream file(jsonPath);
        json j;
        file >> j;

        font.name = j["name"];
        font.size = j["size"];
        font.bold = j["bold"];
        font.italic = j["italic"];
        font.texWidth = j["width"];
        font.texHeight = j["height"];

        for (auto& [key, val] : j["characters"].items())
        {
            Glyph g;
            g.x = val["x"];
            g.y = val["y"];
            g.width = val["width"];
            g.height = val["height"];
            g.originX = val["originX"];
            g.originY = val["originY"];
            g.advance = val["advance"];
            font.glyphs[key[0]] = g;
        }

        return font;
    }
}