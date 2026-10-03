// ============================================================
//  Game.h  -  all game state + the functions main() calls
// ============================================================
#pragma once
#include "Compat.h"
#include "Types.h"
#include "Fx.h"
#include "Score.h"

const int W = 1200;      // window size
const int H = 675;

enum Screen { SC_MENU, SC_SETUP, SC_PLACE, SC_COVER, SC_BATTLE, SC_RESULT, SC_SCORES, SC_HELP };
enum Phase  { PH_AIM, PH_AI_THINK, PH_FLIGHT, PH_AFTER_SHOT, PH_TURN_END };
enum FxType { FX_RIPPLE, FX_BLAST, FX_POPUP };
enum CoverNext { COVER_TO_PLACE, COVER_TO_TURN };

struct Player {
    std::string name;
    bool  isAI;
    Board board;
    int   score;
    int   shotsFired;
    int   hits;
};

struct Effect   { int type; float x, y, t, dur; bool big; std::string text; sf::Color color; };
struct Particle { float x, y, vx, vy, life, maxLife, size; sf::Color color; };
struct LogLine  { std::string text; sf::Color color; };

struct Game {
    // system
    sf::RenderWindow* win;
    Audio  audio;
    bool   running;
    float  time;
    float  fade;
    int    mx, my;            // mouse position
    int    hot;               // id of the button under the mouse (-1 = none)

    // screens
    Screen screen;
    int    menuSel;

    // new-game options
    GameMode   mode;
    Difficulty diff;
    bool       salvo;
    std::string names[2];
    int        focus;         // which name box is being typed in

    // one match
    Player pl[2];
    int    placing;           // whose fleet is being placed
    int    cur;               // whose turn it is (attacker)
    int    viewer;            // whose screen we are showing
    int    turnNo;
    int    shotsLeft, shotsTotal;

    // placement
    int  selShip;
    bool horiz;

    // pass-the-device screen
    int coverFor;
    int coverNext;

    // battle
    Phase phase;
    float phaseT;
    int   tgtR, tgtC;
    bool  gameOver;
    int   winner;
    bool  confirmQuit;
    float shake;
    std::vector<Effect>   fx;
    std::vector<Particle> parts;
    std::vector<LogLine>  log;

    // result / leaderboard
    bool scoreSaved;
    std::vector<ScoreEntry> scores;

    Game();
};

void gameInit(Game& g, sf::RenderWindow* win);
void gameEvent(Game& g, const compat::Ev& e);
void gameUpdate(Game& g, float dt);
void gameDraw(Game& g, sf::RenderTarget& t);

// exposed so the test harness (and readers) can jump around
void changeScreen(Game& g, Screen s);
void startSession(Game& g);
