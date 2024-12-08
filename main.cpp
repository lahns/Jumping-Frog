/*
I've used the following resouces in spite of writing this project:
-Example demo game "Catch the ball" by professor Małafiejski
-https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
-https://learnopelgl.com/In-Practice/2D-Game/Collisions/Collision-detecion
*/

#include <cmath>
#include <cstring>
#include <ctime>
#include <ncurses.h>
#include <cstdio>
#include <time.h>
#include <math.h>
#include "constants.h"
#include "vector.h"
#include "car.h"
#include "point.h"

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
	short is_near_f_car; // is near a friendly car 0 means no, 1 means yes
	short is_on_car;	 // is frog travelling on a friendly car? 0 means no, 1 means yes
	int car_to_bind;	 // friendly car index, that the frog can bound with
	int f_pressed = 0;
};

struct game_model
{
	int window_x_size;
	int window_y_size; // 5+9*n size min size 32
	float time;
	frog frog;
	clock_t last_frame;
	win *playwin;
	win *statwin;
	int last_inp;
	int ending;
	Vector<car> *cars = vector_init<car>();
	Vector<int> *street_corners = vector_init<int>();
	Vector<point> *bush_corners = vector_init<point>();
	int score = 0;
};

/*
FUNCTIONS THAT SETUP CRUCIAL OBJECTS
*/

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
	else
	{
		game.window_x_size = 101;
		game.window_y_size = 32;
	}
	if (fscanf(fptr, "%s %f", parameter, &time) == 2)
	{
		if (strcmp(TIME_PARAMETER, parameter) == 0)
		{
			game.time = time;
		}
	}
	else
	{
		game.time = 60;
	}
}

frog setup_frog(game_model &game)
{
	frog frog;
	frog.x = getmaxx(game.playwin->window) / 2 - 1; // start in the middle
	frog.y = getmaxy(game.playwin->window) - 4;		// start at the bottom of the window
	vector_push_back(frog.art, (char *)" @.@ ");
	vector_push_back(frog.art, (char *)"( - )");
	vector_push_back(frog.art, (char *)"/   \\");
	frog.is_near_f_car = 0;
	frog.is_on_car = 0;
	frog.can_move = 1;
	return frog;
}

void create_car(int x, int y, game_model &game)
{
	car car;
	car.y = y;
	car.x = x;

	car.last_frame = clock();
	roll_if_stops(car);
	if (car.does_stop == 0)
	{ // roll if friendly only if its not a stopping car
		roll_if_friendly(car);
	}
	roll_if_speed_changes(car);
	give_car_random_speed(car);
	give_car_random_type(car);
	give_car_color(car);
	vector_push_back(game.cars, car);
}

/*
FUNCTIONS THAT SETUP NCURSES, GAME WINDOWS AND HANDLE THE MENU AND THE GAME OVER SCREEN
*/
void CleanWin(win *W, int bo) // bo(rder): 0 | 1
{
	werase(W->window);
	wattron(W->window, COLOR_PAIR(W->color));
	if (bo)
		box(W->window, 0, 0);
}

