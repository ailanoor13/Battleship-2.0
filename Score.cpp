#include "Score.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

static const char* FILE_NAME = "leaderboard.txt";

void saveScore(const std::string& name, int score, const std::string& tag) {
    std::ofstream out(FILE_NAME, std::ios::app);
    if (out.is_open())
        out << name << "|" << score << "|" << tag << "\n";
}

static bool byScoreDesc(const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; }

std::vector<ScoreEntry> loadScores() {
    std::vector<ScoreEntry> list;
    std::ifstream in(FILE_NAME);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        ScoreEntry e;
        size_t p1 = line.find('|');
        if (p1 != std::string::npos) {                 // new format  name|score|tag
            size_t p2 = line.find('|', p1 + 1);
            e.name = line.substr(0, p1);
            std::string sc = (p2 == std::string::npos) ? line.substr(p1 + 1) : line.substr(p1 + 1, p2 - p1 - 1);
            e.score = std::atoi(sc.c_str());
            e.tag = (p2 == std::string::npos) ? "" : line.substr(p2 + 1);
        } else {                                       // old console format  name score
            std::istringstream ss(line);
            ss >> e.name >> e.score;
            e.tag = "CLASSIC";
        }
        while (!e.tag.empty() && (e.tag.back() == '\r' || e.tag.back() == '\n')) e.tag.pop_back();
        list.push_back(e);
    }
    std::stable_sort(list.begin(), list.end(), byScoreDesc);
    return list;
}
