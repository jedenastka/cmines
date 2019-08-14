#include <curses.h>
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
        ~Game();
        void start();
    private:
        enum class Selection {
            NONE,
            FLAG,
            QUESTION_MARK,
            DIG
        };
        enum class Status {
            IDLE,
            MAKING_MOVE,
            WIN,
            GAME_OVER
        };
        enum class Tile {
            FIELD,
            EMPTY,
            FLAG,
            QUESTION_MARK,
            MINE
        };
        WINDOW* win;
        WINDOW* bar;
        std::mutex m_writeToConsole;
        std::map<Status, std::string> faces;
        std::map<Tile, char> tileset;
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
        void dig(int x, int y);
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
    , status(Status::IDLE)
    , gameEnd(0)
    , timerOn(0)
    , r_score(r_scoreArg)
{
	clear();
	refresh();
	// make a win and configure
	win = newwin(height + 2, width + 4, 3, 0);
	keypad(win, 1);
	// make bar and configure
	bar = newwin(3, 14, 0, 0);
	// initialize faces map
	faces[Status::IDLE] = ":)";
	faces[Status::MAKING_MOVE] = ":O";
	faces[Status::GAME_OVER] = "X(";
	faces[Status::WIN] = "B)";
	// initialize tileset map
	tileset[Tile::FIELD] = '%';
	tileset[Tile::EMPTY] = ' ';
	tileset[Tile::FLAG] = '!';
	tileset[Tile::QUESTION_MARK] = '?';
	tileset[Tile::MINE] = 'X';
    // initialize ncurses color pairs
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_BLACK, COLOR_BLACK);
    init_pair(8, COLOR_WHITE, COLOR_BLACK);
	// generate mines
	generate(mines);
}

Game::~Game() {
	delwin(win);
	delwin(bar);
	clear();
	refresh();
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
    wclear(win);
    wrefresh(win);
    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            bool cursorOn = j == cursorX && i == cursorY;
            Selection selectionOn = selection[j][i];
            bool mineOn = minefield[j][i];
            char tile;
            auto attributes = A_NORMAL;
            if (mineOn && status == Status::GAME_OVER) {
                tile = tileset[Tile::MINE];
            } else {
                if (selectionOn == Selection::NONE) {
                    tile = tileset[Tile::FIELD];
                } else if (selectionOn == Selection::DIG) {
                    if (!mineOn) {
                        int mines = checkMines(j, i);
                        if (mines > 0) {
                            attributes = attributes | COLOR_PAIR(mines) | A_BOLD;
                            tile = std::to_string(mines)[0];
                        } else {
                            tile = tileset[Tile::EMPTY];
                        }
                    } else {
                        if (status == Status::WIN) {
                            tile = tileset[Tile::FLAG];
                        } else {
                            tile = tileset[Tile::MINE];
                        }
                    }
                } else if (selectionOn == Selection::FLAG) {
                    tile = tileset[Tile::FLAG];
                } else if (selectionOn == Selection::QUESTION_MARK) {
                    tile = tileset[Tile::QUESTION_MARK];
                }
            }
            if (cursorOn) {
                attributes = attributes | A_UNDERLINE;
            }
            /*int x;
            switch (selectionOn) {
                case Selection::NONE:
                    x = 0;
                    break;
                case Selection::DIG:
                    x = 1;
                    break;
                case Selection::FLAG:
                    x = 2;
                    break;
                case Selection::QUESTION_MARK:
                    x = 3;
                    break;
                default:
                    x = 9;
                    break;
            }
            tile = std::to_string(x)[0];*/
            wattron(win, attributes);
            mvwaddch(win, i + 1, j + 2, tile);
            wattroff(win, attributes);
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
    int diged = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (selection[j][i] == Selection::DIG && minefield[j][i] == 1) {
                endGame(Status::GAME_OVER);
                return;
            } else if (selection[j][i] == Selection::DIG) {
                diged++;
            }
        }
    }
    if (width * height - mines == diged) {
        endGame(Status::WIN);
    }
}

void Game::dig(int x, int y) {
    if (selection[x][y] == Selection::NONE || selection[x][y] == Selection::QUESTION_MARK) {
        selection[x][y] = Selection::DIG;
        if (checkMines(x, y) == 0) {
            for (int i = x - 1; i <= x + 1; i++) {
                for (int j = y - 1; j <= y + 1; j++) {
                    if (!(i == x && j == y)
                    && i >= 0 && i < width && j >= 0 && j < height) {
                        dig(i, j);
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
            selected = Selection::DIG;
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
        if (oldValue == Selection::DIG) {
			update = 0;
		} else if (selected == Selection::FLAG) {
            if (oldValue == Selection::FLAG) {
                newValue = Selection::QUESTION_MARK;
            } else if (oldValue == Selection::QUESTION_MARK) {
                newValue = Selection::NONE;
            } else if (oldValue == Selection::NONE) {
				newValue = Selection::FLAG;
			}
        } else if (selected == Selection::DIG) {
			if (oldValue == Selection::NONE || oldValue == Selection::QUESTION_MARK) {
				timerOn = 1;
                dig(cursorX, cursorY);
                update = 0;
			} else if (oldValue == Selection::FLAG) {
				update = 0;
			}
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
            if (timer > 999) {
                timer = 999;
            }
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
    bool newGame = 0;
    initscr();
    if (has_colors()) {
        start_color();
    }
    do {
		Game *p_game = new Game(10, 10, 10, r_score);
		p_game->start();
		delete p_game;
		printw("New game (y/N)? ");
		char input = getch();
		newGame = 0;
		switch (input) {
			case 'Y':
			case 'y':
				newGame = 1;
				break;
		}
	} while (newGame);
    endwin();
}
