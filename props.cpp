#include "props.h"

extern ScePspFVector3 g_eye;
extern float angleVehicle;
extern int g_drawCalls, g_trisDrawn;

PropInstance props[MAX_PROPS];
int propCount = 0;
Objeto3D propModels[PROP_COUNT];
PspTexture propTex[PROP_COUNT];
PspTexture backdropTex;
float g_propPivotX[PROP_COUNT];
float g_propPivotZ[PROP_COUNT];
float g_propSizeX[PROP_COUNT];
int   g_propOk[PROP_COUNT];

#ifndef VIEW_RADIUS
#define VIEW_RADIUS 14.0f
#endif
#ifndef COS_HALF_FOV
#define COS_HALF_FOV 0.30f
#endif

#define LCG_A 1664525u
#define LCG_C 1013904223u

#define WALL_SEG_W     4.0f 
#define WALL_HEIGHT    6.0f    
#define WALL_OUTSET    1.5f
#define MAX_WALL_SEGS  128

struct WallSeg { float x, z, rot; };
static WallSeg g_wall[MAX_WALL_SEGS];
static int g_wallCount = 0;

PspTexture wallTex;


static inline float PropOffset(signed char v) { return (float)v / 256.0f; }
static inline float PropRotationDegree(unsigned char r) { return (float)r * (360.0f/256.0f); }

void LoadWallTexture(void)
{
    if (!LoadTexturePsp("assets/tex/mountain.raw", wallTex, false))
        printf("AVISO: montain.raw nao carregou\n");
}

static void AddWallSeg(float x, float z, float rot)
{
    if (g_wallCount >= MAX_WALL_SEGS) return;
    g_wall[g_wallCount].x   = x;
    g_wall[g_wallCount].z   = z;
    g_wall[g_wallCount].rot = rot;
    g_wallCount++;
}

void BuildBorderWall(void)
{
    g_wallCount = 0;

    const float x0 = -0.5f - WALL_OUTSET;
    const float x1 = (float)mapWidth  - 0.5f + WALL_OUTSET;
    const float z0 = -0.5f - WALL_OUTSET;
    const float z1 = (float)mapHeight - 0.5f + WALL_OUTSET;

    /* norte e sul: varre em X */
    for (float x = x0; x < x1; x += WALL_SEG_W) {
        float cx = x + WALL_SEG_W * 0.5f;
        AddWallSeg(cx, z0,   0.0f);    /* borda -Z, olha para +Z */
        AddWallSeg(cx, z1, 180.0f);    /* borda +Z, olha para -Z */
    }
    /* leste e oeste: varre em Z */
    for (float z = z0; z < z1; z += WALL_SEG_W) {
        float cz = z + WALL_SEG_W * 0.5f;
        AddWallSeg(x0, cz,  90.0f);    /* borda -X, olha para +X */
        AddWallSeg(x1, cz, 270.0f);    /* borda +X, olha para -X */
    }
}

void DrawBorderWall(void)
{
    if (wallTex.nLevels <= 0 || wallTex.data[0] == NULL) return;

    float a = angleVehicle * M_PI / 180.0f;
    float fwdX = sinf(a), fwdZ = cosf(a);

    sceGuDisable(GU_CULL_FACE);          /* quad de face unica */
    UseTexturePsp(wallTex);

    for (int i = 0; i < g_wallCount; i++)
    {
        WallSeg* s = &g_wall[i];

        /* culling so por CONE: a parede precisa aparecer mesmo longe,
           senao o vazio que ela deveria tapar reaparece.
           O far plane ja limita o alcance. */
        float dx = s->x - g_eye.x, dz = s->z - g_eye.z;
        float d2 = dx*dx + dz*dz;
        if (d2 > 4.0f) {
            float inv = 1.0f / sqrtf(d2);
            /* cone mais largo que o dos props: a parede e comprida e
               sumir uma peca no canto da tela e muito visivel */
            if ((dx*fwdX + dz*fwdZ) * inv < 0.10f) continue;
        }

        struct VW { float u, v; unsigned int color; float x, y, z; };
        VW* v = (VW*)sceGuGetMemory(6 * sizeof(VW));
        if (!v) break;

        const unsigned int C = 0xFFFFFFFF;
        const float hw = WALL_SEG_W * 0.5f;
        const float h  = WALL_HEIGHT;

        /* base no chao (v=0), topo em v=1 -- mesma convencao do billboard */
        v[0].u=0; v[0].v=0; v[0].color=C; v[0].x=-hw; v[0].y=0; v[0].z=0;
        v[1].u=1; v[1].v=0; v[1].color=C; v[1].x= hw; v[1].y=0; v[1].z=0;
        v[2].u=1; v[2].v=1; v[2].color=C; v[2].x= hw; v[2].y=h; v[2].z=0;
        v[3] = v[0];
        v[4] = v[2];
        v[5].u=0; v[5].v=1; v[5].color=C; v[5].x=-hw; v[5].y=h; v[5].z=0;

        sceGumPushMatrix();
            ScePspFVector3 pos = { s->x, 0.0f, s->z };
            sceGumTranslate(&pos);
            sceGumRotateY(s->rot * M_PI / 180.0f);
            sceGumDrawArray(GU_TRIANGLES,
                GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                6, 0, v);
        sceGumPopMatrix();

        g_drawCalls++;
        g_trisDrawn += 2;
    }

    sceGuEnable(GU_CULL_FACE);
}

