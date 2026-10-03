// ============================================================
//  Game.cpp  -  screens, input, battle flow and drawing
//  Layout:  1) helpers  2) drawing  3) each screen  4) main dispatch
// ============================================================
#include "Game.h"
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

using namespace compat;

static const float PI = 3.14159265f;

// ------------------------------------------------------------
//  1) Small helpers
// ------------------------------------------------------------
static float frand() { return std::rand() / (float)RAND_MAX; }

static bool inRect(float px, float py, float x, float y, float w, float h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static std::string cellName(int r, int c) {           // row A-J, column 1-10  ->  "C4"
    return std::string(1, (char)('A' + r)) + std::to_string(c + 1);
}

static std::string upper(std::string s) {
    for (std::size_t i = 0; i < s.size(); i++) s[i] = (char)std::toupper((unsigned char)s[i]);
    return s;
}

static const char* diffName(Difficulty d) {
    return d == DIFF_EASY ? "EASY" : (d == DIFF_MEDIUM ? "MEDIUM" : "HARD");
}

struct Geom { float x0, y0, cs; };                      // where a board is drawn
static const Geom G_LEFT  = { 92.f,  190.f, 38.f };     // battle: enemy waters
static const Geom G_RIGHT = { 756.f, 190.f, 38.f };     // battle: your fleet
static const Geom G_PLACE = { 110.f, 172.f, 42.f };     // placement board

static bool cellAt(float px, float py, const Geom& g, int& r, int& c) {
    if (px < g.x0 || py < g.y0) return false;
    c = (int)((px - g.x0) / g.cs);
    r = (int)((py - g.y0) / g.cs);
    return r >= 0 && r < GRID && c >= 0 && c < GRID;
}

static float cellCX(const Geom& g, int c) { return g.x0 + c * g.cs + g.cs * 0.5f; }
static float cellCY(const Geom& g, int r) { return g.y0 + r * g.cs + g.cs * 0.5f; }

// a clickable rectangle with an id
struct Btn { int id; float x, y, w, h; std::string label; sf::Color color; bool enabled; bool selected; };

static Btn mkBtn(int id, float x, float y, float w, float h, const std::string& label, sf::Color color,
                 bool enabled = true, bool selected = false) {
    Btn b; b.id = id; b.x = x; b.y = y; b.w = w; b.h = h; b.label = label; b.color = color;
    b.enabled = enabled; b.selected = selected;
    return b;
}

static int hitBtn(const std::vector<Btn>& v, float px, float py) {
    for (std::size_t i = 0; i < v.size(); i++)
        if (v[i].enabled && inRect(px, py, v[i].x, v[i].y, v[i].w, v[i].h)) return v[i].id;
    return -1;
}

static void setHot(Game& g, int id) {
    if (id != g.hot && id != -1) g.audio.play(SFX_HOVER);
    g.hot = id;
}

static void logLine(Game& g, const std::string& text, sf::Color c) {
    LogLine l; l.text = text; l.color = c;
    g.log.push_back(l);
    if (g.log.size() > 6) g.log.erase(g.log.begin());
}

Game::Game() {
    win = 0; running = true; time = 0; fade = 1; mx = my = 0; hot = -1;
    screen = SC_MENU; menuSel = 0;
    mode = MODE_VS_AI; diff = DIFF_MEDIUM; salvo = true; focus = 0;
    placing = 0; cur = 0; viewer = 0; turnNo = 1; shotsLeft = shotsTotal = 0;
    selShip = 0; horiz = true; coverFor = 0; coverNext = COVER_TO_PLACE;
    phase = PH_AIM; phaseT = 0; tgtR = tgtC = 0; gameOver = false; winner = 0; confirmQuit = false; shake = 0;
    scoreSaved = false;
    for (int i = 0; i < 2; i++) {
        pl[i].isAI = false; pl[i].score = 0; pl[i].shotsFired = 0; pl[i].hits = 0;
        resetBoard(pl[i].board);
    }
}

void changeScreen(Game& g, Screen s) {
    g.screen = s;
    g.fade = 1.f;
    g.hot = -1;
    g.fx.clear();
    g.parts.clear();
}

static void spawnParticles(Game& g, float x, float y, int n, sf::Color c1, sf::Color c2, float speed) {
    for (int i = 0; i < n; i++) {
        Particle p;
        float a = frand() * 2 * PI;
        float sp = speed * (0.25f + frand());
        p.x = x; p.y = y; p.vx = std::cos(a) * sp; p.vy = std::sin(a) * sp - 20.f;
        p.maxLife = 0.5f + frand() * 0.7f; p.life = p.maxLife;
        p.size = 2.f + frand() * 3.f;
        p.color = mix(c1, c2, frand());
        g.parts.push_back(p);
    }
}

static void addEffect(Game& g, int type, float x, float y, float dur, bool big, sf::Color c, const std::string& text = "") {
    Effect e; e.type = type; e.x = x; e.y = y; e.t = 0; e.dur = dur; e.big = big; e.color = c; e.text = text;
    g.fx.push_back(e);
}

static void updateEffects(Game& g, float dt) {
    for (std::size_t i = 0; i < g.fx.size(); i++) g.fx[i].t += dt;
    for (std::size_t i = g.fx.size(); i > 0; i--)
        if (g.fx[i - 1].t >= g.fx[i - 1].dur) g.fx.erase(g.fx.begin() + (i - 1));
    for (std::size_t i = 0; i < g.parts.size(); i++) {
        Particle& p = g.parts[i];
        p.x += p.vx * dt; p.y += p.vy * dt; p.vy += 260.f * dt; p.life -= dt;
    }
    for (std::size_t i = g.parts.size(); i > 0; i--)
        if (g.parts[i - 1].life <= 0) g.parts.erase(g.parts.begin() + (i - 1));
    if (g.shake > 0) g.shake -= dt * 1.6f;
}

// ------------------------------------------------------------
//  2) Drawing helpers
// ------------------------------------------------------------
static void drawButton(sf::RenderTarget& t, Game& g, const Btn& b, unsigned fs = 20) {
    bool hot = (g.hot == b.id) && b.enabled;
    sf::Color c = b.enabled ? b.color : sf::Color(70, 90, 110);
    int fillA = b.selected ? 95 : (hot ? 70 : 26);
    sf::Color fill = withAlpha(c, fillA);
    if (hot || b.selected) drawGlowCutRect(t, b.x, b.y, b.w, b.h, 12, fill, c, 2.f, b.selected ? 1.f : 1.4f);
    else                   drawCutRect(t, b.x, b.y, b.w, b.h, 12, fill, withAlpha(c, 200), 2.f);
    sf::Color tc = !b.enabled ? sf::Color(90, 110, 130) : ((hot || b.selected) ? col::WHITE : col::INK);
    float cx = b.x + b.w * 0.5f, cy = b.y + b.h * 0.5f;
    if (hot || b.selected) drawGlowText(t, gFonts.bold, b.label, fs, cx, cy, tc, AL_CENTER, 2.f, 0.6f);
    else                   drawText(t, gFonts.bold, b.label, fs, cx, cy, tc, AL_CENTER, 2.f);
}

static void drawHeader(sf::RenderTarget& t, Game& g, const std::string& title, const std::string& sub = "") {
    drawGlowText(t, gFonts.title, title, 38, W / 2.f, 62, col::WHITE, AL_CENTER, 3.f, 1.0f);
    if (!sub.empty()) drawText(t, gFonts.body, sub, 24, W / 2.f, 102, col::CYAN, AL_CENTER, 2.f);
    (void)g;
}

// animated radar used behind the menu
static void drawRadar(sf::RenderTarget& t, float cx, float cy, float R, float time) {
    sf::Color c = col::CYAN;
    float sweep = std::fmod(time * 55.f, 360.f);
    const int SEG = 40;
    const float span = 80.f;

    sf::VertexArray fan(sf::PrimitiveType::TriangleFan);
    fan.append(sf::Vertex{ sf::Vector2f(cx, cy), withAlpha(c, 0) });
    for (int i = 0; i <= SEG; i++) {
        float k = i / (float)SEG;
        float ang = (sweep - span + span * k) * PI / 180.f;
        fan.append(sf::Vertex{ sf::Vector2f(cx + std::cos(ang) * R, cy + std::sin(ang) * R), withAlpha(c, (int)(k * k * 95)) });
    }
    t.draw(fan);

    drawCircle(t, cx, cy, R * 0.33f, 1.5f, withAlpha(c, 60));
    drawCircle(t, cx, cy, R * 0.66f, 1.5f, withAlpha(c, 60));
    drawCircle(t, cx, cy, R, 2.5f, withAlpha(c, 120));
    drawLine(t, cx - R, cy, cx + R, cy, 1.f, withAlpha(c, 40));
    drawLine(t, cx, cy - R, cx, cy + R, 1.f, withAlpha(c, 40));
    for (int a = 0; a < 360; a += 6) {
        float r1 = R, r2 = R - ((a % 30 == 0) ? 16.f : 8.f);
        float rad = a * PI / 180.f;
        drawLine(t, cx + std::cos(rad) * r1, cy + std::sin(rad) * r1, cx + std::cos(rad) * r2, cy + std::sin(rad) * r2, 1.5f, withAlpha(c, 110));
    }
    float lead = sweep * PI / 180.f;
    drawLine(t, cx, cy, cx + std::cos(lead) * R, cy + std::sin(lead) * R, 2.f, withAlpha(c, 190));

    // enemy contacts light up when the sweep passes over them
    static const float blipAng[5] = { 20.f, 95.f, 160.f, 235.f, 310.f };
    static const float blipRad[5] = { 0.55f, 0.78f, 0.42f, 0.7f, 0.5f };
    for (int i = 0; i < 5; i++) {
        float d = std::fmod(sweep - blipAng[i] + 720.f, 360.f);
        if (d > 150.f) continue;
        int a = (int)((1.f - d / 150.f) * 255.f);
        float bx = cx + std::cos(blipAng[i] * PI / 180.f) * R * blipRad[i];
        float by = cy + std::sin(blipAng[i] * PI / 180.f) * R * blipRad[i];
        drawRadialGlow(t, bx, by, 26.f, withAlpha(col::MAG, a / 2));
        std::vector<sf::Vector2f> dm;
        dm.push_back(sf::Vector2f(bx, by - 7)); dm.push_back(sf::Vector2f(bx + 7, by));
        dm.push_back(sf::Vector2f(bx, by + 7)); dm.push_back(sf::Vector2f(bx - 7, by));
        drawPoly(t, dm, withAlpha(col::MAG, a));
    }
}

// one ship, drawn as a single neon hull spanning its cells
static void drawShipShape(sf::RenderTarget& t, const Ship& s, const Geom& G, bool sunk, float alpha = 1.f) {
    float x = G.x0 + s.col * G.cs + 3, y = G.y0 + s.row * G.cs + 3;
    float w = (s.horizontal ? s.size * G.cs : G.cs) - 6;
    float h = (s.horizontal ? G.cs : s.size * G.cs) - 6;
    sf::Color fill    = sunk ? sf::Color(110, 12, 96, (int)(215 * alpha)) : sf::Color(0, 140, 180, (int)(215 * alpha));
    sf::Color outline = sunk ? col::MAG : col::CYAN;
    outline = withAlpha(outline, (int)(255 * alpha));
    if (alpha >= 0.99f) drawGlowCutRect(t, x, y, w, h, G.cs * 0.28f, fill, outline, 2.f, 0.8f);
    else                drawCutRect(t, x, y, w, h, G.cs * 0.28f, fill, outline, 2.f);
    for (int i = 1; i < s.size; i++) {                    // deck lines between cells
        sf::Color dl = withAlpha(sf::Color(0, 0, 0), (int)(70 * alpha));
        if (s.horizontal) drawLine(t, x + i * G.cs - 3, y + 5, x + i * G.cs - 3, y + h - 5, 1.f, dl);
        else              drawLine(t, x + 5, y + i * G.cs - 3, x + w - 5, y + i * G.cs - 3, 1.f, dl);
    }
}

static void drawX(sf::RenderTarget& t, float cx, float cy, float r, sf::Color c) {
    drawLine(t, cx - r, cy - r, cx + r, cy + r, 3.f, c);
    drawLine(t, cx - r, cy + r, cx + r, cy - r, 3.f, c);
}

static void drawReticle(sf::RenderTarget& t, float cx, float cy, float cs, float time, sf::Color c) {
    float pulse = 2.f * std::sin(time * 9.f);
    float h = cs * 0.5f + 2 + pulse, l = cs * 0.28f;
    fillRect(t, cx - cs * 0.5f + 1, cy - cs * 0.5f + 1, cs - 2, cs - 2, withAlpha(c, 40));
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2) {
            drawLine(t, cx + sx * h, cy + sy * h, cx + sx * (h - l), cy + sy * h, 3.f, c);
            drawLine(t, cx + sx * h, cy + sy * h, cx + sx * h, cy + sy * (h - l), 3.f, c);
        }
    drawLine(t, cx - 5, cy, cx + 5, cy, 2.f, c);
    drawLine(t, cx, cy - 5, cx, cy + 5, 2.f, c);
}

