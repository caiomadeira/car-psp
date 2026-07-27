#ifndef PROPS_H
#define PROPS_H

#include "common.h"
#include "map.h"
#include "object3d.h"
#include "texture.h"

#define PROPS_TEXTURE_PATH  "assets/tex/props-texture.raw"

enum PropModel {
    PROP_POST_LIGHT = 0,
    PROP_CHURRO_CAR,
    PROP_OUTDOOR_1,
    PROP_OUTDOOR_2,
    PROP_STREET_MENU,
    PROP_NEWSSTAND,
    PROP_SOCCER_BALL,
    PROP_TREE_1,
    PROP_BUSH_1,
    PROP_COUNT,
};

struct PropModelDef {
    const char* tri;
    const char* tex;
    float scale;
    float aspect;
    float offsetY;
    int billboard; //  billboard: altura = largura * aspect  
};

static const PropModelDef PROP_MODELS[PROP_COUNT] = {
    {"assets/tri/poste.tri", PROPS_TEXTURE_PATH, 0.15f, 1.5f, 0.0f, 0 },
    {"assets/tri/churros_car.tri", PROPS_TEXTURE_PATH, 0.15f, 1.0f, 0.05f, 0 },
    {"assets/tri/outdoor_pepsi.tri", PROPS_TEXTURE_PATH, 0.25f, 1.0f, 0.0f, 0 },
    {"assets/tri/outdoor_psp.tri", PROPS_TEXTURE_PATH, 0.25f, 1.0f, 0.0f, 0 },
    {"assets/tri/cardapio_rua.tri", PROPS_TEXTURE_PATH, 0.25f, 1.0f, 0.0f, 0 },
    {"assets/tri/banca.tri", PROPS_TEXTURE_PATH, 0.15f, 1.0f, 0.0f, 0 },
    {"assets/tri/ball.tri", PROPS_TEXTURE_PATH, 0.25f, 1.0f, 0.0f, 0 },
    // billboards
    { NULL, "assets/tex/tree1.raw",  0.55f, 1.7f, 1 },
    { NULL, "assets/tex/brush1.raw",  0.30f, 0.9f, 1 },
};

#define MAX_PROPS 256

struct PropInstance {
    short cellX, cellZ; // props sao instanciados em celulas da matriz (mapa)
    signed char ox, oz;
    unsigned char model;
    unsigned char rot;
};

extern PropInstance props[MAX_PROPS];
extern int propCount;
extern Objeto3D propModels[PROP_COUNT];
extern PspTexture propTex[PROP_COUNT];

extern float g_propPivotX[PROP_COUNT];
extern float g_propPivotZ[PROP_COUNT];
extern float g_propSizeX[PROP_COUNT];
extern int   g_propOk[PROP_COUNT];

extern PspTexture wallTex;
void LoadWallTexture(void);
void BuildBorderWall(void);
void DrawBorderWall(void);

void PopulateScatter(int tileTag, int propTag, int spacing, unsigned int seed);
void LoadPropModels(void);
void AddProp(int cellx, int cellz, int model, float ox, float oz, float rotDegree);
void DrawProps(void);
void PopulatePropsOnTile(int tileTag, int propTag, int repeatTimes, float side);
void ClearProps(void);

// referente ao fundo panoramico 2d
extern PspTexture backdropTex;
void LoadBackdrop(void);
void DrawBackdrop(float cameraAngleDegree);

#endif