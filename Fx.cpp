#include "Fx.h"
#include "Fonts.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

FontSet gFonts;

sf::Color withAlpha(sf::Color c, int a) {
    if (a < 0) a = 0; if (a > 255) a = 255;
    return sf::Color(c.r, c.g, c.b, (std::uint8_t)a);
}

sf::Color mix(sf::Color a, sf::Color b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    return sf::Color((std::uint8_t)(a.r + (b.r - a.r) * t), (std::uint8_t)(a.g + (b.g - a.g) * t),
                     (std::uint8_t)(a.b + (b.b - a.b) * t), (std::uint8_t)(a.a + (b.a - a.a) * t));
}

bool FontSet::load() {
    bool ok = true;
    ok &= compat::loadFontFromMemory(title, kFontOrbitronBlack, kFontOrbitronBlackSize);
    ok &= compat::loadFontFromMemory(bold,  kFontOrbitronBold,  kFontOrbitronBoldSize);
    ok &= compat::loadFontFromMemory(body,  kFontRajdhani,      kFontRajdhaniSize);
    ok &= compat::loadFontFromMemory(mono,  kFontShareTech,     kFontShareTechSize);
    return ok;
}

// ------------------------------------------------------------
//  Text
// ------------------------------------------------------------
static float capCenter(const sf::Font& f, unsigned size) {
    sf::Text t = compat::makeText(f, "H", size);
    compat::Bounds b = compat::textBounds(t);
    return std::round(b.y + b.h * 0.5f);
}

float textWidth(const sf::Font& f, const std::string& s, unsigned size, float spacing) {
    sf::Text t = compat::makeText(f, s, size);
    t.setLetterSpacing(spacing);
    return compat::textBounds(t).w;
}

void drawText(sf::RenderTarget& target, const sf::Font& f, const std::string& s, unsigned size,
              float x, float y, sf::Color c, Align a, float spacing) {
    sf::Text t = compat::makeText(f, s, size);
    t.setFillColor(c);
    t.setLetterSpacing(spacing);
    compat::Bounds b = compat::textBounds(t);
    float ox = b.x;
    if (a == AL_CENTER) ox += b.w * 0.5f;
    if (a == AL_RIGHT)  ox += b.w;
    t.setOrigin(sf::Vector2f(std::round(ox), capCenter(f, size)));
    t.setPosition(sf::Vector2f(std::round(x), std::round(y)));
    target.draw(t);
}

void drawGlowText(sf::RenderTarget& target, const sf::Font& f, const std::string& s, unsigned size,
                  float x, float y, sf::Color c, Align a, float spacing, float glow) {
    const float PI = 3.14159265f;
    for (int layer = 3; layer >= 1; layer--) {
        float rad = layer * (1.2f + size / 40.f);
        int alpha = (int)(34.f * glow / layer);
        for (int k = 0; k < 10; k++) {
            float ang = k * (2 * PI / 10);
            drawText(target, f, s, size, x + std::cos(ang) * rad, y + std::sin(ang) * rad, withAlpha(c, alpha), a, spacing);
        }
    }
    drawText(target, f, s, size, x, y, c, a, spacing);
}

// ------------------------------------------------------------
//  Shapes
// ------------------------------------------------------------
void fillRect(sf::RenderTarget& t, float x, float y, float w, float h, sf::Color c) {
    sf::RectangleShape r(sf::Vector2f(w, h));
    r.setPosition(sf::Vector2f(x, y));
    r.setFillColor(c);
    t.draw(r);
}

void strokeRect(sf::RenderTarget& t, float x, float y, float w, float h, float th, sf::Color c) {
    sf::RectangleShape r(sf::Vector2f(w - th * 2, h - th * 2));
    r.setPosition(sf::Vector2f(x + th, y + th));
    r.setFillColor(sf::Color::Transparent);
    r.setOutlineColor(c);
    r.setOutlineThickness(th);
    t.draw(r);
}

