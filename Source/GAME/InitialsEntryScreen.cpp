#include "../CCL.h"
#include "../entt-3.13.1/single_include/entt/entt.hpp"
#include "../UTIL/Utilities.h"
#include "GameComponents.h"
#include "HighScoresManager.h"
#include <iostream>

namespace GAME
{
    void Update_InitialsEntryScreen(entt::registry& registry, entt::entity entity)
    {
        auto* inputComponent = registry.ctx().find<UTIL::Input>();
        if (!inputComponent) return;

        // We are using PlayerNameInput from the registry context
        auto* nameInputPtr = registry.ctx().find<PlayerNameInput>();
        if (!nameInputPtr) return;

        PlayerNameInput& nameInput = *nameInputPtr;
        GW::INPUT::GInput& input = inputComponent->immediateInput;

        // Ensure fixed-length name (e.g., "AAA")
        if ((int)nameInput.name.size() != nameInput.maxLength) {
            nameInput.name.assign(nameInput.maxLength, 'A');
            nameInput.currentIndex = 0;
        }

        // ---- INPUT HANDLING ----
        float leftVal = 0, rightVal = 0, upVal = 0, downVal = 0;
        float aVal = 0, dVal = 0, wVal = 0, sVal = 0;
        float enterVal = 0, backVal = 0;

        input.GetState(G_KEY_LEFT, leftVal);
        input.GetState(G_KEY_RIGHT, rightVal);
        input.GetState(G_KEY_UP, upVal);
        input.GetState(G_KEY_DOWN, downVal);

        input.GetState(G_KEY_A, aVal);
        input.GetState(G_KEY_D, dVal);
        input.GetState(G_KEY_W, wVal);
        input.GetState(G_KEY_S, sVal);

        input.GetState(G_KEY_ENTER, enterVal);
        input.GetState(G_KEY_BACKSPACE, backVal);

        bool leftPressed = (leftVal > 0.5f) || (aVal > 0.5f);
        bool rightPressed = (rightVal > 0.5f) || (dVal > 0.5f);
        bool upPressed = (upVal > 0.5f) || (wVal > 0.5f);
        bool downPressed = (downVal > 0.5f) || (sVal > 0.5f);
        bool enterPressed = (enterVal > 0.5f);
        bool backPressed = (backVal > 0.5f);

        static bool leftWas = false;
        static bool rightWas = false;
        static bool upWas = false;
        static bool downWas = false;
        static bool enterWas = false;
        static bool backWas = false;

        int maxIndex = nameInput.maxLength - 1;

        // Move which letter is selected
        if (leftPressed && !leftWas) {
            nameInput.currentIndex--;
            if (nameInput.currentIndex < 0)
                nameInput.currentIndex = maxIndex;
        }
        if (rightPressed && !rightWas) {
            nameInput.currentIndex++;
            if (nameInput.currentIndex > maxIndex)
                nameInput.currentIndex = 0;
        }

        // Change the letter at current index
        char& c = nameInput.name[nameInput.currentIndex];

        if (upPressed && !upWas) {
            if (c < 'A' || c > 'Z') c = 'A';
            else c = (c == 'Z') ? 'A' : char(c + 1);
        }
        if (downPressed && !downWas) {
            if (c < 'A' || c > 'Z') c = 'A';
            else c = (c == 'A') ? 'Z' : char(c - 1);
        }

        // BACKSPACE resets initials to "AAA"
        if (backPressed && !backWas) {
            nameInput.name.assign(nameInput.maxLength, 'A');
            nameInput.currentIndex = 0;
        }

        // Confirm with ENTER
        if (enterPressed && !enterWas) {
            nameInput.entryComplete = true;
        }

        leftWas = leftPressed;
        rightWas = rightPressed;
        upWas = upPressed;
        downWas = downPressed;
        enterWas = enterPressed;
        backWas = backPressed;

        // Still editing initials? Then stop here.
        if (!nameInput.entryComplete)
            return;

        long finalScore = 0;

        // 1) Prefer global run score
        if (auto* globalScore = registry.ctx().find<GAME::Score>()) {
            finalScore = globalScore->score;
        }

        // 2) Fallback: any Player + Score entity
        if (finalScore == 0) {
            auto view = registry.view<GAME::Player, GAME::Score>();
            if (view.begin() != view.end()) {
                entt::entity player = *view.begin();
                finalScore = view.get<GAME::Score>(player).score;
            }
        }

        if (finalScore == 0) {
            std::cout << "[InitialsEntry] Warning: finalScore is 0 (maybe no score was tracked?)\n";
        }

        if (auto* manager = registry.ctx().find<HighScoreManager>()) {
            AddHighScore(*manager, nameInput.name, static_cast<int>(finalScore));
            std::cout << "Saved high score for " << nameInput.name
                << " with score " << finalScore << std::endl;
        }

        // Clean up name input in context
        if (registry.ctx().find<PlayerNameInput>())
            registry.ctx().erase<PlayerNameInput>();

        // Destroy initials screen entity
        registry.destroy(entity);

        // Now go to GameOverScreen PNG menu
        entt::entity gameOverEntity = registry.create();
        registry.emplace<GAME::GameOverScreen>(gameOverEntity);
    }

    CONNECT_COMPONENT_LOGIC()
    {
        registry.on_update<GAME::InitialsEntryScreen>()
            .connect<Update_InitialsEntryScreen>();
    }
}