// a whole board: water, ships, shots. showShips=false hides the ships (enemy waters).
static void drawBoard(sf::RenderTarget& t, Game& g, const Board& b, const Geom& G, bool showShips,
                      sf::Color accent, bool reticle, int hr, int hc) {
    float size = G.cs * GRID;
    drawGlowCutRect(t, G.x0 - 34, G.y0 - 34, size + 40, size + 40, 16, withAlpha(col::PANEL, 240), accent, 2.f, 0.9f);

    for (int i = 0; i < GRID; i++) {
        drawText(t, gFonts.mono, std::to_string(i + 1), 15, G.x0 + i * G.cs + G.cs / 2, G.y0 - 16, withAlpha(accent, 200), AL_CENTER);
        drawText(t, gFonts.mono, std::string(1, (char)('A' + i)), 15, G.x0 - 17, G.y0 + i * G.cs + G.cs / 2, withAlpha(accent, 200), AL_CENTER);
    }

    // water
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++) {
            float v = 6.f * std::sin(g.time * 1.6f + r * 0.5f + c * 0.8f);
            sf::Color w((std::uint8_t)(10 + v), (std::uint8_t)(42 + v * 2), (std::uint8_t)(85 + v * 2));
            fillRect(t, G.x0 + c * G.cs + 1, G.y0 + r * G.cs + 1, G.cs - 2, G.cs - 2, w);
        }

    // ships
    for (int i = 0; i < NUM_SHIPS; i++) {
        const Ship& s = b.ships[i];
        if (!s.placed) continue;
        bool sunk = isSunk(s);
        if (showShips || sunk) drawShipShape(t, s, G, sunk);
    }

    // shots
    for (int r = 0; r < GRID; r++)
        for (int c = 0; c < GRID; c++) {
            int st = b.shot[r][c];
            float cx = cellCX(G, c), cy = cellCY(G, r);
            if (st == SHOT_MISS) {
                drawCircle(t, cx, cy, G.cs * 0.22f, 1.5f, withAlpha(col::DIM, 90));
                drawCircle(t, cx, cy, G.cs * 0.09f, 0.f, col::DIM, withAlpha(col::DIM, 220));
            } else if (st == SHOT_HIT) {
                fillRect(t, cx - G.cs * 0.5f + 3, cy - G.cs * 0.5f + 3, G.cs - 6, G.cs - 6, withAlpha(col::HIT, 235));
                drawRadialGlow(t, cx, cy, G.cs * 0.9f, withAlpha(col::HIT, 60));
                drawX(t, cx, cy, G.cs * 0.24f, col::WHITE);
            } else if (st == SHOT_SUNK) {
                drawX(t, cx, cy, G.cs * 0.24f, sf::Color(255, 255, 255, 230));
            }
        }

    if (reticle && hr >= 0 && b.shot[hr][hc] == SHOT_NONE)
        drawReticle(t, cellCX(G, hc), cellCY(G, hr), G.cs, g.time, col::MAG);
}

