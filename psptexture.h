#ifndef PSP_TEXTURE_H
#define PSP_TEXTURE_H

#include "common.h"
#include "texture.h"

struct TexDef {
    int mapCode; // numero da celula no mapa.txt
    const char* file; // arquivo de textura em assets/tex/
    bool mipmap;
};

static const TexDef TEX_TABLE[] = {
    {  0, "concret1",          true },
    {  1, "concret1",             true },
    {  2, "concret3",             true },
    { 13, "sand",           true },
    { 14, "concret2",       true }, //sidewalk
    { 15, "sidewalk_beach", true },
};

#define NUM_ENV_TEX (int)(sizeof(TEX_TABLE)/sizeof(TEX_TABLE[0]))
#define MAX_MAP_CODE 64 //maior mapcode possivel acima disso os indices adicionados sao tratados como invalidos (-1)

#define MAPCODE_CONCRETE_DEFAULT 0 // p ser usado sem percorrer o enum

// carrega todas as texturas da tabela, chama uma vez no loading
int LoadAllEnvironmentTextures();
int TexSlotFromMapCode(int mapCode);
PspTexture& EnvTexture(int slot);

#endif