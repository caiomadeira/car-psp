#include "hud.h"

PspTexture fontTex; 
PspTexture bgTex;

void InitHUD(void) {
    LoadTexturePsp("assets/font/font.raw", fontTex, false); 
    LoadTexturePsp("assets/img/test_bg.raw", bgTex, false);
}

void Begin2D(void) {
    sceGuDisable(GU_DEPTH_TEST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumOrtho(0, 480, 272, 0, -1, 1);
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    sceGuTexOffset(0.0f, 0.0f);
    sceGuTexScale(1.0f, 1.0f);
}

void End2D(void) { 
    sceGuEnable(GU_DEPTH_TEST); 
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_ALPHA_TEST); 
}

void DrawFullScreen(const PspTexture& tex) {
    Vertex2D* v = (Vertex2D*)sceGuGetMemory(2 * sizeof(Vertex2D));
    v[0] = (Vertex2D){ 0.0f, 0.0f,   0,   0, 0 };
    v[1] = (Vertex2D){ 1.0f, 1.0f, 480, 272, 0 };   // 0-1 = textura inteira

    UseTexturePsp(tex);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
    sceGumDrawArray(GU_SPRITES,
        GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 2, 0, v);
}

float DrawChar(char c, float px, float py, float scale, unsigned int color)
{
    int col = (unsigned char)c % 16;
    int row = (unsigned char)c / 16;
    float cellUV = (float)FONT_CELL / (float)fontTex.width;   // celula em 0-1
    float u0 = col * cellUV, v0 = row * cellUV;
    float u1 = u0 + cellUV, v1 = v0 + cellUV;
    float sz = FONT_CELL * scale;

    Vertex2D* v = (Vertex2D*)sceGuGetMemory(2 * sizeof(Vertex2D));
    v[0] = (Vertex2D){ u0, v0, px,      py,      0 };
    v[1] = (Vertex2D){ u1, v1, px + sz, py + sz, 0 };

    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuColor(color);
    sceGumDrawArray(GU_SPRITES,
        GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 2, 0, v);

    return sz * 0.55f;
}

float DrawText2D(const char* s, float px, float py, float scale, unsigned int color)
{
    UseTexturePsp(fontTex);
    float x = px;
    for (; *s; s++) {
        if (*s == ' ') { x += FONT_CELL * scale * 0.4f; continue; }
        x += DrawChar(*s, x, py, scale, color);
    }
    return x - px;
}