WINDOW *Start()
{
	WINDOW *win;

	if ((win = initscr()) == NULL)
	{ // initialize ncurses
		fprintf(stderr, "Error initialising ncurses.\n");
		exit(EXIT_FAILURE);
	}

	start_color(); // initialize colors
	init_pair(FROG_COLOR, COLOR_GREEN, COLOR_BLACK);
	init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
	init_pair(STAT_COLOR, COLOR_CYAN, COLOR_BLACK);
	init_pair(PLAIN_COLOR, COLOR_BLACK, COLOR_GREEN);
	init_pair(CAR_COLOR_1, COLOR_BLACK, COLOR_MAGENTA);
	init_pair(CAR_COLOR_2, COLOR_BLACK, COLOR_YELLOW);
	init_pair(CAR_COLOR_3, COLOR_BLACK, COLOR_WHITE);
	init_pair(BUSH_COLOR, COLOR_GRAY, COLOR_GREEN);
	noecho();
	nodelay(win, DELAY_ON);
	keypad(win, true);
	cbreak(); // Switch off echoing, turn off cursor
	curs_set(0);
	return win;
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

void show_score_bar(game_model &game) 
{
	WINDOW *statwin = game.statwin->window;
	int max_x = getmaxx(statwin);
	CleanWin(game.statwin, 1);
	mvwprintw(statwin, 1, 5, "Time: %.2f", game.time);
	mvwprintw(statwin, 1, (max_x - strlen(AUTHOR)) / 2, AUTHOR);
	mvwprintw(statwin, 1, max_x - 10, "Score: %d", game.score);
}

void refresh_both(game_model &game) //refreshes both game main windows
{
	wrefresh(game.playwin->window);
	wrefresh(game.statwin->window);
}


void Welcome(WINDOW *win) // Welcome screen : press any key to continue
{
	init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
	mvwaddstr(win, getmaxy(win) / 2, (getmaxx(win) / 2) - (strlen(WELCOME) / 2), WELCOME);
	mvwaddstr(win, getmaxy(win) / 2 + 1, (getmaxx(win) / 2) - (strlen(TIP) / 2), TIP);
	while (wgetch(win) == ERR);
	wclear(win); // clear (after next refresh)
	wrefresh(win);
}

void game_over(game_model game, WINDOW *win) // End screen : press any key to continue
{
	wclear(win);
	char score_text[1001];
	sprintf(score_text, "Your final score: %d", game.score);
	switch (game.ending)
	{
	case RAN_OUT_OF_TIME_ENDING:
		mvwaddstr(win, getmaxy(win) / 2 + 1, (getmaxx(win) / 2) - (strlen(RAN_OF_TIME) / 2), RAN_OF_TIME);
		break;
	case GOT_HIT_ENDING:
		mvwaddstr(win, getmaxy(win) / 2 + 1, (getmaxx(win) / 2) - (strlen(GOT_HIT) / 2), GOT_HIT);
		break;
	};

	mvwaddstr(win, getmaxy(win) / 2, (getmaxx(win) / 2) - (strlen(OVER_TEXT) / 2), OVER_TEXT);
	mvwprintw(win, getmaxy(win) / 2 + 2, (getmaxx(win) / 2) - (strlen(score_text) / 2), score_text);
	mvwprintw(win, getmaxy(win) / 2 + 3, (getmaxx(win) / 2) - (strlen(PRESS_TEXT) / 2), PRESS_TEXT);
	while (getch() != ' '){};
	refresh();
}


/*FUNCTIONS FOR COLLISION HANDLING*/

int check_car_collision(game_model &game)
{
	Vector<car> *cars = game.cars;
	int car_count = vector_size(cars);
	for (int i = 0; i < car_count; i++)
	{
		car curr_car = vector_get(cars, i);
		if (curr_car.can_collide)
		{
			short collision_x = game.frog.x + (game.frog.art_size - 1) >= curr_car.x &&
								curr_car.x + (curr_car.art_size - 1) >= game.frog.x;
			short collision_y = (game.frog.y + 2) >= curr_car.y &&
								(curr_car.y) + 2 >= game.frog.y;

			if (collision_x && collision_y)
			{
				game.ending = GOT_HIT_ENDING;
				return -1; // -1  meaning the frog got hit
			}
		}
	}

	return 0;
}

int check_bush_collision(game_model &game, point frog_xy)
{

	int bush_count = vector_size(game.bush_corners);
	for (int i = 0; i < bush_count; i++)
	{
		point curr_point = vector_get(game.bush_corners, i);
		short collision_x = frog_xy.x + (game.frog.art_size - 1) >= curr_point.x - 2 && // -2, frog art x is in frogs left-upper corner
							curr_point.x + 2 >= frog_xy.x;								// + 2, bushes always have width of 3 units
		short collision_y = (frog_xy.y + 2) >= curr_point.y &&							//  2, bushes always have height of 3 units
							curr_point.y + 2 >= frog_xy.y;

		if (collision_x && collision_y)
		{
			return 0; // 0  meaning frog will collide with a bush
		}
	}
	return 1;
}

void find_roads_and_bushes(game_model &game) // find y's coordinates for roads and (x,y) coordinates for bushes(static obstacles)
{
	int max_y = getmaxy(game.playwin->window);
	int max_x = getmaxx(game.playwin->window);
	for (int i = max_y - 4; i >= 5; i -= 6)
	{
		vector_push_back(game.street_corners, i - 3);
		vector_push_back(game.street_corners, i - 6);
		int bush_limit = 8;
		for (int j = 1; j < max_x; j += 10)
		{
			if (bush_limit > 0 && i < max_y - 4)
			{
				if (rand() % 2)
				{
					vector_push_back(game.bush_corners, create_point(j, i));
					bush_limit--;
				}
			}
		}

		i -= 3;
	}
}

/*
FUNCTIONS FOR BASIC PRINTING 
*/

void print_large_string(win &window, Vector<char *> *art, int x, int y, int color) //prints large string from char* vectors
{
	int size = vector_size(art);
	wattron(window.window, COLOR_PAIR(color));
	for (int i = 0; i < size; i++)
	{
		mvwprintw(window.window, y + i, x, "%s", vector_get(art, i));
	}
	wattroff(window.window, COLOR_PAIR(color));
}

void print_bushes(game_model &game) // prints bushes on game's board based off prefound bush corners
{
	int bush_count = vector_size(game.bush_corners);
	for (int i = 0; i < bush_count; i++)
	{
		point curr_bush = vector_get(game.bush_corners, i);
		wattron(game.playwin->window, COLOR_PAIR(BUSH_COLOR));
		for (int j = 0; j < 3; j++)
		{
			mvwprintw(game.playwin->window, curr_bush.y + j, curr_bush.x, "###");
		}
		wattroff(game.playwin->window, COLOR_PAIR(BUSH_COLOR));
	}
}

void streets_and_plains(game_model &game) // function that prints out streets and plains(safe places between streets where bushes can spawn)
{
	int max_y = getmaxy(game.playwin->window);
	int max_x = getmaxx(game.playwin->window);
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

void game_board(game_model &game) // print out the game board outline
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

/*FUNCTIONS CRUCIAL FOR THE GAME TO WORK
MOVING CARS, HANDLING INPUT, CORE OF THE GAME
*/

car get_car(game_model &game, int i) // util - get car from car vector inside game model
{
	return vector_get(game.cars, i);
}

void create_cars_for_roads(game_model &game) // util - creates and pushes cars to game's car vector based off prefound street corners
{
	for (int i = 0; i < vector_size(game.street_corners); i++)
	{
		create_car(1, vector_get(game.street_corners, i), game);
	}
}

void handle_stopping_car(car &car, frog &frog, clock_t now) // logic for dealing with stopping cars inside of check and move cars function
{
	point car_p = create_point(car.x + car.art_size / 2, car.y - 1); // middle of the car
	point frog_p = create_point(frog.x + 1, frog.y - 1);			 // middle of the frog
	double dist = distance_of_two_points(car_p, frog_p);
	if (car.does_stop && dist >= 10)
	{
		car.x++;
		car.last_frame = now;
	}
}

void handle_friendly_car(car &car, frog &frog, clock_t now, double &best_dist, int car_idx) // logic for dealing with friendly cars inside of check and move cars function
{

	car.x++;
	car.last_frame = now; // move the car first
	if (frog.car_to_bind == -1)
	{
		point car_p = create_point(car.x + car.art_size / 2, car.y - 1); // middle of the car
		point frog_p = create_point(frog.x + 1, frog.y - 1);			 // middle of the frog
		double dist = distance_of_two_points(car_p, frog_p);
		if (dist <= 5)
		{
			if (dist < best_dist)
			{
				frog.car_to_bind = car_idx;
				best_dist = dist;
			}
		}
	}
}

void check_cars_and_move(game_model &game) // crucial func that moves all game's cars when they should, and moves frog when travelling by friendly car
{
	frog &frog = game.frog;
	int max_x = getmaxx(game.playwin->window);
	if (frog.f_pressed == 0)
		frog.car_to_bind = -1;
	double best_distance = 10001;
	for (int i = 0; i < vector_size(game.cars); i++)
	{
		clock_t now = clock();
		car *curr_car = vector_get_pointer(game.cars, i);
		double elapsed_time = (double)(now - curr_car->last_frame) / CLOCKS_PER_SEC;

		if (elapsed_time >= curr_car->move_time) // if the time elapsed from the last car move is greater than cars cooldown - move
		{
			if (curr_car->x + curr_car->art_size + 1 <= max_x) // if car reached max windows x, wrap
			{
				if (curr_car->does_chng_speed)
				{
					give_car_random_speed(*curr_car);
				}

				if (curr_car->does_stop)
					handle_stopping_car(*curr_car, frog, now);
				else if (curr_car->is_friendly)
					handle_friendly_car(*curr_car, game.frog, now, best_distance, i);
				else
				{
					curr_car->x++;
					curr_car->last_frame = now;
				}
			}
			else
			{
				curr_car->x = 1; // if car reaches the border reset its position to the beginning of its road
				if (i != frog.car_to_bind)
				{									  // dont regen the friendly car, that frog is travelling ing
					short should_reroll = rand() % 2; // pseudorandom number in range [0,1]
					if (should_reroll)
					{
						regen_car(curr_car); // reroll new parameters for a car
					}
				}
			}
		}
	}
	if (frog.is_on_car)
	{
		car best_car = get_car(game, frog.car_to_bind);
		frog.x = best_car.x;
		frog.y = best_car.y;
	}
}

void reset_frog_val(frog &frog) // helper function for input handling, resets a few variables that should be reset on any factual move
{
	frog.can_move = 0;
	frog.is_on_car = 0;
	frog.f_pressed = 0;
}

int handle_input(game_model &game) // handles all player input
{
	frog *frog = &game.frog;
	game.last_inp = getch(); // Non-blocking input

	if (frog->can_move == 1)
	{
		point next_pos;
		next_pos.x = frog->x;
		next_pos.y = frog->y;
		switch (game.last_inp)
		{
		case ERR: // No input detected
			return 1;
		case KEY_UP:
			if (frog->y > 2)
			{
				next_pos.y -= 1;
				if (check_bush_collision(game, next_pos))
				{
					frog->y -= 1;
					reset_frog_val(*frog);
				}
			}
			else
			{
				game.score += 1;
				game.frog = setup_frog(game); // reset frog if reached the end
			}
			break;
		case KEY_DOWN:
			if (frog->y < getmaxy(game.playwin->window) - 4)
			{
				next_pos.y += 1;
				if (check_bush_collision(game, next_pos))
				{
					frog->y += 1;
					reset_frog_val(*frog);
				}
			}
			break;
		case KEY_RIGHT:
			if (frog->x < getmaxx(game.playwin->window) - 6)
			{
				next_pos.x += 1;
				if (check_bush_collision(game, next_pos))
				{
					frog->x += 1;
					reset_frog_val(*frog);
				}
			}
			break;
		case KEY_LEFT:
			if (frog->x > 1)
			{
				next_pos.x -= 1;
				if (check_bush_collision(game, next_pos))
				{
					frog->x -= 1;
					reset_frog_val(*frog);
				}
			}
			break;
		case 'f':
			frog->f_pressed = 1;
			if (frog->car_to_bind != -1)
			{
				frog->is_on_car = 1;
				frog->can_move = 0;
			}
			break;
		case 'q': // Quit game
			return GAME_END_FLAG;
		}
	}
	return 1;
}

int main_game_loop(game_model &game)
{
	CleanWin(game.playwin, 0);
	game_board(game);

	for (int i = 0; i < vector_size(game.cars); i++)
	{
		car car = get_car(game, i);
		print_large_string(*game.playwin, car.art, car.x, car.y, car.color);
	}

	print_large_string(*game.playwin, game.frog.art, game.frog.x, game.frog.y, FROG_COLOR);

	show_score_bar(game);

	clock_t now = clock();
	double elapsed_time = (double)(now - game.last_frame) / CLOCKS_PER_SEC;
	check_cars_and_move(game);
	print_bushes(game);
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

	if (check_car_collision(game) == 0)
	{
		return handle_input(game);
	}
	else
	{
		return -1;
	}

	// Process input in every loop
}

void cleanup_game(game_model &game) //free the memory allocated for game's vectors and deletes windows created
{
	vector_free<car>(game.cars);
	vector_free<int>(game.street_corners);
	vector_free<point>(game.bush_corners);
	delwin(game.playwin->window);
	delwin(game.statwin->window);
}

int main()
{
	game_model game;
	read_game_model(game);
	srand((int)clock());
	WINDOW *main_window = Start();
	Welcome(main_window);
	int mid_x = getmaxx(main_window) / 2 - game.window_x_size / 2;
	int mid_y = getmaxy(main_window) / 2 / 2;
	game.playwin = Init(main_window, game.window_y_size, game.window_x_size, 0, mid_x, MAIN_COLOR, 0, DELAY_OFF);
	wbkgd(game.playwin->window, COLOR_PAIR(MAIN_COLOR));
	game.statwin = Init(main_window, 3, game.window_x_size, game.window_y_size + 0, mid_x, STAT_COLOR, 1, DELAY_OFF);
	wrefresh(main_window);
	find_roads_and_bushes(game);
	create_cars_for_roads(game);
	game.frog = setup_frog(game);
	while (main_game_loop(game) != GAME_END_FLAG)
	{
	};
	game_over(game, main_window);
	refresh();
	
	cleanup_game(game);
	endwin();
}