static void drawEffects(sf::RenderTarget& t, Game& g) {
    for (std::size_t i = 0; i < g.fx.size(); i++) {
        const Effect& e = g.fx[i];
        float p = e.t / e.dur;
        if (e.type == FX_RIPPLE) {
            for (int k = 0; k < 2; k++) {
                float pp = p - k * 0.2f;
                if (pp < 0) continue;
                drawCircle(t, e.x, e.y, 5.f + pp * 34.f, 2.f, withAlpha(col::CYAN, (int)((1.f - pp) * 220)));
            }
        } else if (e.type == FX_BLAST) {
            float R = e.big ? 100.f : 50.f;
            if (p < 0.35f) drawRadialGlow(t, e.x, e.y, 26.f + p * R, withAlpha(sf::Color(255, 190, 90), (int)((1.f - p / 0.35f) * 230)));
            drawCircle(t, e.x, e.y, 8.f + p * R, 3.f, withAlpha(e.color, (int)((1.f - p) * 240)));
            if (e.big) drawCircle(t, e.x, e.y, 4.f + p * R * 1.4f, 2.f, withAlpha(col::WHITE, (int)((1.f - p) * 160)));
        } else if (e.type == FX_POPUP) {
            float a = p < 0.7f ? 1.f : (1.f - p) / 0.3f;
            drawGlowText(t, gFonts.bold, e.text, e.big ? 26 : 20, e.x, e.y - p * 38.f, withAlpha(e.color, (int)(a * 255)), AL_CENTER, 2.f, 0.6f);
        }
    }
    for (std::size_t i = 0; i < g.parts.size(); i++) {
        const Particle& p = g.parts[i];
        float a = p.life / p.maxLife;
        fillRect(t, p.x - p.size / 2, p.y - p.size / 2, p.size, p.size, withAlpha(p.color, (int)(a * 255)));
    }
}

// ------------------------------------------------------------
//  3) Battle logic (turn flow) - kept before the screens that use it
// ------------------------------------------------------------
static void startTurn(Game& g) {
    Player& A = g.pl[g.cur];
    g.viewer = (g.mode == MODE_VS_AI) ? 0 : g.cur;
    g.shotsTotal = shotsForTurn(A.board, g.salvo);
    g.shotsLeft = g.shotsTotal;
    g.phase = A.isAI ? PH_AI_THINK : PH_AIM;
    g.phaseT = 0;
    logLine(g, "> " + A.name + " TAKES " + std::to_string(g.shotsTotal) + (g.shotsTotal == 1 ? " SHOT" : " SHOTS"), col::CYAN);
}

static void resetBattleState(Game& g) {
    g.cur = 0; g.viewer = 0; g.turnNo = 1; g.gameOver = false; g.winner = 0;
    g.confirmQuit = false; g.shake = 0; g.log.clear();
}

static void enterBattle(Game& g) {          // start the first turn
    changeScreen(g, SC_BATTLE);
    startTurn(g);
}

static void enterResult(Game& g) {
    Player& w = g.pl[g.winner];
    g.scoreSaved = false;
    if (!w.isAI) {
        std::string tag = (g.mode == MODE_VS_AI) ? std::string("VS AI ") + diffName(g.diff) : std::string("PVP");
        saveScore(w.name, w.score, tag);
        g.scoreSaved = true;
    }
    g.audio.play(w.isAI ? SFX_LOSE : SFX_WIN);
    changeScreen(g, SC_RESULT);
}

static void resolveShot(Game& g) {
    Player& A = g.pl[g.cur];
    Player& D = g.pl[1 - g.cur];
    ShotResult r = fireAt(D.board, g.tgtR, g.tgtC);
    if (!r.valid) { g.phase = PH_AFTER_SHOT; g.phaseT = 0; return; }

    A.shotsFired++;
    const Geom& G = (g.cur == g.viewer) ? G_LEFT : G_RIGHT;
    float x = cellCX(G, g.tgtC), y = cellCY(G, g.tgtR);
    std::string where = cellName(g.tgtR, g.tgtC);

    if (r.hit) {
        A.hits++;
        A.score += 10;
        if (!g.salvo) { g.shotsTotal++; g.shotsLeft++; }   // classic: a hit earns another shot
        if (r.sunk) {
            std::string nm = upper(D.board.ships[r.shipIndex].name);
            addEffect(g, FX_BLAST, x, y, 0.9f, true, col::MAG);
            addEffect(g, FX_POPUP, x, y - 10, 1.3f, true, col::MAG, "SUNK!");
            spawnParticles(g, x, y, 60, sf::Color(255, 200, 80), col::MAG, 260.f);
            g.shake = 0.6f;
            g.audio.play(SFX_SUNK);
            logLine(g, "> " + A.name + " " + where + ": SUNK " + nm + "!", col::MAG);
        } else {
            addEffect(g, FX_BLAST, x, y, 0.7f, false, col::HIT);
            addEffect(g, FX_POPUP, x, y - 10, 1.0f, false, col::HIT, "HIT");
            spawnParticles(g, x, y, 24, sf::Color(255, 190, 90), col::HIT, 170.f);
            g.audio.play(SFX_HIT);
            logLine(g, "> " + A.name + " " + where + ": HIT", col::HIT);
        }
    } else {
        A.score -= 1;
        addEffect(g, FX_RIPPLE, x, y, 0.9f, false, col::CYAN);
        addEffect(g, FX_POPUP, x, y - 10, 0.9f, false, col::DIM, "MISS");
        g.audio.play(SFX_SPLASH);
        logLine(g, "> " + A.name + " " + where + ": MISS", col::DIM);
    }

    g.shotsLeft--;
    if (r.allSunk) {
        g.gameOver = true;
        g.winner = g.cur;
        g.phase = PH_TURN_END;
        g.phaseT = 0;
        return;
    }
    g.phase = PH_AFTER_SHOT;
    g.phaseT = 0;
}

static void endTurn(Game& g) {
    g.cur = 1 - g.cur;
    g.turnNo++;
    if (g.mode == MODE_VS_PLAYER) {                    // hide the screen between players
        g.coverFor = g.cur;
        g.coverNext = COVER_TO_TURN;
        changeScreen(g, SC_COVER);
    } else {
        startTurn(g);
    }
}

static void battleUpdate(Game& g, float dt) {
    if (g.confirmQuit) return;
    g.phaseT += dt;
    Player& A = g.pl[g.cur];
    Player& D = g.pl[1 - g.cur];
    switch (g.phase) {
    case PH_AIM:
        break;
    case PH_AI_THINK:
        if (g.phaseT > 0.7f) {
            AIPick p = aiChooseShot(D.board, g.diff);
            g.tgtR = p.r; g.tgtC = p.c;
            g.phase = PH_FLIGHT; g.phaseT = 0;
            g.audio.play(SFX_LAUNCH);
        }
        break;
    case PH_FLIGHT:
        if (g.phaseT >= 0.38f) resolveShot(g);
        break;
    case PH_AFTER_SHOT:
        if (g.phaseT > (A.isAI ? 0.75f : 0.5f)) {
            if (g.shotsLeft > 0) {
                g.phase = A.isAI ? PH_AI_THINK : PH_AIM;
                g.phaseT = 0.35f;
            } else {
                g.phase = PH_TURN_END;
                g.phaseT = 0;
            }
        }
        break;
    case PH_TURN_END:
        if (g.gameOver) { if (g.phaseT > 1.8f) enterResult(g); }
        else if (g.phaseT > 1.0f) endTurn(g);
        break;
    }
}

// ------------------------------------------------------------
//  3b) New game (session) setup
// ------------------------------------------------------------
static std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(' ');
    if (a == std::string::npos) return "";
    std::size_t b = s.find_last_not_of(' ');
    return s.substr(a, b - a + 1);
}

void startSession(Game& g) {
    std::string n1 = upper(trim(g.names[0])); if (n1.empty()) n1 = "PLAYER 1";
    std::string n2 = upper(trim(g.names[1])); if (n2.empty()) n2 = "PLAYER 2";
    g.pl[0].name = n1;
    g.pl[0].isAI = false;
    if (g.mode == MODE_VS_AI) { g.pl[1].name = "COMPUTER"; g.pl[1].isAI = true; }
    else                      { g.pl[1].name = n2;         g.pl[1].isAI = false; }
    for (int i = 0; i < 2; i++) {
        resetBoard(g.pl[i].board);
        g.pl[i].score = 0; g.pl[i].shotsFired = 0; g.pl[i].hits = 0;
    }
    if (g.pl[1].isAI) randomizeFleet(g.pl[1].board);
    g.placing = 0; g.selShip = 0; g.horiz = true;
    resetBattleState(g);
    g.scoreSaved = false;
    changeScreen(g, SC_PLACE);
}

// ------------------------------------------------------------
//  4) Screens
// ------------------------------------------------------------

