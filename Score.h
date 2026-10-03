// ============================================================
//  Score.h  -  leaderboard saved in leaderboard.txt
// ============================================================
#pragma once
#include <string>
#include <vector>

struct ScoreEntry {
    std::string name;
    int         score;
    std::string tag;     // e.g. "VS AI HARD" or "PVP"
};

void saveScore(const std::string& name, int score, const std::string& tag);
std::vector<ScoreEntry> loadScores();       // sorted, best first
