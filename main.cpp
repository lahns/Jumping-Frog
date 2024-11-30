/*
I've used the following resouces in spite of writing this project:
-Example demo game "Catch the ball" by professor Małafiejski
-https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
*/

#include <cstring>
#include <ncurses/ncurses.h>
#include <cstdio>
#include <time.h>
#include "vector.cpp"

#define SIZE_PARAMETER "sizeyx"
#define TIME_PARAMETER "time"
#define MAIN_COLOR 0
#define FROG_COLOR 1
#define STAT_COLOR 2
#define DELAY_ON 1
#define DELAY_OFF 0
#define GAME_END_FLAG -1
#define FRAME_TIME 0.2 //one frame - 20 ms

struct win{
	WINDOW* window;
	int x, y;
	int rows, cols;
	int color;
};

struct frog{
	int x;
	int y;
	Vector<char*> *art = vector_init<char*>();
	int art_size = 3;
	short can_move;
};

struct game_model{
    int window_x_size;
    int window_y_size;
	float time;
	frog frog;
	clock_t last_frame;
	win* playwin;
	win* statwin;
	int last_inp;
};

void read_game_model(game_model &game){
    FILE *fptr; // pointer to a file
    char parameter[100];
    int x_size, y_size;
	float time;
    fptr = fopen("game_model.txt", "r");
    if(fscanf(fptr, "%s %d %d", parameter, &y_size, &x_size) == 3){
        if(strcmp(SIZE_PARAMETER, parameter) == 0){
            game.window_x_size = x_size;
            game.window_y_size = y_size;
        }
    }
	if(fscanf(fptr, "%s %f", parameter, &time) == 2){
		if(strcmp(TIME_PARAMETER, parameter) == 0){
            game.time = time;
        }
	}
}

WINDOW* Start()
{
	WINDOW* win;

	if ( (win = initscr()) == NULL ) {					// initialize ncurses
		fprintf(stderr, "Error initialising ncurses.\n");
		exit(EXIT_FAILURE);
    	}

	start_color();								// initialize colors
	init_pair(FROG_COLOR, COLOR_GREEN, COLOR_BLACK);
	init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
	init_pair(STAT_COLOR, COLOR_BLUE, COLOR_BLACK);
	noecho();
	nodelay(win, DELAY_ON);
	keypad(win, true);	
	cbreak();							// Switch off echoing, turn off cursor
	curs_set(0);
	return win;
}

void Welcome(WINDOW* win)							// Welcome screen (optional): press any key to continue
{	
	init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
	const char* welcome_text = "Do you want to play a game?";
	const char* tip_text = "Press any key to continue..";
	mvwaddstr(win, getmaxy(win)/2, (getmaxx(win)/2)-(strlen(welcome_text)/2), welcome_text);
	mvwaddstr(win, getmaxy(win)/2+1, (getmaxx(win)/2)-(strlen(tip_text)/2), tip_text);
	wgetch(win);								// waiting here..
	wclear(win);								// clear (after next refresh)
	wrefresh(win);
}

// void init_ncurses(){
// initscr();
// noecho();
// clear();
// keypad(stdscr, TRUE);
// curs_set(0);
// }
void CleanWin(win* W, int bo)							// bo(rder): 0 | 1
{
	werase(W->window);
	wattron(W->window,COLOR_PAIR(W->color));
	if (bo) box(W->window,0,0);
}

win* Init(WINDOW* parent, int rows, int cols, int y, int x, int color, int bo, int delay)
{							// C++ version; compile with g++
	win* W = new win;					// C version; compile with gcc
	W->x = x; W->y = y; W->rows = rows; W->cols = cols; W->color = color;
	W->window = subwin(parent, rows, cols, y, x);
	CleanWin(W, bo);
	if (delay == DELAY_OFF)  nodelay(W->window,TRUE);							// non-blocking reading of characters (for real-time game)
	wrefresh(W->window);
	return W;
}

void print_large_string(win &window, Vector<char*> *art, int x, int y, int color){
	int size = vector_size(art);
	wattron(window.window, COLOR_PAIR(color));
	for(int i = 0; i<size; i++){
		mvwprintw(window.window, y+i, x,"%s",vector_get(art, i));
	}
	wattroff(window.window, COLOR_PAIR(color));
}

frog setup_frog(game_model &game){
	frog frog;
	frog.x = getmaxx(game.playwin->window)/2; //start in the middle 
	frog.y = getmaxy(game.playwin->window)-4; //start at the bottom of the window
	vector_push_back(frog.art, (char*)" @.@ ");
	vector_push_back(frog.art, (char*)"( - )");
	vector_push_back(frog.art, (char*)"/   \\");
	frog.can_move = 1;
	return frog;
}

int handle_input(game_model &game) {
    frog* frog = &game.frog;
    game.last_inp = getch(); // Non-blocking input

    if (frog->can_move == 1) {
        switch (game.last_inp) {
            case ERR: // No input detected
                return 1;
            case KEY_UP:
                if (frog->y > 1) {
                    frog->y -= 1;
                    frog->can_move = 0;
                }
                break;
            case KEY_DOWN:
                if (frog->y < getmaxy(game.playwin->window) - 4) {
                    frog->y += 1;
                    frog->can_move = 0;
                }
                break;
            case KEY_RIGHT:
                if (frog->x < getmaxx(game.playwin->window) - 6) {
                    frog->x += 1;
                    frog->can_move = 0;
                }
                break;
            case KEY_LEFT:
                if (frog->x > 1) {
                    frog->x -= 1;
                    frog->can_move = 0;
                }
                break;
            case 'q': // Quit game
                return GAME_END_FLAG;
        }
    }
    return 1;
}


void show_timer(game_model &game){
	CleanWin(game.statwin, 1);
	mvwprintw(game.statwin->window,1,10, "Time: %.2f", game.time);
}

void refresh_both(game_model &game){
	wrefresh(game.playwin->window);
	wrefresh(game.statwin->window);
}

int main_game_loop(game_model &game) {
    CleanWin(game.playwin, 1);
    print_large_string(*game.playwin, game.frog.art, game.frog.x, game.frog.y, FROG_COLOR);
    show_timer(game);

    clock_t now = clock();
    double elapsed_time = (double)(now - game.last_frame) / CLOCKS_PER_SEC;
	game.time = float(elapsed_time);
    if (elapsed_time >= FRAME_TIME) {
        game.last_frame = now;
        game.frog.can_move = 1;
    }

    refresh_both(game);
    return handle_input(game); // Process input in every loop
}


int main()
{
    game_model game;
    read_game_model(game);
	WINDOW *main_window = Start();
	Welcome(main_window);
	int mid_x = getmaxx(main_window)/2/2;
	int mid_y = getmaxy(main_window)/2/2/2;
	game.playwin = Init(main_window, game.window_y_size, game.window_x_size, mid_y, mid_x, MAIN_COLOR, 1, DELAY_OFF);
	game.statwin = Init(main_window, 3, game.window_x_size, game.window_y_size+mid_y, mid_x, STAT_COLOR, 1, DELAY_OFF);	
	wrefresh(main_window);
	
	game.frog = setup_frog(game);

	while(main_game_loop(game) != GAME_END_FLAG){
	};

	refresh();			/* Print it on to the real screen */
    getch();			/* Wait for user input */
	endwin();			/* End curses mode		  */

}
