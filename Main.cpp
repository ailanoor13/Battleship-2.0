// ============================================================
//  Main.cpp  -  opens the window and runs the game loop
// ============================================================
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "Game.h"
#include <ctime>
#include <cstdlib>

static void applyLetterbox(sf::RenderWindow& win) {
    sf::Vector2u sz = win.getSize();
    win.setView(compat::letterboxView((float)W, (float)H, sz.x, sz.y));
}

#if SFML3
static void toggleFullscreen(sf::RenderWindow& win, bool& isFull) {
    isFull = !isFull;
    if (isFull) win.create(sf::VideoMode::getDesktopMode(), "BATTLESHIP - SEMPEC", sf::Style::None, sf::State::Fullscreen);
    else        compat::createWindow(win, W, H, "BATTLESHIP - SEMPEC");
    applyLetterbox(win);
}
#else
static void toggleFullscreen(sf::RenderWindow& win, bool& isFull) {
    isFull = !isFull;
    if (isFull) win.create(sf::VideoMode::getDesktopMode(), "BATTLESHIP - SEMPEC", sf::Style::Fullscreen);
    else        compat::createWindow(win, W, H, "BATTLESHIP - SEMPEC");
    applyLetterbox(win);
}
#endif

int main() {
    std::srand((unsigned)std::time(nullptr));

    sf::RenderWindow win;
    compat::createWindow(win, W, H, "BATTLESHIP - SEMPEC");
    applyLetterbox(win);
    bool isFull = false;
#ifdef _WIN32
    FreeConsole();                     // hide the black console window
#endif

    Game g;
    gameInit(g, &win);

    sf::Clock clock;
    while (win.isOpen() && g.running) {
        compat::Ev ev;
        while (compat::pollEvent(win, ev)) {
            if (ev.type == compat::EV_RESIZE) { applyLetterbox(win); continue; }
            if (ev.type == compat::EV_KEY && ev.key == compat::K_F11) { toggleFullscreen(win, isFull); continue; }
            gameEvent(g, ev);
        }

        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;
        gameUpdate(g, dt);

        win.clear(sf::Color::Black);
        gameDraw(g, win);
        win.display();
    }
    win.close();
    return 0;
}