void gradRectV(sf::RenderTarget& t, float x, float y, float w, float h, sf::Color top, sf::Color bottom) {
    sf::VertexArray va(sf::PrimitiveType::TriangleStrip, 4);
    va[0] = sf::Vertex{ sf::Vector2f(x, y), top };
    va[1] = sf::Vertex{ sf::Vector2f(x + w, y), top };
    va[2] = sf::Vertex{ sf::Vector2f(x, y + h), bottom };
    va[3] = sf::Vertex{ sf::Vector2f(x + w, y + h), bottom };
    t.draw(va);
}

void gradRectH(sf::RenderTarget& t, float x, float y, float w, float h, sf::Color left, sf::Color right) {
    sf::VertexArray va(sf::PrimitiveType::TriangleStrip, 4);
    va[0] = sf::Vertex{ sf::Vector2f(x, y), left };
    va[1] = sf::Vertex{ sf::Vector2f(x, y + h), left };
    va[2] = sf::Vertex{ sf::Vector2f(x + w, y), right };
    va[3] = sf::Vertex{ sf::Vector2f(x + w, y + h), right };
    t.draw(va);
}

void drawLine(sf::RenderTarget& t, float x1, float y1, float x2, float y2, float th, sf::Color c) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.01f) return;
    sf::RectangleShape r(sf::Vector2f(len, th));
    r.setOrigin(sf::Vector2f(0.f, th * 0.5f));
    r.setPosition(sf::Vector2f(x1, y1));
    compat::setRotationDeg(r, std::atan2(dy, dx) * 180.f / 3.14159265f);
    r.setFillColor(c);
    t.draw(r);
}

void drawGlowLine(sf::RenderTarget& t, float x1, float y1, float x2, float y2, float th, sf::Color c) {
    drawLine(t, x1, y1, x2, y2, th + 10, withAlpha(c, 14));
    drawLine(t, x1, y1, x2, y2, th + 6,  withAlpha(c, 26));
    drawLine(t, x1, y1, x2, y2, th + 2,  withAlpha(c, 70));
    drawLine(t, x1, y1, x2, y2, th,      c);
}

void drawCircle(sf::RenderTarget& t, float cx, float cy, float r, float th, sf::Color c, sf::Color fill) {
    sf::CircleShape s(r, 72);
    s.setOrigin(sf::Vector2f(r, r));
    s.setPosition(sf::Vector2f(cx, cy));
    s.setFillColor(fill);
    s.setOutlineColor(c);
    s.setOutlineThickness(th);
    t.draw(s);
}

void drawGlowCircle(sf::RenderTarget& t, float cx, float cy, float r, float th, sf::Color c) {
    drawCircle(t, cx, cy, r, th + 8, withAlpha(c, 18));
    drawCircle(t, cx, cy, r, th + 4, withAlpha(c, 40));
    drawCircle(t, cx, cy, r, th, c);
}

void drawRadialGlow(sf::RenderTarget& t, float cx, float cy, float r, sf::Color c) {
    const int N = 36;
    sf::VertexArray fan(sf::PrimitiveType::TriangleFan);
    fan.append(sf::Vertex{ sf::Vector2f(cx, cy), c });
    sf::Color rim = withAlpha(c, 0);
    for (int i = 0; i <= N; i++) {
        float a = i * 2 * 3.14159265f / N;
        fan.append(sf::Vertex{ sf::Vector2f(cx + std::cos(a) * r, cy + std::sin(a) * r), rim });
    }
    t.draw(fan);
}

void drawPoly(sf::RenderTarget& t, const std::vector<sf::Vector2f>& pts, sf::Color fill, sf::Color outline, float th) {
    sf::ConvexShape s;
    s.setPointCount(pts.size());
    for (std::size_t i = 0; i < pts.size(); i++) s.setPoint(i, pts[i]);
    s.setFillColor(fill);
    s.setOutlineColor(outline);
    s.setOutlineThickness(th);
    t.draw(s);
}