void LoadPropModels(void) {
    for (int i = 0; i < PROP_COUNT; i++) {
        const PropModelDef& d = PROP_MODELS[i];

        if (d.billboard) {
            // billboard: SO textura, sem malha 3D
            if (!LoadTexturePsp(d.tex, propTex[i], false))
                printf("AVISO: prop %d, textura %s nao carregou\n", i, d.tex);
        } else {
            g_propOk[i] = propModels[i].leObjeto(d.tri, 0xFFFFFFFF) ? 1 : 0;
            if (!g_propOk[i])
                printf("AVISO: prop %d, modelo %s nao carregou\n", i, d.tri);
            propModels[i].carregarTextura(d.tex);

            g_propPivotX[i] = propModels[i].getCenterX();
            g_propPivotZ[i] = propModels[i].getCenterZ();
            g_propSizeX[i]  = propModels[i].getSizeX();
        }
    }
    sceKernelDcacheWritebackInvalidateAll();   // UMA vez, FORA do laco
}

void ClearProps(void) { propCount = 0; }

void AddProp(int cellx, int cellz, int model, float ox, float oz, float rotDegree)
{
    if (propCount >= MAX_PROPS)
        return;
    
    if (model < 0 || model >= PROP_COUNT)
        return;
    
    PropInstance* p = &props[propCount++];
    p->cellX = (short)cellx;
    p->cellZ = (short)cellz;

    // clamp importante. se o prop for colocado foram do intervalo
    // fechado de [-0.5, 0.5] o prop vaza pra celula vizinha
    const float LIM = 127.0f / 256.0f;
    if (ox >  LIM) ox =  LIM;
    if (ox < -LIM) ox = -LIM;
    if (oz >  LIM) oz =  LIM;
    if (oz < -LIM) oz = -LIM;

    p->ox = (signed char)(ox * 256.0f);
    p->oz = (signed char)(oz * 256.0f);

    p->model = (unsigned char)model;

    while(rotDegree < 0.0f) {
        rotDegree += 360.0f;
    }

    while(rotDegree >= 360.0f) {
        rotDegree -= 360.0f;
    }

    p->rot = (unsigned char)(rotDegree * (256.0f/360.0f));

}

/*

LCG - Linear Congruential Generator(LCG)
é um algoritmo classico pra geração de uma sequência de numeros pseudo-aleatórios
calculados com uma função linear em trechos (função definida em trechos (intervalos abertos/partes))
é definido como: X_{n+1] = (a*Xn + c) mod m, onde:
X é a sequencia de valores pseudo-aleatorios.
m é o modulo sendo m > 0
a pe o multiplicador e c o incrmenento
X0 eh a semente ou valor inicial. A seed psao inteiros constantes
quedefinem o gerador.

Basicamente,, pegamos o numero antigo Xn, multiplicamos
por uma constante a, somamos a uma constante c e pegamos o 
resto da divisão por m.
*/

static inline bool IsRoadCell(int x, int z) {
    if (x < 0 || x >= mapWidth || z < 0 || z >= mapHeight) return false;
    int c = mapData[z][x];
    return (c >= 1 && c <= 12);
}

