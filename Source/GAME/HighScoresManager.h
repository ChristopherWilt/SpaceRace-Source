#pragma once
#include <string>
#include <vector>

namespace GAME
{
    struct HighScoreEntry
    {
        std::string name;
        int scores;
    };

    struct HighScoreManager
    {
        std::vector<HighScoreEntry> entries;
    };


    //functions

    void LoadHighScores(HighScoreManager& manager);
    void SaveHighScores(const HighScoreManager& manager);
    void AddHighScore(HighScoreManager& manager, const std::string& name, int score);
    void DrawHighScores(entt::registry& registry);
}