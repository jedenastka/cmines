#include <ncurses.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <map>
#include <thread>
#include <ctime>
#include <mutex>

int random(int from, int to) {
    return rand() % (to + 1) + from;
}

class Game {
    public:
        Game(int widthArg, int heightArg, int minesArg);
        void start();
        void barUpdater();
    private:
        enum class Selection {
            NONE,
            FLAG,
            DISCOVER
        };
        enum class Status {
            IDLE,
            MAKING_MOVE,
            WIN,
            GAME_OVER
        };
        WINDOW* win;
        WINDOW* bar;
        std::mutex m_writeToConsole;
        std::map<Status, std::string> faces;
        bool minefield[10][10];
        Selection selection[10][10];
        int height;
        int width;
        int mines;
        int cursorX;
        int cursorY;
        Selection selected;
        bool gameEnd;
        int timerStart;
        int checkMines(int x, int y);
        void draw();
        void generate(int mines);
        void gameOver();
        void logic();
};

Game::Game(int widthArg, int heightArg, int minesArg)
    : width(widthArg)
    , height(heightArg)
    , mines(minesArg)
    , cursorX(0)
    , cursorY(0)
    , selected(Selection::NONE)
    , gameEnd(0)
    {
    // make a win and configure
    win = newwin(height + 2, width + 4, 3, 0);
    keypad(win, 1);
    // make bar and configure
    bar = newwin(3, 14, 0, 0);
    // initialise faces map
    faces[Status::IDLE] = ":)";
    faces[Status::MAKING_MOVE] = ":O";
    faces[Status::GAME_OVER] = "X(";
    faces[Status::WIN] = "B)";
}

int Game::checkMines(int x, int y) {
    int mines = 0;
    for (int i = x - 1; i <= x + 1; i++) {
        for(int j = y - 1; j <= y + 1; j++) {
            if (minefield[i][j] == 1 && i >= 0 && j >= 0 && i < height && j < width) {
                mines++;
            }
        }
    }
    return mines;
}

void Game::draw() {
    m_writeToConsole.lock();
    wrefresh(win);
    wclear(win);
    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            bool cursorOn = j == cursorX && i == cursorY;
            char tile;
            if (selection[j][i] == Selection::NONE) {
                tile = '%';
            } else if (selection[j][i] == Selection::DISCOVER) {
                if (minefield[j][i] == 0) {
                    int mines = checkMines(j, i);
                    if (mines > 0) {
                        tile = std::to_string(mines)[0];
                    } else {
                        tile = ' ';
                    }
                } else {
                    tile = 'X';
                }
            } else if (selection[j][i] == Selection::FLAG) {
                tile = '!';
            }
            if (cursorOn) {
                wattron(win, A_UNDERLINE);
            }
            mvwaddch(win, i + 1, j + 2, tile);
            if (cursorOn) {
                wattroff(win, A_UNDERLINE);
            }
        }
    }
    box(win, 0, 0);
    wrefresh(win);
    m_writeToConsole.unlock();
}

void Game::generate(int mines) {
    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            minefield[j][i] = 0;
            selection[j][i] = Selection::NONE;
        }
    }
    for (int i = 0; i < mines; i++) {
        while (1) {
            int x = random(0, width - 1);
            int y = random(0, height - 1);
            if (minefield[x][y] == 0) {
                minefield[x][y] = 1;
                break;
            }
        }
    }
}

void Game::gameOver() {
    gameEnd = 1;
    clear();
    printw("Game over!\n");
    refresh();
    getch();
}

void Game::logic() {
    selected = Selection::NONE;
    auto key = wgetch(win);
    switch (key) {
        case KEY_UP:
            cursorY--;
            break;
        case KEY_DOWN:
            cursorY++;
            break;
        case KEY_RIGHT:
            cursorX++;
            break;
        case KEY_LEFT:
            cursorX--;
            break;
        case 'q':
            gameEnd = 1;
            break;
        case '\n':
            selected = Selection::DISCOVER;
            break;
        case ' ':
            selected = Selection::FLAG;
            break;
    }
    if (cursorX > width - 1) {
        cursorX = width - 1;
    } else if (cursorX < 0) {
        cursorX = 0;
    } else if (cursorY > height - 1) {
        cursorY = height - 1;
    } else if (cursorY < 0) {
        cursorY = 0;
    }
    if (selected != Selection::NONE) {
        Selection oldValue = selection[cursorX][cursorY];
        Selection newValue;
        bool update = 1;
        if (oldValue == Selection::FLAG && selected == Selection::FLAG) {
            newValue = Selection::NONE;
        } else if (oldValue == Selection::NONE) {
            newValue = selected;
        } else if ((oldValue == Selection::FLAG && selected == Selection::DISCOVER)
        || oldValue == Selection::DISCOVER ) {
            update = 0;
        }
        if (update) {
            selection[cursorX][cursorY] = newValue;
        }
    }
    if (selection[cursorX][cursorY] == Selection::DISCOVER && minefield[cursorX][cursorY] == 1) {
        gameOver();
    }
}

void Game::barUpdater() {
    int remainingMines = 10;
    int oldTimer = -1;
    while (!gameEnd) {
        int timer = time(NULL) - timerStart;
        if (timer != oldTimer) {
            oldTimer = timer;
            std::stringstream ss;
            ss << std::setfill('0') << std::setw(3) << remainingMines;
            std::string remainingMinesString = ss.str();
            ss.str("");
            ss.clear();
            ss << std::setfill('0') << std::setw(3) << timer;
            std::string timerString = ss.str();
            m_writeToConsole.lock();
            wmove(bar, 1, 2);
            wprintw(bar, remainingMinesString.c_str());
            wmove(bar, 1, 6);
            wprintw(bar, faces[Status::IDLE].c_str());
            wmove(bar, 1, 9);
            wprintw(bar, timerString.c_str());
            box(bar, 0, 0);
            wrefresh(bar);
            m_writeToConsole.unlock();
        }
    }
}

void Game::start() {
    generate(mines);
    // set the timer
    timerStart = time(NULL);
    std::thread barUpdaterThread(&Game::barUpdater, this);
    while (!gameEnd) {
        draw();
        logic();
    }
    barUpdaterThread.join();
}

int main() {
    srand(time(NULL));
    initscr();
    Game game(10, 10, 10);
    game.start();
    endwin();
}