#pragma once
#include <string>
#include <map>

namespace DRAW
{
    struct Glyph
    {
        int x, y;
        int width, height;
        int originX, originY;
        int advance;
    };

    struct Font
    {
        std::string name;
        int size;
        bool bold;
        bool italic;
        int texWidth;
        int texHeight;
        std::map<char, Glyph> glyphs;
        std::string texturePath;
    };

    struct TextVertex
    {
        float x, y;   // Position
        float u, v;   // Texture coordinates
    };
}