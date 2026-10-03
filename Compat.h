// ============================================================
//  Compat.h  -  makes the same code build with SFML 2.6 AND SFML 3.x
//  (SFML 3 changed many function names; all the differences live here)
// ============================================================
#pragma once
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#if SFML_VERSION_MAJOR >= 3
    #define SFML3 1
    #include <optional>
#else
    #define SFML3 0
#endif

namespace compat {

// ---------------- our own simple event type ----------------
enum EvType { EV_NONE, EV_CLOSE, EV_MOVE, EV_PRESS, EV_RELEASE, EV_KEY, EV_TEXT, EV_RESIZE };

// key codes: letters are their ASCII capital letter ('A'..'Z'), others below
enum KeyCode { K_NONE = 0, K_ENTER = 1000, K_ESC, K_BACK, K_TAB, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_SPACE, K_F11 };

struct Ev {
    EvType   type   = EV_NONE;
    int      x      = 0;
    int      y      = 0;
    int      button = 0;      // 1 = left, 2 = right
    int      key    = K_NONE;
    unsigned text   = 0;      // unicode character for EV_TEXT
};

// ---------------- window ----------------
inline void createWindow(sf::RenderWindow& w, unsigned width, unsigned height, const char* title) {
#if SFML3
    w.create(sf::VideoMode(sf::Vector2u(width, height)), title, sf::Style::Default);
#else
    w.create(sf::VideoMode(width, height), title, sf::Style::Default);
#endif
    w.setFramerateLimit(60);
#if SFML3
    w.setMinimumSize(sf::Vector2u(480, 270));
#endif
}

// A view sized logicalW x logicalH, letterboxed (with black bars) so the whole
// game is visible and centered no matter what size or aspect ratio the window is.
inline sf::View letterboxView(float logicalW, float logicalH, unsigned winW, unsigned winH) {
    if (winW == 0) winW = 1; if (winH == 0) winH = 1;
#if SFML3
    sf::View v(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f(logicalW, logicalH)));
#else
    sf::View v(sf::FloatRect(0.f, 0.f, logicalW, logicalH));
#endif
    float windowRatio = (float)winW / (float)winH;
    float viewRatio = logicalW / logicalH;
    float sx = 1.f, sy = 1.f, px = 0.f, py = 0.f;
    if (windowRatio > viewRatio) { sx = viewRatio / windowRatio; px = (1.f - sx) * 0.5f; }
    else                         { sy = windowRatio / viewRatio; py = (1.f - sy) * 0.5f; }
#if SFML3
    v.setViewport(sf::FloatRect(sf::Vector2f(px, py), sf::Vector2f(sx, sy)));
#else
    v.setViewport(sf::FloatRect(px, py, sx, sy));
#endif
    return v;
}

#if SFML3
inline int mapKey(sf::Keyboard::Key k) {
    using K = sf::Keyboard::Key;
    if (k >= K::A && k <= K::Z) return 'A' + (int)k - (int)K::A;
    switch (k) {
    case K::Enter:     return K_ENTER;
    case K::Escape:    return K_ESC;
    case K::Backspace: return K_BACK;
    case K::Tab:       return K_TAB;
    case K::Up:        return K_UP;
    case K::Down:      return K_DOWN;
    case K::Left:      return K_LEFT;
    case K::Right:     return K_RIGHT;
    case K::Space:     return K_SPACE;
    case K::F11:       return K_F11;
    default:           return K_NONE;
    }
}

