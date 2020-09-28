/* for determine winner */

#include <stdbool.h>
#include <vector>
using namespace std; 


/* save available slot */
class Slot
{
public:
	int x; // x coordinate
	int y; // y coordinate
	int x_2; // x-2 coordinate
	int y_2; // y-2 coordinate
};


/* define cell class to save each cell and connected neighbors */
class Cell
{
public:
	int color;
	int visit; /* 0 means unvisit, 1 means visited */
	int i,j; /* piece position */
	vector<Cell*> neighbor;
};