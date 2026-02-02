#include "HighScoresManager.h"
#include "../DRAW/Utility/TextMeshBuilder.h"
#include"../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h" 
#include "../../gateware-main/Gateware.h"
#include "../../entt-3.13.1/single_include/entt/entt.hpp"

namespace GAME
{
    void UpdateNameInput(entt::registry& registry, GW::INPUT::GInput& input, int playerScore)
    {
        static std::string currentName;
        static bool nameComplete = false;

        if (nameComplete) return;

        // Input A–Z
        for (int key = G_KEY_A; key <= G_KEY_Z; ++key)
        {
            float pressed = 0;
            input.GetState(key, pressed);
            if (pressed > 0.5f && currentName.size() < 10)
                currentName += static_cast<char>('A' + (key - G_KEY_A));
        }

        // Backspace
        float back = 0;
        input.GetState(G_KEY_BACKSPACE, back);
        if (back > 0.5f && !currentName.empty())
            currentName.pop_back();

        // Enter
        float enter = 0;
        input.GetState(G_KEY_ENTER, enter);
        if (enter > 0.5f)
        {
            auto& mgr = registry.ctx().get<GAME::HighScoreManager>();
            AddHighScore(mgr, currentName.empty() ? "PLAYER" : currentName, playerScore);
            nameComplete = true;
        }

        // Render text
        auto& tr = registry.ctx().get<DRAW::TextRenderer>();
        auto verts = DRAW::BuildTextMesh(tr.font, "ENTER NAME: " + currentName, 100.f, 300.f);
        // upload and draw verts here
    }
}