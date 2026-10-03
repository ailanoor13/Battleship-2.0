// ============================================================
//  Engine.cpp  -  fleet placement, shooting rules and the AI
//  Pure C++ (no SFML), so it is easy to test and explain.
// ============================================================
#include "Types.h"
#include <random>
#include <ctime>

static std::mt19937& rng() {
    static std::mt19937 gen((unsigned)std::time(nullptr));
    return gen;
}

int randInt(int n) {
    if (n <= 1) return 0;
    std::uniform_int_distribution<int> d(0, n - 1);
    return d(rng());
}

static bool inBounds(int r, int c) { return r >= 0 && r < GRID && c >= 0 && c < GRID; }

// ------------------------------------------------------------
//  Board and fleet
// ------------------------------------------------------------
void resetBoard(Board& b) {
    static const char* names[NUM_SHIPS] = { "Aircraft Carrier", "Battleship", "Destroyer", "Submarine", "Patrol Boat" };
    static const int   sizes[NUM_SHIPS] = { 5, 4, 3, 3, 2 };
    for (int i = 0; i < NUM_SHIPS; i++) {
        b.ships[i].name = names[i];
        b.ships[i].size = sizes[i];
        b.ships[i].row = 0;
        b.ships[i].col = 0;
        b.ships[i].horizontal = true;
        b.ships[i].placed = false;
        b.ships[i].hits = 0;
    }
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++) {
            b.shipAt[r][c] = -1;
            b.shot[r][c] = SHOT_NONE;
        }
}

bool isSunk(const Ship& s) { return s.placed && s.hits >= s.size; }

bool canPlace(const Board& b, int shipIdx, int r, int c, bool horizontal) {
    int size = b.ships[shipIdx].size;
    for (int i = 0; i < size; i++) {
        int rr = horizontal ? r : r + i;
        int cc = horizontal ? c + i : c;
        if (!inBounds(rr, cc)) return false;
        if (b.shipAt[rr][cc] != -1 && b.shipAt[rr][cc] != shipIdx) return false;
    }
    return true;
}

void removeShip(Board& b, int shipIdx) {
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++)
            if (b.shipAt[r][c] == shipIdx) b.shipAt[r][c] = -1;
    b.ships[shipIdx].placed = false;
}

bool placeShip(Board& b, int shipIdx, int r, int c, bool horizontal) {
    removeShip(b, shipIdx);                       // moving a ship = remove + place again
    if (!canPlace(b, shipIdx, r, c, horizontal)) return false;
    Ship& s = b.ships[shipIdx];
    for (int i = 0; i < s.size; i++) {
        int rr = horizontal ? r : r + i;
        int cc = horizontal ? c + i : c;
        b.shipAt[rr][cc] = shipIdx;
    }
    s.row = r; s.col = c; s.horizontal = horizontal; s.placed = true; s.hits = 0;
    return true;
}

void randomizeFleet(Board& b) {
    resetBoard(b);
    for (int i = 0; i < NUM_SHIPS; i++) {
        bool done = false;
        while (!done) {
            int r = randInt(GRID), c = randInt(GRID);
            bool h = randInt(2) == 0;
            done = placeShip(b, i, r, c, h);
        }
    }
}

bool fleetReady(const Board& b) {
    for (int i = 0; i < NUM_SHIPS; i++)
        if (!b.ships[i].placed) return false;
    return true;
}

// ------------------------------------------------------------
//  Shooting
// ------------------------------------------------------------
ShotResult fireAt(Board& b, int r, int c) {
    ShotResult res;
    res.valid = false; res.hit = false; res.shipIndex = -1; res.sunk = false; res.allSunk = false;

    if (!inBounds(r, c) || b.shot[r][c] != SHOT_NONE) return res;
    res.valid = true;

    int idx = b.shipAt[r][c];
    if (idx == -1) {                      // miss
        b.shot[r][c] = SHOT_MISS;
        return res;
    }

    res.hit = true;
    res.shipIndex = idx;
    b.shot[r][c] = SHOT_HIT;
    b.ships[idx].hits++;

    if (isSunk(b.ships[idx])) {           // every cell of that ship becomes "sunk"
        res.sunk = true;
        for (int rr = 0; rr < GRID; rr++)
            for (int cc = 0; cc < GRID; cc++)
                if (b.shipAt[rr][cc] == idx) b.shot[rr][cc] = SHOT_SUNK;
    }
    res.allSunk = allShipsSunk(b);
    return res;
}