// ===================== MENU =====================
static std::vector<Btn> menuButtons(const Game& g) {
    static const char* labels[4] = { "NEW GAME", "INSTRUCTIONS", "LEADERBOARD", "EXIT" };
    std::vector<Btn> v;
    for (int i = 0; i < 4; i++)
        v.push_back(mkBtn(i, (W - 360) / 2.f, 330.f + i * 58.f, 360.f, 48.f, labels[i],
                          (i % 2 == 0) ? col::CYAN : col::MAG, true, g.menuSel == i));
    return v;
}

static void menuActivate(Game& g, int i) {
    g.audio.play(SFX_CLICK);
    if (i == 0)      changeScreen(g, SC_SETUP);
    else if (i == 1) changeScreen(g, SC_HELP);
    else if (i == 2) { g.scores = loadScores(); changeScreen(g, SC_SCORES); }
    else             g.running = false;
}

static void menuEvent(Game& g, const Ev& e) {
    std::vector<Btn> b = menuButtons(g);
    if (e.type == EV_MOVE) {
        int id = hitBtn(b, (float)e.x, (float)e.y);
        setHot(g, id);
        if (id >= 0) g.menuSel = id;
    } else if (e.type == EV_PRESS && e.button == 1) {
        int id = hitBtn(b, (float)e.x, (float)e.y);
        if (id >= 0) menuActivate(g, id);
    } else if (e.type == EV_KEY) {
        if (e.key == K_UP)         { g.menuSel = (g.menuSel + 3) % 4; g.audio.play(SFX_HOVER); }
        else if (e.key == K_DOWN)  { g.menuSel = (g.menuSel + 1) % 4; g.audio.play(SFX_HOVER); }
        else if (e.key == K_ENTER) menuActivate(g, g.menuSel);
    }
}

static void menuDraw(sf::RenderTarget& t, Game& g) {
    drawRadar(t, W / 2.f, 250.f, 250.f, g.time);
    drawGlowText(t, gFonts.title, "BATTLE", 84, W / 2.f, 112, col::WHITE, AL_CENTER, 2.f, 1.4f);
    drawOutlineText(t, gFonts.title, "SHIP", 118, W / 2.f, 208, col::MAG, 3.f, AL_CENTER, 4.f);

    float pw = 430, ph = 34, px = (W - pw) / 2.f, py = 255;
    drawGlowCutRect(t, px, py, pw, ph, 10, withAlpha(col::CYAN, 70), col::CYAN, 2.f, 1.f);
    drawText(t, gFonts.bold, "STRATEGIC NAVAL WARFARE", 15, W / 2.f, py + ph / 2, col::WHITE, AL_CENTER, 4.f);

    std::vector<Btn> b = menuButtons(g);
    for (std::size_t i = 0; i < b.size(); i++) drawButton(t, g, b[i]);

    drawText(t, gFonts.mono, "AILA NOOR  //  RUBAMA SHAHZAD  //  DEGREE 47  //  SEMPEC", 14, W / 2.f, 632, col::DIM, AL_CENTER, 2.f);
}

// ===================== SETUP =====================
static std::vector<Btn> setupButtons(const Game& g) {
    static const char* dl[3] = { "EASY", "MEDIUM", "HARD" };
    bool ai = (g.mode == MODE_VS_AI);
    std::vector<Btn> v;
    v.push_back(mkBtn(1, 430, 140, 190, 44, "VS COMPUTER", col::CYAN, true, ai));
    v.push_back(mkBtn(2, 640, 140, 190, 44, "VS PLAYER", col::MAG, true, !ai));
    for (int d = 0; d < 3; d++)
        v.push_back(mkBtn(10 + d, 430.f + d * 155.f, 208, 140, 44, dl[d], col::CYAN, ai, ai && g.diff == d));
    v.push_back(mkBtn(20, 430, 276, 190, 44, "SALVO", col::CYAN, true, g.salvo));
    v.push_back(mkBtn(21, 640, 276, 190, 44, "CLASSIC", col::MAG, true, !g.salvo));
    v.push_back(mkBtn(30, 230, 520, 180, 48, "BACK", col::MAG));
    v.push_back(mkBtn(31, 790, 520, 180, 48, "START GAME", col::CYAN));
    return v;
}

static void setupEvent(Game& g, const Ev& e) {
    std::vector<Btn> b = setupButtons(g);
    if (e.type == EV_MOVE) {
        setHot(g, hitBtn(b, (float)e.x, (float)e.y));
    } else if (e.type == EV_PRESS && e.button == 1) {
        float px = (float)e.x, py = (float)e.y;
        if (inRect(px, py, 430, 378, 400, 44)) { g.focus = 0; g.audio.play(SFX_CLICK); return; }
        if (g.mode == MODE_VS_PLAYER && inRect(px, py, 430, 440, 400, 44)) { g.focus = 1; g.audio.play(SFX_CLICK); return; }
        int id = hitBtn(b, px, py);
        if (id < 0) return;
        g.audio.play(SFX_CLICK);
        if (id == 1) g.mode = MODE_VS_AI;
        else if (id == 2) g.mode = MODE_VS_PLAYER;
        else if (id >= 10 && id <= 12) g.diff = (Difficulty)(id - 10);
        else if (id == 20) g.salvo = true;
        else if (id == 21) g.salvo = false;
        else if (id == 30) changeScreen(g, SC_MENU);
        else if (id == 31) startSession(g);
    } else if (e.type == EV_TEXT) {
        if (e.text >= 32 && e.text <= 126 && e.text != '|') {
            if (g.focus == 1 && g.mode == MODE_VS_AI) return;
            std::string& s = g.names[g.focus];
            if (s.size() < 12) s.push_back((char)e.text);
        }
    } else if (e.type == EV_KEY) {
        if (e.key == K_BACK) { std::string& s = g.names[g.focus]; if (!s.empty()) s.pop_back(); }
        else if (e.key == K_TAB && g.mode == MODE_VS_PLAYER) g.focus = 1 - g.focus;
        else if (e.key == K_ENTER) startSession(g);
        else if (e.key == K_ESC) changeScreen(g, SC_MENU);
    }
}

static void drawField(sf::RenderTarget& t, Game& g, float x, float y, float w, float h, const std::string& text,
                      const std::string& placeholder, bool focused, sf::Color c, bool enabled) {
    if (focused) drawGlowCutRect(t, x, y, w, h, 12, withAlpha(c, 30), c, 2.f, 1.f);
    else         drawCutRect(t, x, y, w, h, 12, withAlpha(col::PANEL, 230), withAlpha(c, enabled ? 150 : 60), 2.f);
    if (text.empty()) drawText(t, gFonts.body, placeholder, 24, x + 18, y + h / 2, withAlpha(col::DIM, enabled ? 150 : 80), AL_LEFT, 1.f);
    else              drawText(t, gFonts.bold, text, 20, x + 18, y + h / 2, col::WHITE, AL_LEFT, 1.f);
    if (focused && std::fmod(g.time, 1.f) < 0.5f) {
        float cx = x + 18 + (text.empty() ? 0.f : textWidth(gFonts.bold, text, 20, 1.f)) + 3;
        fillRect(t, cx, y + 10, 2, h - 20, col::CYAN);
    }
}

static void setupDraw(sf::RenderTarget& t, Game& g) {
    drawGlowText(t, gFonts.title, "NEW GAME", 38, W / 2.f, 62, col::WHITE, AL_CENTER, 3.f, 1.f);
    drawGlowCutRect(t, 190, 100, 820, 492, 22, withAlpha(col::PANEL, 235), col::CYAN, 2.f, 0.7f);

    drawText(t, gFonts.bold, "MODE", 16, 232, 162, col::CYAN, AL_LEFT, 3.f);
    drawText(t, gFonts.bold, "DIFFICULTY", 16, 232, 230, g.mode == MODE_VS_AI ? col::CYAN : sf::Color(80, 100, 120), AL_LEFT, 3.f);
    drawText(t, gFonts.bold, "RULES", 16, 232, 298, col::CYAN, AL_LEFT, 3.f);
    drawText(t, gFonts.body, g.salvo ? "SALVO: fire (ships alive + 1) shots per turn."
                                     : "CLASSIC: one shot per turn, but a hit earns another shot.", 21, 430, 350, col::DIM, AL_LEFT, 1.f);
    drawText(t, gFonts.bold, "PLAYER 1", 16, 232, 400, col::MAG, AL_LEFT, 3.f);
    drawText(t, gFonts.bold, "PLAYER 2", 16, 232, 462, col::MAG, AL_LEFT, 3.f);

    drawField(t, g, 430, 378, 400, 44, g.names[0], "TYPE YOUR NAME", g.focus == 0, col::CYAN, true);
    if (g.mode == MODE_VS_PLAYER) drawField(t, g, 430, 440, 400, 44, g.names[1], "TYPE YOUR NAME", g.focus == 1, col::MAG, true);
    else                          drawField(t, g, 430, 440, 400, 44, "COMPUTER", "", false, col::MAG, false);

    std::vector<Btn> b = setupButtons(g);
    for (std::size_t i = 0; i < b.size(); i++) drawButton(t, g, b[i], 18);
}

