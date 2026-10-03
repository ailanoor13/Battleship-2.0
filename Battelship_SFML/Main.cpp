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

int main() {
    std::srand((unsigned)std::time(nullptr));

    sf::RenderWindow win;
    compat::createWindow(win, W, H, "BATTLESHIP - SEMPEC");
#ifdef _WIN32
    FreeConsole();                     // hide the black console window
#endif

    Game g;
    gameInit(g, &win);

    sf::Clock clock;
    while (win.isOpen() && g.running) {
        compat::Ev ev;
        while (compat::pollEvent(win, ev)) gameEvent(g, ev);

        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;
        gameUpdate(g, dt);

        win.clear();
        gameDraw(g, win);
        win.display();
    }
    win.close();
    return 0;
}
