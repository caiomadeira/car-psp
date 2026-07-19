#ifndef HUD_H
#define HUD_H

#include "common.h"
#include "psptexture.h"
#include <pspgu.h>
#include <pspgum.h>

// tem q bater com o --cell do make_font.py
#define FONT_CELL 16

struct Vertex2D {
    float u, v;
    float x, y, z;
};

extern PspTexture fontTex;
extern PspTexture bgTex;

// apenas chama no loading
void InitHUD(void);

void Begin2D(void);

void End2D(void);

void DrawFullScreen(const PspTexture& tex);
// desenha UM caractere na posicao (px, py) da tela. Devolve a largura avancada.
float DrawChar(char c, float px, float py, float scale, unsigned int color);
// desenha uma string. Retorna a largura total.
float DrawText2D(const char* s, float px, float py, float scale, unsigned int color);


#endif