// ===================== PLACEMENT =====================
static int nextUnplaced(const Board& b) {
    for (int i = 0; i < NUM_SHIPS; i++) if (!b.ships[i].placed) return i;
    return -1;
}

static std::vector<Btn> placeButtons(const Game& g) {
    const Board& b = g.pl[g.placing].board;
    std::vector<Btn> v;
    for (int i = 0; i < NUM_SHIPS; i++)
        v.push_back(mkBtn(10 + i, 640, 160.f + i * 52.f, 480, 46, b.ships[i].name, col::CYAN, true, g.selShip == i));
    v.push_back(mkBtn(1, 645, 470, 150, 44, "ROTATE", col::CYAN));
    v.push_back(mkBtn(2, 810, 470, 150, 44, "RANDOM", col::MAG));
    v.push_back(mkBtn(3, 975, 470, 150, 44, "RESET", col::CYAN));
    v.push_back(mkBtn(4, 645, 524, 480, 52, "READY FOR BATTLE", col::LIME, fleetReady(b)));
    v.push_back(mkBtn(5, 90, 612, 130, 40, "BACK", col::MAG));
    return v;
}

static void placeAt(Game& g, int r, int c) {
    Board& b = g.pl[g.placing].board;
    if (g.selShip >= 0) {
        int size = b.ships[g.selShip].size;
        int rr = r, cc = c;
        if (g.horiz) cc = std::min(cc, GRID - size); else rr = std::min(rr, GRID - size);
        if (placeShip(b, g.selShip, rr, cc, g.horiz)) {
            g.audio.play(SFX_PLACE);
            g.selShip = nextUnplaced(b);
            return;
        }
    }
    int idx = b.shipAt[r][c];
    if (idx != -1) {                                   // pick the ship up again
        g.horiz = b.ships[idx].horizontal;
        removeShip(b, idx);
        g.selShip = idx;
        g.audio.play(SFX_CLICK);
        return;
    }
    g.audio.play(SFX_ERROR);
}

static void finishPlacement(Game& g) {
    g.audio.play(SFX_CLICK);
    if (g.mode == MODE_VS_AI) { resetBattleState(g); enterBattle(g); return; }
    if (g.placing == 0) {
        g.placing = 1; g.selShip = 0; g.horiz = true;
        g.coverFor = 1; g.coverNext = COVER_TO_PLACE;
        changeScreen(g, SC_COVER);
    } else {
        resetBattleState(g);
        g.coverFor = 0; g.coverNext = COVER_TO_TURN;
        changeScreen(g, SC_COVER);
    }
}

static void placeEvent(Game& g, const Ev& e) {
    Board& b = g.pl[g.placing].board;
    std::vector<Btn> bt = placeButtons(g);
    if (e.type == EV_MOVE) {
        setHot(g, hitBtn(bt, (float)e.x, (float)e.y));
    } else if (e.type == EV_PRESS) {
        if (e.button == 2) { g.horiz = !g.horiz; g.audio.play(SFX_CLICK); return; }
        int r, c;
        if (cellAt((float)e.x, (float)e.y, G_PLACE, r, c)) { placeAt(g, r, c); return; }
        int id = hitBtn(bt, (float)e.x, (float)e.y);
        if (id >= 10) {
            int i = id - 10;
            if (b.ships[i].placed) { g.horiz = b.ships[i].horizontal; removeShip(b, i); }
            g.selShip = i; g.audio.play(SFX_CLICK);
        } else if (id == 1) { g.horiz = !g.horiz; g.audio.play(SFX_CLICK); }
        else if (id == 2) { randomizeFleet(b); g.selShip = -1; g.audio.play(SFX_PLACE); }
        else if (id == 3) { resetBoard(b); g.selShip = 0; g.audio.play(SFX_CLICK); }
        else if (id == 4) { if (fleetReady(b)) finishPlacement(g); }
        else if (id == 5) { g.audio.play(SFX_CLICK); changeScreen(g, SC_SETUP); }
    } else if (e.type == EV_KEY) {
        if (e.key == 'R') { g.horiz = !g.horiz; g.audio.play(SFX_CLICK); }
        else if (e.key == K_ENTER && fleetReady(b)) finishPlacement(g);
        else if (e.key == K_ESC) changeScreen(g, SC_SETUP);
    }
}

static void placeDraw(sf::RenderTarget& t, Game& g) {
    Board& b = g.pl[g.placing].board;
    drawHeader(t, g, "DEPLOY YOUR FLEET", g.pl[g.placing].name + "  -  PLACE ALL 5 SHIPS");

    int hr = -1, hc = -1;
    bool over = cellAt((float)g.mx, (float)g.my, G_PLACE, hr, hc);
    drawBoard(t, g, b, G_PLACE, true, col::CYAN, false, -1, -1);

    if (over && g.selShip >= 0) {                      // ghost preview
        int size = b.ships[g.selShip].size;
        int rr = hr, cc = hc;
        if (g.horiz) cc = std::min(cc, GRID - size); else rr = std::min(rr, GRID - size);
        bool ok = canPlace(b, g.selShip, rr, cc, g.horiz);
        sf::Color gc = ok ? col::CYAN : col::HIT;
        for (int i = 0; i < size; i++) {
            int r2 = g.horiz ? rr : rr + i, c2 = g.horiz ? cc + i : cc;
            fillRect(t, G_PLACE.x0 + c2 * G_PLACE.cs + 2, G_PLACE.y0 + r2 * G_PLACE.cs + 2, G_PLACE.cs - 4, G_PLACE.cs - 4, withAlpha(gc, 110));
            strokeRect(t, G_PLACE.x0 + c2 * G_PLACE.cs + 2, G_PLACE.y0 + r2 * G_PLACE.cs + 2, G_PLACE.cs - 4, G_PLACE.cs - 4, 2, gc);
        }
    }

    drawGlowCutRect(t, 620, 140, 520, 450, 18, withAlpha(col::PANEL, 235), col::MAG, 2.f, 0.7f);
    std::vector<Btn> bt = placeButtons(g);
    for (std::size_t k = 0; k < bt.size(); k++) {
        const Btn& bb = bt[k];
        if (bb.id >= 10) {                             // fleet rows
            int i = bb.id - 10;
            const Ship& s = b.ships[i];
            bool hot = g.hot == bb.id;
            if (bb.selected || hot) drawGlowCutRect(t, bb.x, bb.y, bb.w, bb.h, 10, withAlpha(col::CYAN, bb.selected ? 60 : 30), col::CYAN, 2.f, 0.8f);
            else                    drawCutRect(t, bb.x, bb.y, bb.w, bb.h, 10, withAlpha(col::CYAN, 14), withAlpha(col::CYAN, 110), 1.5f);
            for (int q = 0; q < s.size; q++)
                fillRect(t, bb.x + 20 + q * 18, bb.y + bb.h / 2 - 8, 15, 16, s.placed ? col::SHIP : withAlpha(col::CYAN, 120));
            drawText(t, gFonts.bold, upper(s.name), 16, bb.x + 128, bb.y + bb.h / 2, col::INK, AL_LEFT, 1.f);
            drawText(t, gFonts.mono, s.placed ? "PLACED" : "TO PLACE", 15, bb.x + bb.w - 16, bb.y + bb.h / 2,
                     s.placed ? col::LIME : col::MAG, AL_RIGHT, 1.f);
        } else {
            drawButton(t, g, bb, 16);
        }
    }
    drawText(t, gFonts.mono, "CLICK THE GRID TO PLACE  //  R OR RIGHT-CLICK ROTATES", 14, 880, 450, col::DIM, AL_CENTER, 1.f);
    drawText(t, gFonts.mono, g.horiz ? "DIRECTION: HORIZONTAL" : "DIRECTION: VERTICAL", 14, 880, 430, col::CYAN, AL_CENTER, 1.f);
}

