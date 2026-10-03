// ============================================================
//  Fx.h  -  drawing helpers (neon style) and sound effects
// ============================================================
#pragma once
#include "Compat.h"

namespace col {
    const sf::Color BG(3, 4, 10);
    const sf::Color CYAN(0, 229, 255);
    const sf::Color MAG(255, 43, 214);
    const sf::Color LIME(198, 255, 61);
    const sf::Color INK(214, 246, 255);
    const sf::Color DIM(111, 188, 208);
    const sf::Color WATER(10, 42, 85);
    const sf::Color SHIP(0, 184, 212);
    const sf::Color HIT(255, 43, 106);
    const sf::Color WHITE(255, 255, 255);
    const sf::Color PANEL(5, 10, 22);
}

sf::Color withAlpha(sf::Color c, int a);
sf::Color mix(sf::Color a, sf::Color b, float t);

// ---------------- fonts (embedded in the exe) ----------------
struct FontSet {
    sf::Font title;   // Orbitron Black
    sf::Font bold;    // Orbitron Bold
    sf::Font body;    // Rajdhani SemiBold
    sf::Font mono;    // Share Tech Mono
    bool load();
};
extern FontSet gFonts;

// ---------------- text ----------------
enum Align { AL_LEFT, AL_CENTER, AL_RIGHT };
void  drawText(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size,
               float x, float y, sf::Color c, Align a = AL_LEFT, float spacing = 1.f);
void  drawGlowText(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size,
                   float x, float y, sf::Color c, Align a = AL_CENTER, float spacing = 1.f, float glow = 1.f);
float textWidth(const sf::Font& f, const std::string& s, unsigned size, float spacing = 1.f);

// ---------------- shapes ----------------
void fillRect(sf::RenderTarget& t, float x, float y, float w, float h, sf::Color c);
void strokeRect(sf::RenderTarget& t, float x, float y, float w, float h, float th, sf::Color c);
void gradRectV(sf::RenderTarget& t, float x, float y, float w, float h, sf::Color top, sf::Color bottom);
void gradRectH(sf::RenderTarget& t, float x, float y, float w, float h, sf::Color left, sf::Color right);
void drawLine(sf::RenderTarget& t, float x1, float y1, float x2, float y2, float th, sf::Color c);
void drawGlowLine(sf::RenderTarget& t, float x1, float y1, float x2, float y2, float th, sf::Color c);
void drawCircle(sf::RenderTarget& t, float cx, float cy, float r, float th, sf::Color c, sf::Color fill = sf::Color::Transparent);
void drawGlowCircle(sf::RenderTarget& t, float cx, float cy, float r, float th, sf::Color c);
void drawRadialGlow(sf::RenderTarget& t, float cx, float cy, float r, sf::Color c);
void drawPoly(sf::RenderTarget& t, const std::vector<sf::Vector2f>& pts, sf::Color fill, sf::Color outline = sf::Color::Transparent, float th = 0.f);

// rectangle with two cut corners (top-left and bottom-right), the "gaming" shape from the panel
std::vector<sf::Vector2f> cutRectPts(float x, float y, float w, float h, float cut);
void drawCutRect(sf::RenderTarget& t, float x, float y, float w, float h, float cut,
                 sf::Color fill, sf::Color outline, float th = 2.f);
void drawGlowCutRect(sf::RenderTarget& t, float x, float y, float w, float h, float cut,
                     sf::Color fill, sf::Color glowColor, float th = 2.f, float glow = 1.f);

// full-screen background: grid, glows, slabs, scanlines and the frame
void drawBackdrop(sf::RenderTarget& t, float time, int W, int H);

// ---------------- sound ----------------
enum Sfx { SFX_CLICK, SFX_HOVER, SFX_PLACE, SFX_ERROR, SFX_LAUNCH, SFX_SPLASH, SFX_HIT, SFX_SUNK, SFX_WIN, SFX_LOSE, SFX_COUNT };

struct Audio {
    Audio();
    ~Audio();
    void init();
    void play(Sfx s);
    bool muted;
private:
    struct Impl;
    Impl* impl;
};

// outlined text (transparent fill, neon outline) - used for the big "SHIP" title
void drawOutlineText(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size,
                     float x, float y, sf::Color c, float thickness, Align a = AL_CENTER, float spacing = 1.f);
