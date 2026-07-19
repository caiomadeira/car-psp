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
    {  1, "CROSS",          true },
    {  2, "DL",             true },
    {  3, "DLR",            true },
    {  4, "DR",             true },
    {  5, "LR",             true },
    {  7, "UD",             true },
    {  8, "UDL",            true },
    {  9, "UDR",            true },
    { 10, "UL",             true },
    { 11, "ULR",            true },
    { 12, "UR",             true },
    { 13, "sand",           true },
    { 14, "sidewalk",       true },
    { 15, "sidewalk_beach", true },
    { 16, "asphalt",        true },
};

#define NUM_ENV_TEX (int)(sizeof(TEX_TABLE)/sizeof(TEX_TABLE[0]))
#define MAX_MAP_CODE 64 //maior mapcode possivel acima disso os indices adicionados sao tratados como invalidos (-1)

#define MAPCODE_ASPHALT 16 // p ser usado sem percorrer o enum

// carrega todas as texturas da tabela, chama uma vez no loading
int LoadAllEnvironmentTextures();
int TexSlotFromMapCode(int mapCode);
PspTexture& EnvTexture(int slot);

#endif