// ===================== COVER (pass the device) =====================
static std::vector<Btn> coverButtons() {
    std::vector<Btn> v;
    v.push_back(mkBtn(1, 440, 450, 320, 56, "I AM READY", col::CYAN));
    return v;
}

static void coverGo(Game& g) {
    g.audio.play(SFX_CLICK);
    if (g.coverNext == COVER_TO_PLACE) changeScreen(g, SC_PLACE);
    else enterBattle(g);
}

static void coverEvent(Game& g, const Ev& e) {
    std::vector<Btn> b = coverButtons();
    if (e.type == EV_MOVE) setHot(g, hitBtn(b, (float)e.x, (float)e.y));
    else if (e.type == EV_PRESS && e.button == 1) { if (hitBtn(b, (float)e.x, (float)e.y) == 1) coverGo(g); }
    else if (e.type == EV_KEY && (e.key == K_ENTER || e.key == K_SPACE)) coverGo(g);
}

static void coverDraw(sf::RenderTarget& t, Game& g) {
    drawGlowText(t, gFonts.title, "PASS THE DEVICE", 54, W / 2.f, 170, col::MAG, AL_CENTER, 3.f, 1.2f);
    drawText(t, gFonts.body, "NEXT COMMANDER", 28, W / 2.f, 250, col::DIM, AL_CENTER, 4.f);
    drawGlowText(t, gFonts.title, g.pl[g.coverFor].name, 64, W / 2.f, 325, col::CYAN, AL_CENTER, 3.f, 1.3f);
    drawText(t, gFonts.body, "The other player must look away before you continue.", 24, W / 2.f, 395, col::INK, AL_CENTER, 1.f);
    std::vector<Btn> b = coverButtons();
    for (std::size_t i = 0; i < b.size(); i++) drawButton(t, g, b[i], 22);
}

// ===================== BATTLE =====================
static void battleEvent(Game& g, const Ev& e) {
    if (g.confirmQuit) {
        std::vector<Btn> b;
        b.push_back(mkBtn(1, 430, 350, 150, 48, "YES", col::MAG));
        b.push_back(mkBtn(2, 620, 350, 150, 48, "NO", col::CYAN));
        if (e.type == EV_MOVE) setHot(g, hitBtn(b, (float)e.x, (float)e.y));
        else if (e.type == EV_PRESS && e.button == 1) {
            int id = hitBtn(b, (float)e.x, (float)e.y);
            if (id == 1) changeScreen(g, SC_MENU);
            else if (id == 2) g.confirmQuit = false;
        } else if (e.type == EV_KEY) {
            if (e.key == 'Y') changeScreen(g, SC_MENU);
            else if (e.key == 'N' || e.key == K_ESC) g.confirmQuit = false;
        }
        return;
    }
    if (e.type == EV_KEY && e.key == K_ESC) { g.confirmQuit = true; return; }
    if (e.type == EV_PRESS && e.button == 1 && g.phase == PH_AIM && !g.gameOver) {
        if (g.pl[g.cur].isAI) return;
        int r, c;
        if (cellAt((float)e.x, (float)e.y, G_LEFT, r, c)) {
            Board& db = g.pl[1 - g.cur].board;
            if (db.shot[r][c] != SHOT_NONE) {
                g.audio.play(SFX_ERROR);
                addEffect(g, FX_POPUP, cellCX(G_LEFT, c), cellCY(G_LEFT, r), 0.9f, false, col::LIME, "USED");
            } else {
                g.tgtR = r; g.tgtC = c;
                g.phase = PH_FLIGHT; g.phaseT = 0;
                g.audio.play(SFX_LAUNCH);
            }
        }
    }
}

static void battleDraw(sf::RenderTarget& t, Game& g) {
    Player& V = g.pl[g.viewer];
    Player& O = g.pl[1 - g.viewer];
    Player& A = g.pl[g.cur];
    bool humanAim = (g.phase == PH_AIM) && !g.confirmQuit && !A.isAI && !g.gameOver;

    // top bar
    drawGlowText(t, gFonts.bold, "BATTLESHIP", 20, 150, 52, col::CYAN, AL_CENTER, 3.f, 0.8f);
    std::string modeTxt = (g.mode == MODE_VS_AI) ? std::string("VS COMPUTER  //  ") + diffName(g.diff) : std::string("PLAYER VS PLAYER");
    modeTxt += g.salvo ? "  //  SALVO" : "  //  CLASSIC";
    drawText(t, gFonts.mono, modeTxt, 14, W - 62, 52, col::DIM, AL_RIGHT, 1.f);

    // turn banner
    bool mine = (g.cur == g.viewer);
    sf::Color bc = mine ? col::CYAN : col::MAG;
    std::string bt;
    if (g.gameOver) bt = "FLEET DESTROYED";
    else {
        int idx = std::min(g.shotsTotal, g.shotsTotal - g.shotsLeft + 1);
        std::string who = mine ? (g.mode == MODE_VS_AI ? std::string("YOUR TURN") : A.name + "'S TURN") : (A.name + " IS FIRING");
        bt = who + "  //  SHOT " + std::to_string(idx) + " OF " + std::to_string(g.shotsTotal);
    }
    float pulse = 0.5f + 0.5f * std::sin(g.time * 4.f);
    drawGlowCutRect(t, W / 2.f - 320, 78, 640, 42, 14, withAlpha(bc, (int)(30 + 25 * pulse)), bc, 2.f, 1.f);
    drawText(t, gFonts.bold, bt, 19, W / 2.f, 99, col::WHITE, AL_CENTER, 3.f);

    // boards
    int hr = -1, hc = -1;
    if (humanAim) cellAt((float)g.mx, (float)g.my, G_LEFT, hr, hc);
    drawText(t, gFonts.bold, "ENEMY WATERS  //  " + O.name, 15, G_LEFT.x0 + 176, 138, col::CYAN, AL_CENTER, 2.f);
    drawText(t, gFonts.bold, "YOUR FLEET  //  " + V.name, 15, G_RIGHT.x0 + 176, 138, col::MAG, AL_CENTER, 2.f);
    drawBoard(t, g, O.board, G_LEFT, false, col::CYAN, humanAim, hr, hc);
    drawBoard(t, g, V.board, G_RIGHT, true, col::MAG, false, -1, -1);

    // missile in flight
    if (g.phase == PH_FLIGHT) {
        bool left = (g.cur == g.viewer);
        const Geom& G = left ? G_LEFT : G_RIGHT;
        float tx = cellCX(G, g.tgtC), ty = cellCY(G, g.tgtR);
        float sx = tx + (left ? -160.f : 160.f), sy = 30.f;
        float p = std::min(1.f, g.phaseT / 0.38f);
        drawReticle(t, tx, ty, G.cs, g.time, col::MAG);
        for (int k = 12; k >= 1; k--) {
            float q = p - k * 0.03f;
            if (q < 0) continue;
            float qx = sx + (tx - sx) * q, qy = sy + (ty - sy) * q;
            fillRect(t, qx - 2, qy - 2, 4, 4, withAlpha(col::MAG, (int)(200 * (1.f - k / 13.f))));
        }
        float hx = sx + (tx - sx) * p, hy = sy + (ty - sy) * p;
        drawRadialGlow(t, hx, hy, 22.f, withAlpha(col::WHITE, 150));
        drawCircle(t, hx, hy, 4.f, 0.f, col::WHITE, col::WHITE);
    }

    // middle panel
    drawGlowCutRect(t, 490, 156, 220, 420, 14, withAlpha(col::PANEL, 235), col::CYAN, 2.f, 0.7f);
    const float cx = 600.f;
    drawText(t, gFonts.mono, "SHOTS LEFT", 13, cx, 182, col::DIM, AL_CENTER, 2.f);
    int n = std::min(g.shotsTotal, 8);
    for (int i = 0; i < n; i++) {
        float px = cx - (n - 1) * 12.f + i * 24.f;
        if (i < g.shotsLeft) { drawRadialGlow(t, px, 212, 16, withAlpha(col::CYAN, 90)); drawCircle(t, px, 212, 8, 0.f, col::CYAN, col::CYAN); }
        else drawCircle(t, px, 212, 8, 2.f, withAlpha(col::DIM, 120));
    }
    drawLine(t, 510, 240, 690, 240, 1.f, withAlpha(col::CYAN, 60));
    drawText(t, gFonts.mono, "YOUR SCORE", 13, cx, 262, col::DIM, AL_CENTER, 2.f);
    drawGlowText(t, gFonts.bold, std::to_string(V.score), 30, cx, 292, col::LIME, AL_CENTER, 1.f, 0.8f);
    drawText(t, gFonts.body, "OPPONENT  " + std::to_string(O.score), 20, cx, 326, col::DIM, AL_CENTER, 1.f);
    drawText(t, gFonts.mono, "ACCURACY", 13, cx, 356, col::DIM, AL_CENTER, 2.f);
    int acc = V.shotsFired > 0 ? (100 * V.hits) / V.shotsFired : 0;
    drawText(t, gFonts.bold, std::to_string(acc) + "%", 24, cx, 384, col::CYAN, AL_CENTER, 1.f);
    drawLine(t, 510, 414, 690, 414, 1.f, withAlpha(col::CYAN, 60));
    drawText(t, gFonts.mono, "ENEMY FLEET", 13, cx, 434, col::DIM, AL_CENTER, 2.f);
    for (int i = 0; i < NUM_SHIPS; i++) {
        const Ship& s = O.board.ships[i];
        bool sunk = isSunk(s);
        float ry = 462.f + i * 22.f;
        sf::Color c = sunk ? col::MAG : col::CYAN;
        drawText(t, gFonts.body, s.name, 17, 506, ry, withAlpha(c, sunk ? 150 : 230), AL_LEFT, 0.f);
        for (int q = 0; q < s.size; q++)
            fillRect(t, 694.f - (s.size - q) * 11.f, ry - 4, 9, 9, sunk ? withAlpha(col::MAG, 220) : withAlpha(col::CYAN, 230));
        if (sunk) drawLine(t, 504, ry, 690, ry, 1.5f, withAlpha(col::MAG, 170));
    }

    // log
    int nl = (int)g.log.size();
    int shown = std::min(3, nl);
    for (int j = 0; j < shown; j++) {
        const LogLine& l = g.log[nl - shown + j];
        drawText(t, gFonts.body, l.text, 21, 70, 598.f + j * 22.f, withAlpha(l.color, 110 + 145 * (j + 1) / shown), AL_LEFT, 1.f);
    }

    drawEffects(t, g);

    if (g.confirmQuit) {
        fillRect(t, 0, 0, (float)W, (float)H, sf::Color(0, 0, 0, 200));
        drawGlowCutRect(t, 380, 250, 440, 190, 18, withAlpha(col::PANEL, 250), col::MAG, 2.f, 1.f);
        drawGlowText(t, gFonts.bold, "QUIT TO MENU?", 26, W / 2.f, 300, col::WHITE, AL_CENTER, 3.f, 0.8f);
        std::vector<Btn> b;
        b.push_back(mkBtn(1, 430, 350, 150, 48, "YES", col::MAG));
        b.push_back(mkBtn(2, 620, 350, 150, 48, "NO", col::CYAN));
        for (std::size_t i = 0; i < b.size(); i++) drawButton(t, g, b[i], 20);
    }
}

