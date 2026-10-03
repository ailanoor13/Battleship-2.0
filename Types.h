// ============================================================
//  Types.h  -  core game data (no graphics code in here)
// ============================================================
#pragma once
#include <string>
#include <vector>

const int GRID      = 10;   // board is GRID x GRID
const int NUM_SHIPS = 5;    // fleet size

// what we know about a cell after shooting at it
enum ShotState { SHOT_NONE = 0, SHOT_MISS = 1, SHOT_HIT = 2, SHOT_SUNK = 3 };

enum GameMode  { MODE_VS_AI = 0, MODE_VS_PLAYER = 1 };
enum Difficulty { DIFF_EASY = 0, DIFF_MEDIUM = 1, DIFF_HARD = 2 };

struct Ship {
    const char* name;
    int  size;
    int  row, col;        // top-left cell of the ship
    bool horizontal;      // true = goes right, false = goes down
    bool placed;
    int  hits;            // how many cells have been hit
};

struct Board {
    Ship ships[NUM_SHIPS];
    int  shipAt[GRID][GRID];   // -1 = water, otherwise index of the ship in ships[]
    int  shot[GRID][GRID];     // ShotState of every cell
};

struct ShotResult {
    bool valid;      // false if the cell was already shot or out of range
    bool hit;
    int  shipIndex;  // which ship was hit (-1 on a miss)
    bool sunk;       // did this shot sink the ship?
    bool allSunk;    // was that the last ship?
};

struct AIPick { int r, c; };

// ---------- board / fleet ----------
void resetBoard(Board& b);
bool isSunk(const Ship& s);
bool canPlace(const Board& b, int shipIdx, int r, int c, bool horizontal);
bool placeShip(Board& b, int shipIdx, int r, int c, bool horizontal);
void removeShip(Board& b, int shipIdx);
void randomizeFleet(Board& b);
bool fleetReady(const Board& b);

// ---------- shooting ----------
ShotResult fireAt(Board& b, int r, int c);
int  shipsAlive(const Board& b);
bool allShipsSunk(const Board& b);
int  shotsForTurn(const Board& b, bool salvo);   // salvo: ships alive + 1, classic: 1

// ---------- computer opponent ----------
AIPick aiChooseShot(const Board& enemy, Difficulty d);

// ---------- random helper ----------
int randInt(int n);   // 0 .. n-1