// constantes a e c do numerical recipes in C. Esse numeros foram calculados
// por matematicos de modo que eles demorem muito pra começarem a se repetir.
/*
o algoritmo de LCG tem um problema nos ultimos digitos (os bits menos significativos).
Os ultimos digitios do numero gerado nao sao muitos aleatorios. Eles costumam alternar de
forma obvia (par, impar, par, impar etc). porem os digitos mais significativos (os primeiros)
sao bem embarlahados. o Operador >> 16 pega o nmero binario e empurra ele 16 casas pra direita,
jogando no lixo os 16 bits ruins e usando apenas a metade boa da variavel p/ gerar uma
razoavel posicao aleatoria.
*/
#define PROP_ROT_OFFSET 0.0f
/* lado: +1 = encostado na rua (poste), -1 = encostado no predio (banca) */
/* side: +1 = encostado na rua (poste), -1 = encostado no predio (banca) */
void PopulatePropsOnTile(int tileTag, int propTag, int spacing, float side)
{
    if (spacing < 1) spacing = 1;

    for (int z = 0; z < mapHeight; z++)
    for (int x = 0; x < mapWidth;  x++)
    {
        if (mapData[z][x] != tileTag) continue;

        /* espacamento regular pela posicao, nao por sorteio:
           ao longo de uma calcada reta, (x+z) cresce de 1 em 1 */
        if (((x + z + 3) % spacing) != 0) continue;

        /* o lado da rua define offset E rotacao ao mesmo tempo */
        float d = 0.35f * side;
        float ox = 0.0f, oz = 0.0f, rot = 0.0f;
        if      (IsRoadCell(x + 1, z)) { ox =  d; rot =  90.0f; }
        else if (IsRoadCell(x - 1, z)) { ox = -d; rot = 270.0f; }
        else if (IsRoadCell(x, z + 1)) { oz =  d; rot =   0.0f; }
        else if (IsRoadCell(x, z - 1)) { oz = -d; rot = 180.0f; }
        else continue;      /* calcada sem rua vizinha: nao poe nada */

        AddProp(x, z, propTag, ox, oz, rot + PROP_ROT_OFFSET);
    }
}

/* povoa sem exigir rua vizinha (vegetacao no meio de uma area) */
void PopulateScatter(int tileTag, int propTag, int spacing, unsigned int seed)
{
    if (spacing < 1) spacing = 1;
    for (int z = 0; z < mapHeight; z++)
    for (int x = 0; x < mapWidth;  x++)
    {
        if (mapData[z][x] != tileTag) continue;
        if (((x + z) % spacing) != 0) continue;

        /* offset pseudo-aleatorio dentro da celula: quebra o alinhamento
           da grade, senao a vegetacao vira fila */
        seed = seed * 1664525u + 1013904223u;
        float ox = (((seed >> 16) % 100) / 100.0f - 0.5f) * 0.7f;
        seed = seed * 1664525u + 1013904223u;
        float oz = (((seed >> 16) % 100) / 100.0f - 0.5f) * 0.7f;

        AddProp(x, z, propTag, ox, oz, 0.0f);
    }
}

/* ============================================================================
   BILLBOARD: quad de 2 triangulos que encara a camera.
   beta = atan2(eye.x - px, eye.z - pz)
   ATENCAO: dx vem PRIMEIRO. Na convencao deste projeto o angulo e medido a
   partir de +Z e forward = (sin, cos).
   ATENCAO 2: atan2f devolve RADIANOS e sceGumRotateY espera RADIANOS.
   Nao multiplique por M_PI/180 aqui.
   ========================================================================= */
struct VertexBB { float u, v; unsigned int color; float x, y, z; };
 
static void DrawBillboard(float wx, float wz, float w, float h,
                          const PspTexture& tex)
{
    VertexBB* v = (VertexBB*)sceGuGetMemory(6 * sizeof(VertexBB));
    if (!v) return;
 
    const unsigned int C = 0xFFFFFFFF;
    const float hw = w * 0.5f;
 
    v[0].u=0; v[0].v=0; v[0].color=C; v[0].x=-hw; v[0].y=0; v[0].z=0;   // base
    v[1].u=1; v[1].v=0; v[1].color=C; v[1].x= hw; v[1].y=0; v[1].z=0;   // base
    v[2].u=1; v[2].v=1; v[2].color=C; v[2].x= hw; v[2].y=h; v[2].z=0;   // topo
    v[3] = v[0];
    v[4] = v[2];
    v[5].u=0; v[5].v=1; v[5].color=C; v[5].x=-hw; v[5].y=h; v[5].z=0;   // topo
 
    float beta = atan2f(g_eye.x - wx, g_eye.z - wz);
 
    sceGumPushMatrix();
        ScePspFVector3 pos = { wx, 0.0f, wz };
        sceGumTranslate(&pos);
        sceGumRotateY(beta);
        sceGuDisable(GU_CULL_FACE);
        UseTexturePsp(tex);
        sceGumDrawArray(GU_TRIANGLES,
            GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
            6, 0, v);
        sceGuEnable(GU_CULL_FACE);
    sceGumPopMatrix();
 
    g_drawCalls++;
    g_trisDrawn += 2;
}

#define SIN_HALF_FOV 0.9539f    /* sqrt(1 - 0.30^2) */
#define PROP_VIEW_RADIUS (VIEW_RADIUS + 3.0f)