// ===================== RESULT =====================
static std::vector<Btn> resultButtons() {
    std::vector<Btn> v;
    v.push_back(mkBtn(1, 185, 578, 250, 52, "PLAY AGAIN", col::CYAN));
    v.push_back(mkBtn(2, 475, 578, 250, 52, "MAIN MENU", col::MAG));
    v.push_back(mkBtn(3, 765, 578, 250, 52, "LEADERBOARD", col::CYAN));
    return v;
}

static void resultEvent(Game& g, const Ev& e) {
    std::vector<Btn> b = resultButtons();
    if (e.type == EV_MOVE) setHot(g, hitBtn(b, (float)e.x, (float)e.y));
    else if (e.type == EV_PRESS && e.button == 1) {
        int id = hitBtn(b, (float)e.x, (float)e.y);
        if (id < 0) return;
        g.audio.play(SFX_CLICK);
        if (id == 1) startSession(g);
        else if (id == 2) changeScreen(g, SC_MENU);
        else if (id == 3) { g.scores = loadScores(); changeScreen(g, SC_SCORES); }
    } else if (e.type == EV_KEY && e.key == K_ENTER) startSession(g);
}

static void resultUpdate(Game& g, float dt) {
    if (!g.pl[g.winner].isAI && frand() < dt * 40.f) {      // confetti
        Particle p;
        p.x = frand() * W; p.y = -5; p.vx = (frand() - 0.5f) * 60.f; p.vy = 40.f + frand() * 60.f;
        p.maxLife = p.life = 3.2f; p.size = 3.f + frand() * 4.f;
        float k = frand();
        p.color = k < 0.4f ? col::CYAN : (k < 0.8f ? col::MAG : col::LIME);
        g.parts.push_back(p);
    }
}

static void resultDraw(sf::RenderTarget& t, Game& g) {
    Player& w = g.pl[g.winner];
    Player& l = g.pl[1 - g.winner];
    bool human = !w.isAI;
    std::string title = human ? (g.mode == MODE_VS_AI ? std::string("VICTORY") : w.name + " WINS") : std::string("DEFEAT");
    sf::Color tc = human ? col::LIME : col::MAG;
    drawGlowText(t, gFonts.title, title, title.size() > 9 ? 56 : 78, W / 2.f, 105, tc, AL_CENTER, 4.f, 1.4f);
    drawText(t, gFonts.body, human ? "ENEMY FLEET DESTROYED" : "YOUR FLEET WAS SUNK", 26, W / 2.f, 168, col::INK, AL_CENTER, 3.f);

    drawGlowCutRect(t, 250, 205, 700, 320, 20, withAlpha(col::PANEL, 235), tc, 2.f, 0.7f);
    drawGlowText(t, gFonts.bold, w.name, 22, 410, 245, human ? col::LIME : col::MAG, AL_CENTER, 2.f, 0.7f);
    drawText(t, gFonts.bold, l.name, 22, 790, 245, col::DIM, AL_CENTER, 2.f);
    drawText(t, gFonts.mono, "WINNER", 13, 410, 270, col::DIM, AL_CENTER, 3.f);
    drawLine(t, 280, 288, 920, 288, 1.f, withAlpha(col::CYAN, 70));

    const char* labels[5] = { "SCORE", "SHOTS FIRED", "HITS", "ACCURACY", "SHIPS LEFT" };
    for (int i = 0; i < 5; i++) {
        float y = 318.f + i * 40.f;
        std::string a, b;
        if (i == 0) { a = std::to_string(w.score); b = std::to_string(l.score); }
        if (i == 1) { a = std::to_string(w.shotsFired); b = std::to_string(l.shotsFired); }
        if (i == 2) { a = std::to_string(w.hits); b = std::to_string(l.hits); }
        if (i == 3) { a = std::to_string(w.shotsFired ? 100 * w.hits / w.shotsFired : 0) + "%"; b = std::to_string(l.shotsFired ? 100 * l.hits / l.shotsFired : 0) + "%"; }
        if (i == 4) { a = std::to_string(shipsAlive(w.board)); b = std::to_string(shipsAlive(l.board)); }
        drawText(t, gFonts.mono, labels[i], 15, 600, y, col::DIM, AL_CENTER, 2.f);
        drawText(t, gFonts.bold, a, 24, 410, y, col::WHITE, AL_CENTER, 1.f);
        drawText(t, gFonts.bold, b, 24, 790, y, col::INK, AL_CENTER, 1.f);
    }
    if (g.scoreSaved) drawText(t, gFonts.mono, "SCORE SAVED TO LEADERBOARD", 15, W / 2.f, 550, col::LIME, AL_CENTER, 3.f);

    std::vector<Btn> b = resultButtons();
    for (std::size_t i = 0; i < b.size(); i++) drawButton(t, g, b[i], 19);
    drawEffects(t, g);
}

// ===================== LEADERBOARD =====================
static std::vector<Btn> backButton() {
    std::vector<Btn> v;
    v.push_back(mkBtn(1, 500, 592, 200, 44, "BACK", col::MAG));
    return v;
}

