// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

class Point
{
private:
	int x, y;
	int iteration = -1;
public:
	Point() {}
	Point(int xIn, int Yin) : x(xIn), y(Yin) {}
	Point(int xIn, int yIn, int iterationIn) : x(xIn), y(yIn), iteration(iterationIn) {}

	int getX() { return x; };
	int getY() { return y; };
	int getIteration() { return iteration; };
};
