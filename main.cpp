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
        Game(int widthArg, int heightArg, int minesArg, int &r_scoreArg);
        void start();
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
        int &r_score;
        bool gameEnd;
        bool timerOn;
        Selection selected;
        Status status;
        int checkMines(int x, int y);
        void draw();
        void generate(int mines);
        void endGame(Status newStatus);
        void check();
        void discover(int x, int y);
        void logic();
        void barUpdater();
        int countMinesLeft();
};

Game::Game(int widthArg, int heightArg, int minesArg, int &r_scoreArg)
    : width(widthArg)
    , height(heightArg)
    , mines(minesArg)
    , cursorX(0)
    , cursorY(0)
    , selected(Selection::NONE)
    , gameEnd(0)
    , timerOn(0)
    , r_score(r_scoreArg)
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
    if (minefield[x][y] == 1) {
        return -1;
    }
    int mines = 0;
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
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

void Game::endGame(Status newStatus) {
    status = newStatus;
    timerOn = 0;
    draw();
    wgetch(win);
    gameEnd = 1;
}

void Game::check() {
    int discovered = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (selection[j][i] == Selection::DISCOVER && minefield[j][i] == 1) {
                endGame(Status::GAME_OVER);
                return;
            } else if (selection[j][i] == Selection::DISCOVER) {
                discovered++;
            }
        }
    }
    if (width * height - mines == discovered) {
        endGame(Status::WIN);
    }
}

void Game::discover(int x, int y) {
    if (selection[x][y] == Selection::NONE) {
        selection[x][y] = Selection::DISCOVER;
        if (checkMines(x, y) == 0) {
            for (int i = x - 1; i <= x + 1; i++) {
                for (int j = y - 1; j <= y + 1; j++) {
                    if (!(i == x && j == y)
                    && i >= 0 && i < width && j >= 0 && j < height) {
                        discover(i, j);
                    }
                }
            }
        }
    }
}

void Game::logic() {
    status = Status::IDLE;
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
            if (selected == Selection::DISCOVER) {
                timerOn = 1;
                discover(cursorX, cursorY);
                update = 0;
            } else {
                newValue = selected;
            }
        } else if ((oldValue == Selection::FLAG && selected == Selection::DISCOVER)
        || oldValue == Selection::DISCOVER) {
            update = 0;
        }
        if (update) {
            selection[cursorX][cursorY] = newValue;
        }
    }
    check();
}

int Game::countMinesLeft() {
    int flags = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (selection[j][i] == Selection::FLAG) {
                flags++;
            }
        }
    }
    int minesLeft = mines - flags;
    return minesLeft;
}

void Game::barUpdater() {
    int timerStart;
    int timer = 0;
    int oldTimer = -1;
    int oldMinesLeft = -1;
    bool oldTimerOn = 0;
    Status oldStatus = Status::IDLE;
    while (!gameEnd) {
        if (timerOn != oldTimerOn) {
            oldTimerOn = timerOn;
            if (timerOn) {
                timerStart = time(NULL);
            }
        }
        if (timerOn) {
            timer = time(NULL) - timerStart;
        }
        int minesLeft = countMinesLeft();
        if (timer != oldTimer || status != oldStatus || minesLeft != oldMinesLeft) {
            oldTimer = timer;
            oldMinesLeft = minesLeft;
            oldStatus = status;
            std::stringstream ss;
            ss << std::setfill('0') << std::setw(3) << minesLeft;
            std::string minesLeftString = ss.str();
            ss.str("");
            ss.clear();
            ss << std::setfill('0') << std::setw(3) << timer;
            std::string timerString = ss.str();
            m_writeToConsole.lock();
            wmove(bar, 1, 2);
            wprintw(bar, minesLeftString.c_str());
            wmove(bar, 1, 6);
            wprintw(bar, faces[status].c_str());
            wmove(bar, 1, 9);
            wprintw(bar, timerString.c_str());
            box(bar, 0, 0);
            wrefresh(bar);
            m_writeToConsole.unlock();
        }
    }
    r_score = timer;
}

void Game::start() {
    generate(mines);
    // set the timer
    std::thread barUpdaterThread(&Game::barUpdater, this);
    while (!gameEnd) {
        draw();
        logic();
    }
    barUpdaterThread.join();
}

int main() {
    srand(time(NULL));
    int score;
    int &r_score = score;
    initscr();
    Game game(10, 10, 10, r_score);
    game.start();
    endwin();
}