void DrawProps(void)
{
    float a = angleVehicle * M_PI / 180.0f;
    float fwdX = sinf(a), fwdZ = cosf(a);
 
    for (int i = 0; i < propCount; i++)
    {
        PropInstance* p = &props[i];
 
        float wx = (float)p->cellX + PropOffset(p->ox);
        float wz = (float)p->cellZ + PropOffset(p->oz);
 
        /* culling por distancia: compara d^2 com R^2, sem raiz */
        float dx = wx - g_eye.x, dz = wz - g_eye.z;
        float d2 = dx*dx + dz*dz;
       // if (d2 > VIEW_RADIUS * VIEW_RADIUS) continue;
        if (d2 > PROP_VIEW_RADIUS * PROP_VIEW_RADIUS) continue;
 
        const PropModelDef& def = PROP_MODELS[p->model];

        /* culling por cone, desligado bem perto do olho */
        if (d2 > 4.0f) {
            float dist = sqrtf(d2);
            float propR = def.scale * 1.5f;      /* raio envolvente aproximado */
            if ((dx*fwdX + dz*fwdZ) < COS_HALF_FOV * dist - SIN_HALF_FOV * propR)
                continue;
        }
 
        if (def.billboard) {
            DrawBillboard(wx, wz, def.scale, def.scale * def.aspect,
                          propTex[p->model]);
        } else {
            /* levanta o modelo para a BASE encostar no chao (y=0).
            minY e o ponto mais baixo do modelo; se for negativo, o modelo
            afunda, e -minY*escala e exatamente quanto falta subir. */
            float liftY = -propModels[p->model].getMinY() * def.scale + def.offsetY;

            ScePspFVector3 pos = { wx, liftY, wz };
            ScePspFVector3 scl = { def.scale, def.scale, def.scale };
            sceGumPushMatrix();
                sceGumTranslate(&pos);
                sceGumRotateY(PropRotationDegree(p->rot) * M_PI / 180.0f);
                sceGumScale(&scl);
                propModels[p->model].desenha();
            sceGumPopMatrix();
            g_drawCalls++;
            g_trisDrawn += propModels[p->model].getNFaces();
        }
    }
}

/* ============================================================================
   FUNDO PANORAMICO 2D

   POR QUE 2D: montanhas em geometria 3D teriam o mesmo problema do skybox em
   cubo -- a GE do PSP nao tem clipper de frustum completo e descarta triangulos
   que cruzam o plano near. Em 2D nao existe plano near.

   COMO GIRA COM A CAMERA: em vez de mover geometria, DESLIZA a coordenada de
   textura.  u0 = (angulo / 360) * repeticoes
   Girar 360 graus desloca a textura por BACKDROP_REPEAT larguras completas.
   ========================================================================= */
#define BACKDROP_REPEAT   2.0f    /* quantas vezes a textura cabe numa volta */
#define BACKDROP_TOP      0.0f    /* topo do fundo, em pixels de tela */
#define BACKDROP_BOTTOM 150.0f    /* onde termina (o chao 3D cobre abaixo) */

void LoadBackdrop(void)
{
    /* sem mipmap: o fundo e sempre desenhado no mesmo tamanho,
       entao os niveis reduzidos nunca seriam usados */
    if (!LoadTexturePsp("assets/tex/backdrop.raw", backdropTex, false))
        printf("AVISO: backdrop.raw nao carregou\n");
}

void DrawBackdrop(float cameraAngleDegree)
{
    if (backdropTex.nLevels <= 0 || backdropTex.data[0] == NULL) return;

    struct VBg { float u, v; float x, y, z; };
    VBg* v = (VBg*)sceGuGetMemory(2 * sizeof(VBg));
    if (!v) return;

    /* normaliza o angulo: sem isso, u0 cresce sem limite ao dirigir em
       circulos e a precisao do float degrada com o tempo */
    float ang = cameraAngleDegree;
    while (ang <    0.0f) ang += 360.0f;
    while (ang >= 360.0f) ang -= 360.0f;

    float u0 = (ang / 360.0f) * BACKDROP_REPEAT;
    float u1 = u0 + 1.0f;              /* uma tela = uma largura de textura */

    /* GU_SPRITES: 2 vertices definem o retangulo (canto sup-esq e inf-dir) */
    v[0] = (VBg){ u0, 0.0f,   0.0f, BACKDROP_TOP,    0.0f };
    v[1] = (VBg){ u1, 1.0f, 480.0f, BACKDROP_BOTTOM, 0.0f };

    UseTexturePsp(backdropTex);
    sceGuTexWrap(GU_REPEAT, GU_CLAMP);          /* repete em U, trava em V */
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);   /* fundo opaco: so copia */

    sceGumDrawArray(GU_SPRITES,
        GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 2, 0, v);

    sceGuTexWrap(GU_REPEAT, GU_REPEAT);   /* restaura: as ruas precisam repetir */
    InvalidateTexCache();                 /* mudei estado por fora do bind */

    g_drawCalls++;
}