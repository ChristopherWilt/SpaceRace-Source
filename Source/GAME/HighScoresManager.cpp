#include "HighScoresManager.h"
#include "../json-develop/single_include/nlohmann/json.hpp"
#include "../DRAW/DrawComponents.h"
#include "../DRAW/Utility/TextMeshBuilder.h"
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

namespace GAME
{
    static std::string HighScoreFilePath()
    {
        std::filesystem::path p = std::filesystem::current_path();
        for (int i = 0; i < 6; ++i) {
            auto candidate = p / "assets" / "data" / "highscores.json";
            if (std::filesystem::exists(candidate))
                return candidate.string();
            p = p.parent_path();
        }
        return "assets/data/highscores.json";
    }

    void LoadHighScores(HighScoreManager& manager)
    {
        manager.entries.clear();
        std::ifstream file(HighScoreFilePath());
        if (!file.is_open()) return;

        json j; file >> j;
        for (auto& item : j)
            manager.entries.push_back({ item["name"], item["score"] });
    }

    void SaveHighScores(const HighScoreManager& manager)
    {
        json j = json::array();
        for (auto& e : manager.entries)
            j.push_back({ {"name", e.name}, {"score", e.scores} });

        std::ofstream file(HighScoreFilePath());
        file << j.dump(4);
    }

    void AddHighScore(HighScoreManager& manager, const std::string& name, int score)
    {
        // Add new entry
        manager.entries.push_back({ name, score });

        // Sort descending by score
        std::sort(manager.entries.begin(), manager.entries.end(),
            [](const HighScoreEntry& a, const HighScoreEntry& b) {
                return a.scores > b.scores;
            });

        // Keep only top 10
        if (manager.entries.size() > 10)
            manager.entries.resize(10);

        // Save updated scores to file
        SaveHighScores(manager);
    }
}
