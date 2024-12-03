/*
I've used the following resouces in spite of writing this project:
-Example demo game "Catch the ball" by professor Małafiejski
-https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
*/

#include <cstring>
#include <ctime>
#include <ncurses.h>
#include <cstdio>
#include <time.h>
#include "vector.cpp"

#define SIZE_PARAMETER "sizeyx"
#define TIME_PARAMETER "time"
#define MAIN_COLOR 0
#define FROG_COLOR 1
#define STAT_COLOR 2
#define PLAIN_COLOR 3
#define ROAD_COLOR 4
#define COLOR_GRAY 10
#define DELAY_ON 1
#define DELAY_OFF 0
#define GAME_END_FLAG -1
#define FRAME_TIME 0.2 // one frame - 20 ms
#define RAN_OUT_OF_TIME_ENDING 1
#define GOT_HIT_ENDING 2
#define SMALL_CAR 1
#define BIG_CAR 2

struct win
{
	WINDOW *window;
	int x, y;
	int rows, cols;
	int color;
};

struct frog
{
	int x;
	int y;
	Vector<char *> *art = vector_init<char *>();
	int art_size = 3;
	short can_move;
};

struct car
{
	int art_size;
	Vector<char *> *art = vector_init<char *>();
	int x;
	int y;
	float move_time;
	int type;
	clock_t last_frame;
};

struct game_model
{
	int window_x_size;
	int window_y_size;
	float time;
	frog frog;
	clock_t last_frame;
	win *playwin;
	win *statwin;
	int last_inp;
	int ending;
	Vector<car> *cars = vector_init<car>();
	Vector<int> *street_corners = vector_init<int>();
};

void read_game_model(game_model &game)
{
	FILE *fptr; // pointer to a file
	char parameter[100];
	int x_size, y_size;
	float time;
	game.last_frame = clock();
	fptr = fopen("game_model.txt", "r");
	if (fscanf(fptr, "%s %d %d", parameter, &y_size, &x_size) == 3)
	{
		if (strcmp(SIZE_PARAMETER, parameter) == 0)
		{
			game.window_x_size = x_size;
			game.window_y_size = y_size;
		}
	}
	if (fscanf(fptr, "%s %f", parameter, &time) == 2)
	{
		if (strcmp(TIME_PARAMETER, parameter) == 0)
		{
			game.time = time;
		}
	}
}

WINDOW *Start()
{
	WINDOW *win;

	if ((win = initscr()) == NULL)
	{ // initialize ncurses
		fprintf(stderr, "Error initialising ncurses.\n");
		exit(EXIT_FAILURE);
	}

	start_color();// initialize colors
	init_pair(FROG_COLOR, COLOR_GREEN, COLOR_BLACK);
	init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
	init_pair(STAT_COLOR, COLOR_BLUE, COLOR_BLACK);
	init_pair(PLAIN_COLOR, COLOR_BLACK, COLOR_GREEN);
	init_pair(ROAD_COLOR, COLOR_BLACK, COLOR_MAGENTA);
	noecho();
	nodelay(win, DELAY_ON);
	keypad(win, true);
	cbreak(); // Switch off echoing, turn off cursor
	curs_set(0);
	return win;
}

void Welcome(WINDOW *win) // Welcome screen (optional): press any key to continue
{
	init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
	const char *welcome_text = "Do you want to play a game?";
	const char *tip_text = "Press any key to continue..";
	mvwaddstr(win, getmaxy(win) / 2, (getmaxx(win) / 2) - (strlen(welcome_text) / 2), welcome_text);
	mvwaddstr(win, getmaxy(win) / 2 + 1, (getmaxx(win) / 2) - (strlen(tip_text) / 2), tip_text);
	while (wgetch(win) == ERR)
		;		 // waiting here..
	wclear(win); // clear (after next refresh)
	wrefresh(win);
}

void CleanWin(win *W, int bo) // bo(rder): 0 | 1
{
	werase(W->window);
	wattron(W->window, COLOR_PAIR(W->color));
	if (bo)
		box(W->window, 0, 0);
}

win *Init(WINDOW *parent, int rows, int cols, int y, int x, int color, int bo, int delay)
{
	win *W = new win;
	W->x = x;
	W->y = y;
	W->rows = rows;
	W->cols = cols;
	W->color = color;
	W->window = subwin(parent, rows, cols, y, x);
	CleanWin(W, bo);
	if (delay == DELAY_OFF)
		nodelay(W->window, TRUE); // non-blocking reading of characters (for real-time game)
	wrefresh(W->window);
	return W;
}

