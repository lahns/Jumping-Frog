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
	keypad(win, true);								// Switch off echoing, turn off cursor
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
	wrefresh(window.window);
}

frog setup_frog(win &window){
	frog frog;
	frog.x = getmaxx(window.window)/2; //start in the middle 
	frog.y = getmaxy(window.window)-4; //start at the bottom of the window
	vector_push_back(frog.art, (char*)" @.@ ");
	vector_push_back(frog.art, (char*)"( - )");
	vector_push_back(frog.art, (char*)"/   \\");
	return frog;
}

int handle_input(win &playwin, game_model &game){
	frog* frog = &game.frog;
	
	if(frog->can_move == 1){
		switch(getch()){
		case KEY_UP:
			if(frog->y > 1){
				frog->y-=1;
				frog->can_move = 0;
			}
			break;
		case KEY_DOWN:
			if(frog->y < getmaxy(playwin.window)-4){  // frog size minus border
				frog->y+=1;
				frog->can_move = 0;
			}
			break;
		case KEY_RIGHT:
			if(frog->x < getmaxx(playwin.window)-6) { // frog width minus border
				frog->x+=1;
				frog->can_move = 0;
			}
			
			break;
		case KEY_LEFT:
			if(frog->x > 1){
				frog->x-=1;
				frog->can_move = 0;
			}
			
			break;
		case 'q':
			return GAME_END_FLAG;
	}
	}
	

	return 1;
}

int main_game_loop(win &statwin,win &playwin, game_model &game){
	CleanWin(&playwin, 1);
	print_large_string(playwin, game.frog.art, game.frog.x, game.frog.y, FROG_COLOR);
	clock_t now = clock();
	if((double)(now - game.last_frame) / CLOCKS_PER_SEC > 0.2){
		game.last_frame = now;
		game.frog.can_move = 1;
	}
	return handle_input(playwin, game);
	
}

int main()
{
    game_model game;
    read_game_model(game);
	WINDOW *main_window = Start();
	Welcome(main_window);
	int mid_x = getmaxx(main_window)/2/2;
	int mid_y = getmaxy(main_window)/2/2/2;
	win* playwin = Init(main_window, game.window_y_size, game.window_x_size, mid_y, mid_x, MAIN_COLOR, 1, DELAY_ON);
	win* statwin = Init(main_window, 3, game.window_x_size, game.window_y_size+mid_y, mid_x, STAT_COLOR, 1, DELAY_OFF);	
	wrefresh(main_window);
	
	game.frog = setup_frog(*playwin);

	while(main_game_loop(*statwin, *playwin, game) != GAME_END_FLAG){

	};

	refresh();			/* Print it on to the real screen */
    getch();			/* Wait for user input */
	endwin();			/* End curses mode		  */

}
