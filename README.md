### Técnicas de Culling

O meu codigo de draw building tem a matriz sendo criada a CADA FRAME. Os predios
sao desenhados a cada loop.

```
void DrawBuildingsAndStreets(float scaleBuildingA, float scaleBuildingB, float scaleBuildingC)
{
    for (int z = 0; z < mapHeight; z++)
    {
        for (int x = 0; x < mapWidth; x++)
        {
            if (mapData[z][x] == 0)
            {
                int seed = (x * 13) + (z * 17);
                int buildingType = seed % 3;
                float rotY = (float)((seed % 4) * 90);

                if (buildingType == 0) DrawBuilding(buildingAModel, (float)x, (float)z,scaleBuildingA, rotY);
                else if (buildingType == 1) DrawBuilding(buildingBModel, (float)x, (float)z, scaleBuildingB, rotY);
                else DrawBuilding(buildingCModel, (float)x, (float)z, scaleBuildingC, rotY);
            }
            else if (mapData[z][x] >= 1 && mapData[z][x] <= 12) // eh rua 
            {
                DrawFloor((float)x, (float)z, mapData[z][x]);
            }
        }
    }
}
```

No caso, a boa é usar técnicas de culling.
Culling (descarte, literalmente) é a tecnica de nao processar e nao desenhar o que o jogador nao esta vendo. Se temos um mapa de 50x50 e o player olha pro norte, a GPU nao deve se esforçar pra calcular e desenhar o cenario nas costas do player ou muito distantes. 

O culling acontece antes da renderização (na CPU). Ele corta o objeto inteiro da cena. Diferentemente do zbuffer (que trabalha em conjunto com tecnicas de culling). O z-buffer acontece durante a renderização na GPU (por pixel). Se existe dois items atras de um outro objeto e eles passaram pelo Culling, a GPU anota a profundidade do eixo Z de cada pixel. 

PS: GE = Graphics Engine do PSP (A GPU).