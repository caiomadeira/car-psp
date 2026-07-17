#include "map.h"
#include <stdio.h>

int mapWidth = 0;
int mapHeight = 0;

int mapData[MAP_MAX][MAP_MAX];


bool LoadMap(const char *filename)
{
    FILE *f = fopen(filename,"r");
    if(!f)
        return false;

    if (fscanf(f, "%d %d", &mapWidth, &mapHeight) != 2)
    {
        fclose(f);
        return false;
    }

    for(int z=0;z<mapHeight;z++)
    {
        for(int x=0;x<mapWidth;x++)
        {
            if (fscanf(f,"%d",&mapData[z][x]) != 1)
            {
                fclose(f);
                return false;
            }
        }
    }

    fclose(f);
    return true;
}