int shipsAlive(const Board& b) {
    int n = 0;
    for (int i = 0; i < NUM_SHIPS; i++)
        if (b.ships[i].placed && !isSunk(b.ships[i])) n++;
    return n;
}

bool allShipsSunk(const Board& b) { return shipsAlive(b) == 0; }

int shotsForTurn(const Board& b, bool salvo) {
    return salvo ? shipsAlive(b) + 1 : 1;
}

// ------------------------------------------------------------
//  Computer opponent
//  The AI only looks at b.shot[][] and which ships are sunk -
//  exactly what a human could see. It never reads shipAt[][].
// ------------------------------------------------------------
static AIPick randomFrom(const std::vector<AIPick>& v) {
    if (v.empty()) { AIPick p = { 0, 0 }; return p; }
    return v[randInt((int)v.size())];
}

static AIPick pickEasy(const Board& e) {
    std::vector<AIPick> free;
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++)
            if (e.shot[r][c] == SHOT_NONE) { AIPick p = { r, c }; free.push_back(p); }
    return randomFrom(free);
}

// Medium: random "checkerboard" search, then follow up around hits
static AIPick pickMedium(const Board& e) {
    const int dr[4] = { -1, 1, 0, 0 };
    const int dc[4] = { 0, 0, -1, 1 };

    std::vector<AIPick> best;
    int bestScore = 0;
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++) {
            if (e.shot[r][c] != SHOT_HIT) continue;
            for (int d = 0; d < 4; d++) {
                int nr = r + dr[d], nc = c + dc[d];
                if (!inBounds(nr, nc) || e.shot[nr][nc] != SHOT_NONE) continue;
                int score = 1;
                int br = r - dr[d], bc = c - dc[d];               // cell behind the hit
                if (inBounds(br, bc) && e.shot[br][bc] == SHOT_HIT) score = 10;   // keep going along the line
                if (score > bestScore) { bestScore = score; best.clear(); }
                if (score == bestScore) { AIPick p = { nr, nc }; best.push_back(p); }
            }
        }
    if (!best.empty()) return randomFrom(best);

    std::vector<AIPick> parity;
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++)
            if (e.shot[r][c] == SHOT_NONE && (r + c) % 2 == 0) { AIPick p = { r, c }; parity.push_back(p); }
    if (!parity.empty()) return randomFrom(parity);
    return pickEasy(e);
}

// Hard: probability map. For every ship still afloat, count every place it could
// still fit. Cells that appear in the most placements are the best shots.
// If there are unsunk hits, only placements that cover a hit are counted.
static AIPick pickHard(const Board& e) {
    int score[GRID][GRID];
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++) score[r][c] = 0;

    bool haveHits = false;
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++)
            if (e.shot[r][c] == SHOT_HIT) haveHits = true;

    for (int s = 0; s < NUM_SHIPS; s++) {
        if (isSunk(e.ships[s])) continue;
        int size = e.ships[s].size;
        for (int dir = 0; dir < 2; dir++) {
            bool horiz = (dir == 0);
            for (int r = 0; r < GRID; r++)
                for (int c = 0; c < GRID; c++) {
                    bool ok = true;
                    int covered = 0;
                    for (int i = 0; i < size && ok; i++) {
                        int rr = horiz ? r : r + i;
                        int cc = horiz ? c + i : c;
                        if (!inBounds(rr, cc)) { ok = false; break; }
                        int st = e.shot[rr][cc];
                        if (st == SHOT_MISS || st == SHOT_SUNK) ok = false;
                        if (st == SHOT_HIT) covered++;
                    }
                    if (!ok) continue;
                    if (haveHits && covered == 0) continue;
                    int weight = 1 + covered * covered * 20;
                    for (int i = 0; i < size; i++) {
                        int rr = horiz ? r : r + i;
                        int cc = horiz ? c + i : c;
                        if (e.shot[rr][cc] == SHOT_NONE) score[rr][cc] += weight;
                    }
                }
        }
    }

    std::vector<AIPick> best;
    int bestScore = 0;
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++) {
            if (e.shot[r][c] != SHOT_NONE) continue;
            if (score[r][c] > bestScore) { bestScore = score[r][c]; best.clear(); }
            if (score[r][c] == bestScore && bestScore > 0) { AIPick p = { r, c }; best.push_back(p); }
        }
    if (!best.empty()) return randomFrom(best);
    return pickEasy(e);
}

AIPick aiChooseShot(const Board& enemy, Difficulty d) {
    if (d == DIFF_EASY)   return pickEasy(enemy);
    if (d == DIFF_MEDIUM) return pickMedium(enemy);
    return pickHard(enemy);
}