std::vector<sf::Vector2f> cutRectPts(float x, float y, float w, float h, float cut) {
    std::vector<sf::Vector2f> p;
    p.push_back(sf::Vector2f(x + cut, y));
    p.push_back(sf::Vector2f(x + w, y));
    p.push_back(sf::Vector2f(x + w, y + h - cut));
    p.push_back(sf::Vector2f(x + w - cut, y + h));
    p.push_back(sf::Vector2f(x, y + h));
    p.push_back(sf::Vector2f(x, y + cut));
    return p;
}

void drawCutRect(sf::RenderTarget& t, float x, float y, float w, float h, float cut,
                 sf::Color fill, sf::Color outline, float th) {
    drawPoly(t, cutRectPts(x, y, w, h, cut), fill, outline, -th);   // negative = outline drawn inside
}

void drawGlowCutRect(sf::RenderTarget& t, float x, float y, float w, float h, float cut,
                     sf::Color fill, sf::Color glowColor, float th, float glow) {
    for (int i = 3; i >= 1; i--) {
        float g = i * 3.f;
        drawPoly(t, cutRectPts(x - g, y - g, w + 2 * g, h + 2 * g, cut + g * 0.4f),
                 sf::Color::Transparent, withAlpha(glowColor, (int)(22 * glow / i)), 3.f);
    }
    drawCutRect(t, x, y, w, h, cut, fill, glowColor, th);
}

// ------------------------------------------------------------
//  Backdrop (grid + glows + slabs + scanlines + frame)
// ------------------------------------------------------------
void drawBackdrop(sf::RenderTarget& t, float time, int W, int H) {
    static sf::VertexArray grid(sf::PrimitiveType::Lines);
    static sf::VertexArray scan(sf::PrimitiveType::Lines);
    static bool built = false;
    if (!built) {
        sf::Color g(0, 229, 255, 20);
        for (int x = 0; x <= W; x += 50) { grid.append(sf::Vertex{ sf::Vector2f((float)x, 0.f), g }); grid.append(sf::Vertex{ sf::Vector2f((float)x, (float)H), g }); }
        for (int y = 0; y <= H; y += 50) { grid.append(sf::Vertex{ sf::Vector2f(0.f, (float)y), g }); grid.append(sf::Vertex{ sf::Vector2f((float)W, (float)y), g }); }
        sf::Color s(255, 255, 255, 7);
        for (int y = 0; y < H; y += 4) { scan.append(sf::Vertex{ sf::Vector2f(0.f, (float)y + 0.5f), s }); scan.append(sf::Vertex{ sf::Vector2f((float)W, (float)y + 0.5f), s }); }
        built = true;
    }

    fillRect(t, 0, 0, (float)W, (float)H, col::BG);
    drawRadialGlow(t, 120.f, 60.f, 620.f, withAlpha(col::CYAN, 42));
    drawRadialGlow(t, (float)W, 230.f, 560.f, withAlpha(col::MAG, 38));
    drawRadialGlow(t, 0.f, (float)H - 40, 480.f, withAlpha(col::MAG, 22));
    drawRadialGlow(t, (float)W - 60, (float)H, 600.f, withAlpha(col::CYAN, 40));
    t.draw(grid);

    // angular slabs (same idea as the panel)
    std::vector<sf::Vector2f> a, b, c;
    a.push_back(sf::Vector2f((float)W, 110.f)); a.push_back(sf::Vector2f((float)W, 380.f)); a.push_back(sf::Vector2f(W - 250.f, 110.f));
    drawPoly(t, a, withAlpha(col::MAG, 22));
    b.push_back(sf::Vector2f(0.f, H - 250.f)); b.push_back(sf::Vector2f(0.f, (float)H)); b.push_back(sf::Vector2f(230.f, (float)H));
    drawPoly(t, b, withAlpha(col::CYAN, 20));
    float pulse = 0.5f + 0.5f * std::sin(time * 1.3f);
    drawLine(t, 0, H - 60.f, (float)W, H - 90.f, 2.f, withAlpha(col::CYAN, (int)(30 + 25 * pulse)));

    t.draw(scan);

    // frame with cut corners
    const float m = 12.f, k = 26.f;
    float L = m, R = W - m, T = m, B = H - m;
    sf::Color c1 = col::CYAN, c2 = mix(col::CYAN, col::MAG, 0.5f), c3 = col::MAG;
    drawGlowLine(t, L + k, T, R - k, T, 2.f, c1);
    drawGlowLine(t, R - k, T, R, T + k, 2.f, c2);
    drawGlowLine(t, R, T + k, R, B - k, 2.f, c3);
    drawGlowLine(t, R, B - k, R - k, B, 2.f, c3);
    drawGlowLine(t, R - k, B, L + k, B, 2.f, c2);
    drawGlowLine(t, L + k, B, L, B - k, 2.f, c1);
    drawGlowLine(t, L, B - k, L, T + k, 2.f, c1);
    drawGlowLine(t, L, T + k, L + k, T, 2.f, c1);
    // magenta corner brackets
    float bl = 26.f;
    sf::Color m1 = col::MAG;
    drawLine(t, L + 26, T + 26, L + 26 + bl, T + 26, 4.f, m1);  drawLine(t, L + 26, T + 26, L + 26, T + 26 + bl, 4.f, m1);
    drawLine(t, R - 26, T + 26, R - 26 - bl, T + 26, 4.f, m1);  drawLine(t, R - 26, T + 26, R - 26, T + 26 + bl, 4.f, m1);
    drawLine(t, L + 26, B - 26, L + 26 + bl, B - 26, 4.f, m1);  drawLine(t, L + 26, B - 26, L + 26, B - 26 - bl, 4.f, m1);
    drawLine(t, R - 26, B - 26, R - 26 - bl, B - 26, 4.f, m1);  drawLine(t, R - 26, B - 26, R - 26, B - 26 - bl, 4.f, m1);
}

