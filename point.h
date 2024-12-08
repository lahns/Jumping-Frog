#ifndef POINT_H
#define POINT_H

#include <math.h>

struct point
{
	int x;
	int y;
};

point create_point(int x, int y)
{
	point new_point;
	new_point.x = x;
	new_point.y = y;
	return new_point;
}

double distance_of_two_points(point a, point b)
{
	return sqrt(pow((b.x - a.x), 2) + pow((b.y - a.y), 2));
}

#endif