void print_large_string(win &window, Vector<char *> *art, int x, int y, int color)
{
	int size = vector_size(art);
	wattron(window.window, COLOR_PAIR(color));
	for (int i = 0; i < size; i++)
	{
		mvwprintw(window.window, y + i, x, "%s", vector_get(art, i));
	}
	wattroff(window.window, COLOR_PAIR(color));
}

frog setup_frog(game_model &game)
{
	frog frog;
	frog.x = getmaxx(game.playwin->window) / 2; // start in the middle
	frog.y = getmaxy(game.playwin->window) - 4; // start at the bottom of the window
	vector_push_back(frog.art, (char *)" @.@ ");
	vector_push_back(frog.art, (char *)"( - )");
	vector_push_back(frog.art, (char *)"/   \\");
	frog.can_move = 1;
	return frog;
}

void create_car(int x, int y, game_model &game)
{
	car car;
	car.y = y;
	car.x = x;

	int car_type = 1 + rand() % 2;
	car.last_frame = clock();
	car.move_time = (1.0 + rand() % 3) / 40;

	switch (car_type)
	{
	case SMALL_CAR:
		car.art_size = 5;
		vector_push_back(car.art, (char *)"____ ");
		vector_push_back(car.art, (char *)"|   \\");
		vector_push_back(car.art, (char *)"o---o");
		break;
	case BIG_CAR:
		car.art_size = 10;
		vector_push_back(car.art, (char *)"_________ ");
		vector_push_back(car.art, (char *)"|    |   \\");
		vector_push_back(car.art, (char *)"o---o---oo");
		break;
	};

	vector_push_back(game.cars, car);
}

