#include <ncurses.h>
#include <cstdlib>
#include <ctime>

int random(int from, int to) {
    return rand() % (to + 1) + from;
}

class Game {
    public:
        Game(int widthArg, int heightArg, int minesArg);
        void start();
    private:
        WINDOW* win;
        bool minefield[10][10];
        int selection[10][10];
        int height;
        int width;
        int mines;
        int cursorX;
        int cursorY;
        int selectedSquare;
        int checkMines(int x, int y);
        void draw();
        void generate(int mines);
};

Game::Game(int widthArg, int heightArg, int minesArg)
    : width(widthArg)
    , height(heightArg)
    , mines(minesArg)
    , cursorX(0)
    , cursorY(0)
    , selectedSquare(0)
    {
    // make a win and configure
    win = newwin(height + 1, width, 0, 0);
    keypad(win, 1);
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
    wrefresh(win);
    wclear(win);
    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            bool cursorOn = j == cursorX && i == cursorY;
            if (cursorOn) {
                wattron(win, A_UNDERLINE);
            }
            if (selection[j][i] == 0) {
                wprintw(win, "%%");
            } else if (selection[j][i] == 1) {
                if (minefield[j][i] == 0) {
                    int mines = checkMines(j, i);
                    if (mines > 0) {
                        wprintw(win, "%i", mines);
                    } else {
                        wprintw(win, " ");
                    }
                } else {
                    wprintw(win, "X");
                }
            } else if (selection[j][i] == 2) {
                wprintw(win, "+");
            }
            if (cursorOn) {
                wattroff(win, A_UNDERLINE);
            }
        }
    }
}

void Game::generate(int mines) {
    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            minefield[j][i] = 0;
            selection[j][i] = 0;
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

void Game::start() {
    generate(mines);
    while (1) {
        draw();
        selectedSquare = 0;
        auto key = wgetch(win);
        if (key == KEY_UP) {
            cursorY--;
        } else if (key == KEY_DOWN) {
            cursorY++;
        } else if (key == KEY_RIGHT) {
            cursorX++;
        } else if (key == KEY_LEFT) {
            cursorX--;
        } else if (key == 'q') {
            return;
        } else if (key == '\n') {
            selectedSquare = 1;
        } else if (key == ' ') {
            selectedSquare = 2;
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
        if (selectedSquare > 0) {
            int oldValue = selection[cursorX][cursorY];
            int newValue = selectedSquare;
            if (oldValue != 1) {
                if (oldValue == 2 && selectedSquare == 2) {
                    newValue = 0;
                }
                selection[cursorX][cursorY] = newValue;
            }
        }
    }
}

int main() {
    srand(time(NULL));
    initscr();
    Game game(10, 10, 10);
    game.start();
    endwin();
}