inline bool pollEvent(sf::RenderWindow& w, Ev& out) {
    while (const std::optional<sf::Event> e = w.pollEvent()) {
        out = Ev();
        if (e->is<sf::Event::Closed>()) { out.type = EV_CLOSE; return true; }
        if (const auto* m = e->getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f p = w.mapPixelToCoords(m->position);
            out.type = EV_MOVE; out.x = (int)p.x; out.y = (int)p.y; return true;
        }
        if (const auto* b = e->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f p = w.mapPixelToCoords(b->position);
            out.type = EV_PRESS; out.x = (int)p.x; out.y = (int)p.y;
            out.button = (b->button == sf::Mouse::Button::Left) ? 1 : 2; return true;
        }
        if (const auto* b = e->getIf<sf::Event::MouseButtonReleased>()) {
            sf::Vector2f p = w.mapPixelToCoords(b->position);
            out.type = EV_RELEASE; out.x = (int)p.x; out.y = (int)p.y;
            out.button = (b->button == sf::Mouse::Button::Left) ? 1 : 2; return true;
        }
        if (const auto* r = e->getIf<sf::Event::Resized>()) {
            out.type = EV_RESIZE; out.x = (int)r->size.x; out.y = (int)r->size.y; return true;
        }
        if (const auto* k = e->getIf<sf::Event::KeyPressed>()) {
            int code = mapKey(k->code);
            if (code != K_NONE) { out.type = EV_KEY; out.key = code; return true; }
        }
        if (const auto* t = e->getIf<sf::Event::TextEntered>()) {
            out.type = EV_TEXT; out.text = (unsigned)t->unicode; return true;
        }
    }
    return false;
}
#else
inline int mapKey(sf::Keyboard::Key k) {
    if (k >= sf::Keyboard::A && k <= sf::Keyboard::Z) return 'A' + (int)k - (int)sf::Keyboard::A;
    switch (k) {
    case sf::Keyboard::Return:    return K_ENTER;
    case sf::Keyboard::Escape:    return K_ESC;
    case sf::Keyboard::BackSpace: return K_BACK;
    case sf::Keyboard::Tab:       return K_TAB;
    case sf::Keyboard::Up:        return K_UP;
    case sf::Keyboard::Down:      return K_DOWN;
    case sf::Keyboard::Left:      return K_LEFT;
    case sf::Keyboard::Right:     return K_RIGHT;
    case sf::Keyboard::Space:     return K_SPACE;
    case sf::Keyboard::F11:       return K_F11;
    default:                      return K_NONE;
    }
}

inline bool pollEvent(sf::RenderWindow& w, Ev& out) {
    sf::Event e;
    while (w.pollEvent(e)) {
        out = Ev();
        switch (e.type) {
        case sf::Event::Closed:
            out.type = EV_CLOSE; return true;
        case sf::Event::MouseMoved: {
            sf::Vector2f p = w.mapPixelToCoords(sf::Vector2i(e.mouseMove.x, e.mouseMove.y));
            out.type = EV_MOVE; out.x = (int)p.x; out.y = (int)p.y; return true;
        }
        case sf::Event::MouseButtonPressed: {
            sf::Vector2f p = w.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
            out.type = EV_PRESS; out.x = (int)p.x; out.y = (int)p.y;
            out.button = (e.mouseButton.button == sf::Mouse::Left) ? 1 : 2; return true;
        }
        case sf::Event::MouseButtonReleased: {
            sf::Vector2f p = w.mapPixelToCoords(sf::Vector2i(e.mouseButton.x, e.mouseButton.y));
            out.type = EV_RELEASE; out.x = (int)p.x; out.y = (int)p.y;
            out.button = (e.mouseButton.button == sf::Mouse::Left) ? 1 : 2; return true;
        }
        case sf::Event::Resized:
            out.type = EV_RESIZE; out.x = (int)e.size.width; out.y = (int)e.size.height; return true;
        case sf::Event::KeyPressed: {
            int code = mapKey(e.key.code);
            if (code != K_NONE) { out.type = EV_KEY; out.key = code; return true; }
            break;
        }
        case sf::Event::TextEntered:
            out.type = EV_TEXT; out.text = e.text.unicode; return true;
        default: break;
        }
    }
    return false;
}
#endif

// ---------------- text ----------------
inline sf::Text makeText(const sf::Font& f, const std::string& s, unsigned size) {
#if SFML3
    return sf::Text(f, sf::String(s), size);
#else
    return sf::Text(sf::String(s), f, size);
#endif
}

struct Bounds { float x, y, w, h; };
inline Bounds textBounds(const sf::Text& t) {
    sf::FloatRect r = t.getLocalBounds();
#if SFML3
    Bounds b = { r.position.x, r.position.y, r.size.x, r.size.y };
#else
    Bounds b = { r.left, r.top, r.width, r.height };
#endif
    return b;
}

inline bool loadFontFromMemory(sf::Font& f, const void* data, std::size_t size) {
#if SFML3
    return f.openFromMemory(data, size);
#else
    return f.loadFromMemory(data, size);
#endif
}

// ---------------- rotation ----------------
template <typename T>
inline void setRotationDeg(T& shape, float deg) {
#if SFML3
    shape.setRotation(sf::degrees(deg));
#else
    shape.setRotation(deg);
#endif
}

// ---------------- audio ----------------
inline bool loadSamples(sf::SoundBuffer& buf, const std::vector<std::int16_t>& s, unsigned rate) {
#if SFML3
    return buf.loadFromSamples(s.data(), s.size(), 1, rate, { sf::SoundChannel::Mono });
#else
    return buf.loadFromSamples(s.data(), s.size(), 1, rate);
#endif
}

} // namespace compat
