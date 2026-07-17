#ifndef MAP_H
#define MAP_H

#define MAP_MAX 64

extern int mapWidth;
extern int mapHeight;
extern int mapData[MAP_MAX][MAP_MAX];

bool LoadMap(const char *filename);


#endif