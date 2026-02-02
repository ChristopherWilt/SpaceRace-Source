#include <iostream>
#include "../CCL.h"
#include "../entt-3.13.1/single_include/entt/entt.hpp"
#include "../gateware-main/Gateware.h"
#include "GameComponents.h"
#include "HighScoresManager.h"
#include "APP/Window.hpp"

bool quitMessageShown = false;

namespace GAME
{
    // Signal tag to reset the game
    void Update_GameOverScreen(entt::registry& registry, entt::entity entity)
    {
        UTIL::Input* inputComponent = registry.ctx().find<UTIL::Input>();
        if (!inputComponent) {
            return;
        }

        GW::INPUT::GInput& input = inputComponent->immediateInput;
		float rKey = 0.0f, hKey = 0.0f, qKey = 0.0f, mKey = 0.0f;
        input.GetState(G_KEY_R, rKey);
        input.GetState(G_KEY_H, hKey);
		input.GetState(G_KEY_Q, qKey);
		input.GetState(G_KEY_M, mKey);

        static bool messageShown = false;
        if (!messageShown)
        {
            std::cout << "GAME OVER\n"
                << "Press R to Restart\n"
                << "Press H to View High Scores\n"
				<< "Press Q to Quit\n"
				<< "Press M to return to Main Menu\n";
            messageShown = true;
        }

        if (rKey > 0.5f)
        {
            messageShown = false;

            // Set a flag to request reset instead of calling ResetGame directly
            // This prevents destroying the entity while its update callback is running
            registry.ctx().emplace<RequestReset>();

            return;
        }

        if (hKey > 0.5f)
        {
            //messageShown = false;

            // Navigate to high scores screen
            // First clear game over state
            if (registry.ctx().find<GAME::GameOver>()) {
                registry.ctx().erase<GAME::GameOver>();
            }
            if (registry.ctx().find<GAME::GameOverOverlayData>()) {
                registry.ctx().erase<GAME::GameOverOverlayData>();
            }
            // Create high scores screen
            auto hs = registry.create();
            registry.emplace<GAME::HighScoresScreen>(hs);
        }

		if (qKey > 0.5f)
		{
			if (quitMessageShown == false)
			{
                std::cout << "Quitting the game...\n";
				quitMessageShown = true;
			}
			auto winView = registry.view<APP::Window>();
			for (auto entity : winView) {
				auto& gwClose = registry.get<GW::SYSTEM::GWindow>(entity);
				GW::SYSTEM::UNIVERSAL_WINDOW_HANDLE uwh3{};
				if (+gwClose.GetWindowHandle(uwh3)) {
					HWND h = reinterpret_cast<HWND>(uwh3.window);
					PostMessage(h, WM_CLOSE, 0, 0);
				}
			}
		}

        if (mKey > 0.5f)
        {
            messageShown = false;

            // Flag to request main menu
            registry.ctx().emplace<RequestMainMenu>();
            return;
        }
        
    }

    CONNECT_COMPONENT_LOGIC()
    {
        registry.on_update<GAME::GameOverScreen>().connect<Update_GameOverScreen>();
    }
}