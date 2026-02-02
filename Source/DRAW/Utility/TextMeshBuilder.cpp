#include "FontData.h"
#include "TextMeshBuilder.h"
#include <vector>
#include <array>

namespace DRAW
{ 
    std::vector<TextVertex> BuildTextMesh(const Font& font, const std::string& text, float startX, float startY)
    {
        std::vector<TextVertex> verts;
        float x = startX;
        float y = startY;

        for (char c : text)
        {
            auto it = font.glyphs.find(c);
            if (it == font.glyphs.end()) continue;
            const Glyph& g = it->second;

            float u0 = g.x / (float)font.texWidth;
            float v0 = g.y / (float)font.texHeight;
            float u1 = (g.x + g.width) / (float)font.texWidth;
            float v1 = (g.y + g.height) / (float)font.texHeight;

            float w = (float)g.width;
            float h = (float)g.height;
            float ox = (float)g.originX;
            float oy = (float)g.originY;

            std::array<TextVertex, 6> quad = {
                TextVertex{x + ox,     y - oy,     u0, v0},
                TextVertex{x + ox + w, y - oy,     u1, v0},
                TextVertex{x + ox,     y - oy + h, u0, v1},
                TextVertex{x + ox + w, y - oy,     u1, v0},
                TextVertex{x + ox + w, y - oy + h, u1, v1},
                TextVertex{x + ox,     y - oy + h, u0, v1},
            };

            verts.insert(verts.end(), quad.begin(), quad.end());
            x += g.advance;
        }

        return verts;
    }
}