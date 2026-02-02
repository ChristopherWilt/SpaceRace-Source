#pragma once
#include "FontData.h"
#include <string>
#include <vector>

namespace DRAW
{
    std::string ResolveAssetPath(const std::string& relativePath);

    // Loads a font definition (.json) and its texture (.png)
    Font LoadFont(const std::string& jsonPath, const std::string& texturePath);
}