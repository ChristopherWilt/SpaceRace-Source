#pragma once
#include "FontData.h"
#include <string>
#include <vector>

namespace DRAW
{
    std::vector<TextVertex> BuildTextMesh(
        const Font& font,
        const std::string& text,
        float startX,
        float startY
    );
}