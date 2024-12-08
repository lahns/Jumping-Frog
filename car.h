#ifndef CAR_H
#define CAR_H

#include <time.h>
#include "vector.h"

struct car
{
	int art_size;
	Vector<char *> *art;
	int x;
	int y;
	float move_time;
	int type;
	clock_t last_frame;
	int color;
    short does_chng_speed; // does the car change speed
	short can_collide; // 0 no, 1 yes
	short is_friendly; // 0 no, 1 yes
	short does_stop;   // 0 no, 1 yes
};

void give_car_color(car& car);

void give_car_random_speed(car& car);

void give_car_random_type(car& car);

void roll_if_friendly(car& car);

void roll_if_stops(car& car);

void roll_if_speed_changes(car& car);

void regen_car(car *car_to_regen);

#endif