int handle_input(game_model &game)
{
	frog *frog = &game.frog;
	game.last_inp = getch(); // Non-blocking input

	if (frog->can_move == 1)
	{
		switch (game.last_inp)
		{
		case ERR: // No input detected
			return 1;
		case KEY_UP:
			if (frog->y > 2)
			{
				frog->y -= 1;
				frog->can_move = 0;
			}
			else{
				game.frog = setup_frog(game); // reset frog if reached the end
			}
			break;
		case KEY_DOWN:
			if (frog->y < getmaxy(game.playwin->window) - 4)
			{
				frog->y += 1;
				frog->can_move = 0;
			}
			break;
		case KEY_RIGHT:
			if (frog->x < getmaxx(game.playwin->window) - 6)
			{
				frog->x += 1;
				frog->can_move = 0;
			}
			break;
		case KEY_LEFT:
			if (frog->x > 1)
			{
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

void show_timer(game_model &game)
{
	CleanWin(game.statwin, 1);
	mvwprintw(game.statwin->window, 1, 10, "Time: %.2f", game.time);
}

void refresh_both(game_model &game)
{
	wrefresh(game.playwin->window);
	wrefresh(game.statwin->window);
}

void game_over(int ending, game_model game, WINDOW *win)
{
	wclear(win);
	const char *over_text = "GAME OVER";

	switch (ending)
	{
	case RAN_OUT_OF_TIME_ENDING:
		mvwaddstr(win, getmaxy(win) / 2 + 1, (getmaxx(win) / 2) - (strlen("You ran out of time!") / 2), "You ran out of time!");
		break;
	case GOT_HIT_ENDING:
		mvwaddstr(win, getmaxy(win) / 2 + 1, (getmaxx(win) / 2) - (strlen("Got ran over by a car :(") / 2), "Got ran over by a car :(");
		break;
	};

	mvwaddstr(win, getmaxy(win) / 2, (getmaxx(win) / 2) - (strlen(over_text) / 2), over_text);



	refresh();
}

void streets_and_plains(game_model &game)
{
	int  max_y = getmaxy(game.playwin->window);
	int  max_x = getmaxx(game.playwin->window);
	for (int i = max_y - 10; i > 3; i -= 9)
	{
		for (int j = 1; j < max_x - 1; j++)
		{
			if (i - 4 > 0)
			{ // game ends here
				wattron(game.playwin->window, COLOR_PAIR(PLAIN_COLOR));
				mvwprintw(game.playwin->window, i - 1, j, " ");
				mvwprintw(game.playwin->window, i - 2, j, " ");
				mvwprintw(game.playwin->window, i - 3, j, " ");
				wattroff(game.playwin->window, COLOR_PAIR(PLAIN_COLOR));
			}
		}
	}
}

void find_roads(game_model &game){
	int max_y = getmaxy(game.playwin->window);
	for(int i = max_y-4; i>=5; i-=6){
		vector_push_back(game.street_corners, i-3);
		vector_push_back(game.street_corners, i-6);
		i-=3;
	}
}

void create_cars_for_roads(game_model &game){
	for(int i = 0; i < vector_size(game.street_corners); i++){
		create_car(1, vector_get(game.street_corners, i), game);
	}
}

void game_board(game_model &game)
{
	WINDOW *game_win = game.playwin->window;
	int max_y = getmaxy(game_win);
	int max_x = getmaxx(game_win);
	int start_dest[6] = {1, 2, 3, max_y - 2, max_y - 3, max_y - 4};



	for (int i = 0; i < max_y; i++)
	{
		if (i == 0 || i == max_y - 1)
		{
			for (int j = 0; j < max_x; j++)
			{
				mvwaddch(game_win, i, j, '*');
			}
		}
		else
		{
			mvwprintw(game_win, i, 0, "*");
			mvwprintw(game_win, i, max_x - 1, "*");
		}
	}

	streets_and_plains(game);

	wattron(game_win, COLOR_PAIR(PLAIN_COLOR));
	for (int i = 0; i < 6; i++)
	{
		for (int x = 1; x < max_x - 1; x++)
		{
			mvwprintw(game_win, start_dest[i], x, " ");
		}
	}
	wattroff(game_win, COLOR_PAIR(PLAIN_COLOR));
}

car get_car(game_model &game,int i){
	return vector_get(game.cars, i);
}

void check_cars_and_move(game_model &game){
    int max_x = getmaxx(game.playwin->window);
    for(int i = 0; i < vector_size(game.cars); i++){
        clock_t now = clock();
        car *curr_car = vector_get_pointer(game.cars, i);
        double elapsed_time = (double)(now - curr_car->last_frame) / CLOCKS_PER_SEC;
        printw("autko w cornery: %d x: %d",curr_car->y, curr_car->x);
        if(elapsed_time >= curr_car->move_time){
            if(curr_car->x+curr_car->art_size+1 <= max_x){
                curr_car->x = curr_car->x+1;
                curr_car->last_frame = now;
            }
            else{
                curr_car->x = 1;
            }

        }
    }

}

int main_game_loop(game_model &game)
{
	CleanWin(game.playwin, 0);
	game_board(game);
	print_large_string(*game.playwin, game.frog.art, game.frog.x, game.frog.y, FROG_COLOR);

	for(int i = 0; i < vector_size(game.cars); i++){
		car car = get_car(game, i);
		print_large_string(*game.playwin, car.art, car.x, car.y, MAIN_COLOR);
	}

	show_timer(game);

	clock_t now = clock();
	double elapsed_time = (double)(now - game.last_frame) / CLOCKS_PER_SEC;
	check_cars_and_move(game);
	refresh_both(game);
	if (elapsed_time >= FRAME_TIME)
	{
		game.time -= FRAME_TIME;
		if (game.time <= 0)
		{
			game.ending = RAN_OUT_OF_TIME_ENDING;
			return -1;
		}

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
	srand(time(0));
	WINDOW *main_window = Start();
	Welcome(main_window);
	int mid_x = getmaxx(main_window) / 2 / 2;
	int mid_y = getmaxy(main_window) / 2 / 2 / 2;
	game.playwin = Init(main_window, game.window_y_size, game.window_x_size, mid_y, mid_x, MAIN_COLOR, 0, DELAY_OFF);
	wbkgd(game.playwin->window, COLOR_PAIR(MAIN_COLOR));
	game.statwin = Init(main_window, 3, game.window_x_size, game.window_y_size + mid_y, mid_x, STAT_COLOR, 1, DELAY_OFF);
	wrefresh(main_window);
	find_roads(game);
	create_cars_for_roads(game);
	game.frog = setup_frog(game);
	while (main_game_loop(game) != GAME_END_FLAG){};

	game_over(game.ending, game, main_window);

	refresh();	  /* Print it on to the real screen */
	while(getch() == ERR); /* Wait for user input */
	endwin();	  /* End curses mode		  */
}