// ------------------------------------------------------------
//  Sound effects - generated in code, no audio files needed
// ------------------------------------------------------------
struct Audio::Impl {
    sf::SoundBuffer buffers[SFX_COUNT];
    std::unique_ptr<sf::Sound> sounds[SFX_COUNT][3];
    int next[SFX_COUNT];
    bool ready;
};

static const unsigned RATE = 44100;
static const float TWO_PI = 6.2831853f;

static float noise() { return (std::rand() / (float)RAND_MAX) * 2.f - 1.f; }

// one-pole low-pass filtered noise burst mixed with a sine "thump"
static std::vector<std::int16_t> makeBoom(float seconds, float noiseAmt, float lowHz, float cutoff, float thump) {
    int n = (int)(seconds * RATE);
    std::vector<std::int16_t> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / RATE;
        float env = std::exp(-t * (3.5f / seconds));
        lp += (noise() - lp) * cutoff;
        float v = lp * noiseAmt * env + std::sin(TWO_PI * lowHz * (1.f - 0.5f * t / seconds) * t) * thump * env;
        if (v > 1.f) v = 1.f; if (v < -1.f) v = -1.f;
        s[i] = (std::int16_t)(v * 30000);
    }
    return s;
}

static std::vector<std::int16_t> makeTone(float hz, float seconds, float vol, float sweep, bool square) {
    int n = (int)(seconds * RATE);
    std::vector<std::int16_t> s(n);
    float phase = 0.f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float f = hz + sweep * t;
        phase += TWO_PI * f / RATE;
        float env = (t < 0.05f ? t / 0.05f : 1.f) * std::exp(-t * 4.f);
        float v = square ? (std::sin(phase) > 0 ? 0.5f : -0.5f) : std::sin(phase) + 0.25f * std::sin(2 * phase);
        s[i] = (std::int16_t)(v * env * vol * 30000);
    }
    return s;
}

static std::vector<std::int16_t> makeWhoosh(float seconds) {
    int n = (int)(seconds * RATE);
    std::vector<std::int16_t> s(n);
    float lp = 0.f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float cutoff = 0.02f + 0.30f * t;
        lp += (noise() - lp) * cutoff;
        float env = std::sin(3.14159f * t);
        s[i] = (std::int16_t)(lp * env * 0.9f * 30000);
    }
    return s;
}

