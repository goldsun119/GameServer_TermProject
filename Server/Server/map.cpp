#include "server.h"

int ground[WORLD_WIDTH][WORLD_HEIGHT]{ 0 };
int obstacle[WORLD_WIDTH][WORLD_HEIGHT]{ 0 };

void read_map()
{
	FILE *pFile = NULL;
	int x = 0, y = 0;
	pFile = fopen("map_data\\ground.txt", "r");
	while (!feof(pFile))
	{
		fscanf(pFile, "%d ", &ground[x][y]);
		x += 1;
		if (x >= WORLD_WIDTH)
		{
			x = 0;
			y += 1;
		}
	}
	fclose(pFile);

	x = 0, y = 0;
	pFile = fopen("map_data\\obstacle.txt", "r");
	while (!feof(pFile))
	{
		fscanf(pFile, "%d ", &obstacle[x][y]);
		x += 1;
		if (x >= WORLD_WIDTH)
		{
			x = 0;
			y += 1;
		}
	}
		fclose(pFile);

		cout << "map data is loaded" << endl;
}

bool collision_check(int id, int dir)
{
	int tx = clients[id].x, ty = clients[id].y;
	switch (dir)
	{//0:Up, 1 : Down, 2 : Left, 3 : Right
	case 0:
		ty--;
		break;
	case 1:
		ty++;
		break;
	case 2:
		tx--;
		break;
	case 3:
		tx++;
		break;
	default:
		break;
	}

	if (obstacle[tx][ty] != 0)
	{
		return true;
	}
	return false;
}