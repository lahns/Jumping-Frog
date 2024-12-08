#include "constants.h"
#include "car.h"

void give_car_color(car &car)
{
    int color = (1 + rand() % 3);
    switch (color)
    {
    case 1:
        car.color = CAR_COLOR_1;
        break;
    case 2:
        car.color = CAR_COLOR_2;
        break;
    case 3:
        car.color = CAR_COLOR_3;
        break;
    };
}

void give_car_random_speed(car &car)
{
    car.move_time = (1.0 + rand() % 3) / 40;
}

void give_car_random_type(car &car)
{
    int car_type = 1 + rand() % 2;
    car.art = vector_init<char *>();
    switch (car_type)
    {
    case SMALL_CAR:
        car.art_size = 5;
        vector_push_back(car.art, (char *)"____ ");

        if (car.does_stop)
        {
            vector_push_back(car.art, (char *)"| S \\");
        }
        else if (car.is_friendly)
        {
            vector_push_back(car.art, (char *)"| F \\");
        }
        else
        {
            vector_push_back(car.art, (char *)"|   \\");
        }

        vector_push_back(car.art, (char *)"o---o");
        break;
    case BIG_CAR:
        car.art_size = 10;
        vector_push_back(car.art, (char *)"_________ ");
        if (car.does_stop)
        {
            vector_push_back(car.art, (char *)"|SSSS|   \\");
        }
        else if (car.is_friendly)
        {
            vector_push_back(car.art, (char *)"|FFFF|   \\");
        }
        else
        {
            vector_push_back(car.art, (char *)"|    |   \\");
        }
        vector_push_back(car.art, (char *)"o---o---oo");
        break;
    };
}

void roll_if_friendly(car &car)
{
    int chance = rand() % 101; // 0-90 not friendly 91-100 friendly
    if (chance < 91)
    {
        car.is_friendly = 0;
        car.can_collide = 1;
        car.does_stop = 0;
    }
    else
    {
        car.is_friendly = 1;
        car.can_collide = 0;
        car.does_stop = 0;
    }
}

void roll_if_stops(car &car)
{
    int chance = rand() % 101; // 0-85 doesnt stop 86-100 stops
    if (chance < 86)
    {
        car.does_stop = 0;
        car.can_collide = 1;
        car.is_friendly = 0;
    }
    else
    {
        car.does_stop = 1;
        car.can_collide = 0;
        car.is_friendly = 0;
    }
}

void roll_if_speed_changes(car &car)
{
    int chance = rand() % 101;
    if (chance < 81)
    {
        car.does_chng_speed = 0;
    }
    else
    {
        car.does_chng_speed = 1;
    }
}

void regen_car(car *car_to_regen)
{
	car &new_car = *car_to_regen;
	roll_if_stops(new_car);
	if (new_car.does_stop == 0)
	{ // roll if friendly only if its not a stopping car
		roll_if_friendly(new_car);
	}
	give_car_color(new_car);
	give_car_random_speed(new_car);
	vector_free<char *>(car_to_regen->art);
	give_car_random_type(new_car);
}
