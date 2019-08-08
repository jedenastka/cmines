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
        enum class Selection {
            NONE,
            FLAG,
            DISCOVER
        };
        WINDOW* win;
        bool minefield[10][10];
        Selection selection[10][10];
        int height;
        int width;
        int mines;
        int cursorX;
        int cursorY;
        Selection selected;
        bool gameEnd;
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
            if (selection[j][i] == Selection::NONE) {
                wprintw(win, "%%");
            } else if (selection[j][i] == Selection::DISCOVER) {
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
            } else if (selection[j][i] == Selection::FLAG) {
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

void Game::start() {
    generate(mines);
    while (!gameEnd) {
        draw();
        logic();
    }
}

int main() {
    srand(time(NULL));
    initscr();
    Game game(10, 10, 10);
    game.start();
    endwin();
}