static std::vector<std::int16_t> concat(const std::vector<std::vector<std::int16_t> >& parts) {
    std::vector<std::int16_t> out;
    for (std::size_t i = 0; i < parts.size(); i++) out.insert(out.end(), parts[i].begin(), parts[i].end());
    return out;
}

Audio::Audio() : muted(false), impl(new Impl) { impl->ready = false; for (int i = 0; i < SFX_COUNT; i++) impl->next[i] = 0; }
Audio::~Audio() { delete impl; }

void Audio::init() {
    std::vector<std::int16_t> data[SFX_COUNT];
    data[SFX_CLICK]  = makeTone(900.f, 0.07f, 0.35f, 600.f, false);
    data[SFX_HOVER]  = makeTone(620.f, 0.03f, 0.12f, 0.f, false);
    data[SFX_PLACE]  = makeBoom(0.16f, 0.35f, 150.f, 0.2f, 0.7f);
    data[SFX_ERROR]  = makeTone(150.f, 0.16f, 0.4f, -40.f, true);
    data[SFX_LAUNCH] = makeWhoosh(0.32f);
    data[SFX_SPLASH] = makeBoom(0.45f, 0.8f, 0.f, 0.12f, 0.f);
    data[SFX_HIT]    = makeBoom(0.75f, 0.9f, 80.f, 0.35f, 0.9f);
    data[SFX_SUNK]   = makeBoom(1.3f, 1.0f, 55.f, 0.25f, 1.0f);
    {
        std::vector<std::vector<std::int16_t> > w;
        w.push_back(makeTone(523.f, 0.16f, 0.4f, 0.f, false)); w.push_back(makeTone(659.f, 0.16f, 0.4f, 0.f, false));
        w.push_back(makeTone(784.f, 0.16f, 0.4f, 0.f, false)); w.push_back(makeTone(1046.f, 0.5f, 0.45f, 0.f, false));
        data[SFX_WIN] = concat(w);
        std::vector<std::vector<std::int16_t> > l;
        l.push_back(makeTone(392.f, 0.22f, 0.4f, 0.f, true)); l.push_back(makeTone(330.f, 0.22f, 0.4f, 0.f, true));
        l.push_back(makeTone(262.f, 0.22f, 0.4f, 0.f, true)); l.push_back(makeTone(196.f, 0.6f, 0.4f, 0.f, true));
        data[SFX_LOSE] = concat(l);
    }
    bool all = true;
    for (int i = 0; i < SFX_COUNT; i++) {
        if (!compat::loadSamples(impl->buffers[i], data[i], RATE)) { all = false; continue; }
        for (int k = 0; k < 3; k++) impl->sounds[i][k].reset(new sf::Sound(impl->buffers[i]));
    }
    impl->ready = all;
}

void Audio::play(Sfx s) {
    if (muted) return;
    int k = impl->next[s];
    impl->next[s] = (k + 1) % 3;
    if (!impl->sounds[s][k]) return;
    impl->sounds[s][k]->setVolume(s == SFX_HOVER ? 40.f : 85.f);
    impl->sounds[s][k]->play();
}

// ------------------------------------------------------------
void drawOutlineText(sf::RenderTarget& target, const sf::Font& f, const std::string& s, unsigned size,
                     float x, float y, sf::Color c, float thickness, Align a, float spacing) {
    for (int pass = 0; pass < 3; pass++) {
        float th = thickness + (2 - pass) * 4.f;
        int alpha = (pass == 2) ? 255 : (pass == 1 ? 60 : 24);
        sf::Text t = compat::makeText(f, s, size);
        t.setFillColor(sf::Color::Transparent);
        t.setOutlineColor(withAlpha(c, alpha));
        t.setOutlineThickness(th);
        t.setLetterSpacing(spacing);
        compat::Bounds b = compat::textBounds(t);
        float ox = b.x;
        if (a == AL_CENTER) ox += b.w * 0.5f;
        if (a == AL_RIGHT)  ox += b.w;
        t.setOrigin(sf::Vector2f(std::round(ox), capCenter(f, size)));
        t.setPosition(sf::Vector2f(std::round(x), std::round(y)));
        target.draw(t);
    }
}