static void backEvent(Game& g, const Ev& e) {
    std::vector<Btn> b = backButton();
    if (e.type == EV_MOVE) setHot(g, hitBtn(b, (float)e.x, (float)e.y));
    else if (e.type == EV_PRESS && e.button == 1) { if (hitBtn(b, (float)e.x, (float)e.y) == 1) { g.audio.play(SFX_CLICK); changeScreen(g, SC_MENU); } }
    else if (e.type == EV_KEY && (e.key == K_ESC || e.key == K_ENTER)) changeScreen(g, SC_MENU);
}

static void scoresDraw(sf::RenderTarget& t, Game& g) {
    drawHeader(t, g, "LEADERBOARD", "TOP COMMANDERS");
    drawGlowCutRect(t, 250, 130, 700, 445, 20, withAlpha(col::PANEL, 235), col::CYAN, 2.f, 0.7f);
    drawText(t, gFonts.mono, "RANK", 14, 300, 165, col::MAG, AL_CENTER, 2.f);
    drawText(t, gFonts.mono, "PLAYER", 14, 360, 165, col::MAG, AL_LEFT, 2.f);
    drawText(t, gFonts.mono, "SCORE", 14, 740, 165, col::MAG, AL_RIGHT, 2.f);
    drawText(t, gFonts.mono, "MODE", 14, 780, 165, col::MAG, AL_LEFT, 2.f);
    drawLine(t, 275, 182, 925, 182, 1.f, withAlpha(col::MAG, 120));
    if (g.scores.empty()) {
        drawText(t, gFonts.body, "NO RECORDS YET - WIN A GAME TO APPEAR HERE", 26, W / 2.f, 340, col::DIM, AL_CENTER, 2.f);
    }
    for (int i = 0; i < 8 && i < (int)g.scores.size(); i++) {
        float y = 216.f + i * 42.f;
        sf::Color c = i == 0 ? col::LIME : (i == 1 ? col::CYAN : (i == 2 ? col::MAG : col::INK));
        if (i % 2 == 0) fillRect(t, 262, y - 19, 676, 38, withAlpha(col::CYAN, 10));
        drawText(t, gFonts.bold, std::to_string(i + 1), 20, 300, y, c, AL_CENTER, 1.f);
        drawText(t, gFonts.bold, g.scores[i].name, 20, 360, y, c, AL_LEFT, 1.f);
        drawText(t, gFonts.bold, std::to_string(g.scores[i].score), 20, 740, y, c, AL_RIGHT, 1.f);
        drawText(t, gFonts.mono, g.scores[i].tag, 14, 780, y, col::DIM, AL_LEFT, 1.f);
    }
    std::vector<Btn> b = backButton();
    for (std::size_t i = 0; i < b.size(); i++) drawButton(t, g, b[i], 18);
}

// ===================== HELP =====================
static void helpDraw(sf::RenderTarget& t, Game& g) {
    drawHeader(t, g, "HOW TO PLAY", "COMMAND YOUR FLEET");
    drawGlowCutRect(t, 150, 130, 900, 445, 22, withAlpha(col::PANEL, 235), col::CYAN, 2.f, 0.7f);
    static const char* heads[5] = { "DEPLOY", "FIRE", "SALVO", "WIN", "KEYS" };
    static const char* text[5] = {
        "Place 5 ships on the grid. Click a placed ship to move it. Press R to rotate.",
        "Click the enemy waters to shoot.  HIT = +10 points,  MISS = -1 point.",
        "You fire (ships alive + 1) shots every turn. Lose ships, lose firepower.",
        "Sink all 5 enemy ships first. Ship sizes: 5, 4, 3, 3 and 2 cells.",
        "M = sound on/off.   ESC = back or quit.   R = rotate ship." };
    for (int i = 0; i < 5; i++) {
        float y = 172.f + i * 58.f;
        drawCutRect(t, 185, y - 20, 44, 40, 8, withAlpha(i % 2 ? col::MAG : col::CYAN, 40), i % 2 ? col::MAG : col::CYAN, 2.f);
        drawText(t, gFonts.bold, std::to_string(i + 1), 20, 207, y, col::WHITE, AL_CENTER, 0.f);
        drawText(t, gFonts.bold, heads[i], 15, 250, y - 12, i % 2 ? col::MAG : col::CYAN, AL_LEFT, 2.f);
        drawText(t, gFonts.body, text[i], 22, 250, y + 12, col::INK, AL_LEFT, 0.5f);
    }
    // legend
    float ly = 500;
    fillRect(t, 220, ly - 9, 18, 18, col::SHIP);                     drawText(t, gFonts.mono, "YOUR SHIP", 14, 246, ly, col::INK, AL_LEFT, 1.f);
    fillRect(t, 400, ly - 9, 18, 18, col::HIT);                      drawText(t, gFonts.mono, "HIT", 14, 426, ly, col::INK, AL_LEFT, 1.f);
    drawCircle(t, 549, ly, 4, 0.f, col::DIM, col::DIM);              drawText(t, gFonts.mono, "MISS", 14, 566, ly, col::INK, AL_LEFT, 1.f);
    fillRect(t, 700, ly - 9, 18, 18, sf::Color(110, 12, 96));        drawText(t, gFonts.mono, "SHIP SUNK", 14, 726, ly, col::INK, AL_LEFT, 1.f);
    std::vector<Btn> b = backButton();
    for (std::size_t i = 0; i < b.size(); i++) drawButton(t, g, b[i], 18);
}

// ------------------------------------------------------------
//  5) Main dispatch
// ------------------------------------------------------------
void gameInit(Game& g, sf::RenderWindow* win) {
    g.win = win;
    if (!gFonts.load()) std::fprintf(stderr, "could not load embedded fonts\n");
    g.audio.init();
}

void gameEvent(Game& g, const Ev& e) {
    if (e.type == EV_CLOSE) { g.running = false; return; }
    if (e.type == EV_MOVE || e.type == EV_PRESS || e.type == EV_RELEASE) { g.mx = e.x; g.my = e.y; }
    if (e.type == EV_KEY && e.key == 'M' && g.screen != SC_SETUP) { g.audio.muted = !g.audio.muted; return; }
    switch (g.screen) {
    case SC_MENU:   menuEvent(g, e);   break;
    case SC_SETUP:  setupEvent(g, e);  break;
    case SC_PLACE:  placeEvent(g, e);  break;
    case SC_COVER:  coverEvent(g, e);  break;
    case SC_BATTLE: battleEvent(g, e); break;
    case SC_RESULT: resultEvent(g, e); break;
    case SC_SCORES: backEvent(g, e);   break;
    case SC_HELP:   backEvent(g, e);   break;
    }
}

void gameUpdate(Game& g, float dt) {
    g.time += dt;
    if (g.fade > 0) { g.fade -= dt * 3.5f; if (g.fade < 0) g.fade = 0; }
    updateEffects(g, dt);
    if (g.screen == SC_BATTLE) battleUpdate(g, dt);
    else if (g.screen == SC_RESULT) resultUpdate(g, dt);
}

void gameDraw(Game& g, sf::RenderTarget& t) {
    sf::View base = t.getView();     // the letterboxed WxH view Main.cpp set up (unchanged for offscreen tests)
    sf::View v = base;
    if (g.shake > 0) v.move(sf::Vector2f(std::sin(g.time * 90.f) * g.shake * 9.f, std::cos(g.time * 77.f) * g.shake * 9.f));
    t.setView(v);

    drawBackdrop(t, g.time, W, H);
    switch (g.screen) {
    case SC_MENU:   menuDraw(t, g);   break;
    case SC_SETUP:  setupDraw(t, g);  break;
    case SC_PLACE:  placeDraw(t, g);  break;
    case SC_COVER:  coverDraw(t, g);  break;
    case SC_BATTLE: battleDraw(t, g); break;
    case SC_RESULT: resultDraw(t, g); break;
    case SC_SCORES: scoresDraw(t, g); break;
    case SC_HELP:   helpDraw(t, g);   break;
    }

    t.setView(base);
    if (g.audio.muted) drawText(t, gFonts.mono, "SOUND OFF  (M)", 13, W - 62, H - 34, withAlpha(col::DIM, 180), AL_RIGHT, 1.f);
    if (g.fade > 0) fillRect(t, 0, 0, (float)W, (float)H, sf::Color(0, 0, 0, (int)(g.fade * 255)));
}
