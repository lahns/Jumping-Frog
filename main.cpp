#include <cstring>
#include <ncurses.h>
#include <cstdio>
#include "vector.cpp"

#define SIZE_PARAMETER "sizexy"

struct game_model{
    int window_x_size;
    int window_y_size;
};

void read_game_model(game_model &game){
    FILE *fptr; // pointer to a file
    char parameter[100];
    int x_size, y_size;
    fptr = fopen("game_model.txt", "r");
    if(fscanf(fptr, "%s %d %d", parameter, &x_size, &y_size) == 3){
        if(strcmp(SIZE_PARAMETER, parameter) == 0){
            game.window_x_size = x_size;
            game.window_y_size = y_size;
        }
    }

}

void init_ncurses(){
initscr();
noecho();
clear();
keypad(stdscr, TRUE);
}

int main()
{
    Vector<float> *cars = vector_init<float>();
    vector_push_back(cars, 3.4f);
    vector_push_back(cars, 5.4f);
    game_model game;
    read_game_model(game);
	init_ncurses();		/* Start curses mode 		  */
	char ch;
	printw("Type any character to see it in bold\n");
	ch = getch();			/* If raw() hadn't been called
						 * we have to press enter before it
						 * gets to the program 		*/
		if(ch == KEY_F(1))		/* Without keypad enabled this will */
			printw("F1 Key pressed");/*  not get to us either	*/
						/* Without noecho() some ugly escape
						 * charachters might have been printed
						 * on screen			*/
		else
		{
			attron(A_BOLD);
			printw("%d %d", game.window_x_size, game.window_y_size);
			attroff(A_BOLD);
			for(int i=0; i<vector_size(cars);i++){
			printw("%f\t", cars->data[i]);
			refresh();
			}
		}
		refresh();			/* Print it on to the real screen */
    	getch();			/* Wait for user input */
		endwin();			/* End curses mode		  */

}
