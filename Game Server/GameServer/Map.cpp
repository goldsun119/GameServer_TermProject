#include "Server.h"
#include "2019_텀프_protocol.h"


int map[WORLD_WIDTH][WORLD_HEIGHT]{ 0 };
int Tile1[WORLD_WIDTH][WORLD_HEIGHT]{ 0 };
int Tile2[WORLD_WIDTH][WORLD_HEIGHT]{ 0 };

void read_map() {
	FILE *pFile = NULL;
	int x = 0, y = 0;
	pFile = fopen( "CollisionCheck_Data\\Tile1.txt", "r" );

	while ( !feof( pFile ) ) { 
		fscanf( pFile, "%d ", &map[x][y] );
		x += 1;
		if ( x >= WORLD_WIDTH ) {
			x = 0;
			y += 1;
		}
	}
	fclose( pFile );
	x = 0, y = 0;
	pFile = fopen( "CollisionCheck_Data\\Tile2.txt", "r" );

	while ( !feof( pFile ) ) {
		fscanf( pFile, "%d ", &Tile1[x][y] );
		x += 1;
		if ( x >= WORLD_WIDTH ) {
			x = 0;
			y += 1;
		}
	}
	fclose( pFile );
	x = 0, y = 0;
	pFile = fopen( "CollisionCheck_Data\\Tile3.txt", "r" );

	while ( !feof( pFile ) ) {
		fscanf( pFile, "%d ", &Tile2[x][y] );
		x += 1;
		if ( x >= WORLD_WIDTH ) {
			x = 0;
			y += 1;
		}
	}
	fclose( pFile );

	std::cout << "Map Load Complete!" << std::endl;
}

bool CollisionCheck( int ci, int dir ) {
	int m_x = g_clients[ci].x, m_y = g_clients[ci].y;
	switch ( dir ) {
	case 0:
		m_y--;
		break;
	case 1:
		m_y++;
		break;
	case 2:
		m_x--;
		break;
	case 3:
		m_x++;
		break;
	case CS_NPC:
		break;
	default:
		return false;
		break;
	}

	if ( high_resolution_clock::now() - g_clients[ci].last_move_time <= 1s && g_clients[ci].connect == true && MoveHACK == FALSE ) {
		return false;
	}

	// 272, 68, 191, 13, 14, 43, 44, 752
	if ( map[m_x][m_y] == 152  ) {
		//printf( "%d 충돌0\t", map[m_x][m_y] );
		return false;
	}

	if ( Tile1[m_x][m_y] != 0 ) {
		//printf( "%d 충돌1\t", Tile1[m_x][m_y] );
		return false;
	}

	if ( Tile2[m_x][m_y] != 0 ) {
		//printf( "%d 충돌2\t", Tile2[m_x][m_y] );
		return false;
	}
	else {
		return true;
	}

}