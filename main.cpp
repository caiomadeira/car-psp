#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <string.h>
#include <pspctrl.h>
#include <psprtc.h>
#include <psppower.h>
#include <math.h>
#include "common.h"
#include "map.h"
#include "object3d.h"
#include "texture.h"

// short chega ate 32767. Ele usa 16 bits de memoria (2 bytes (x = 2 bytes e z = 2 bytes; total = 4 bytes)) com range [-32767, 32767]
/*
g_tileCount - qtd tiles por fila
*/
struct TileRef {
    short x, z; 
};

#define TYPES_TEXTURE_COUNT 13 // qtd de diferentes texturas
static TileRef g_tiles[TYPES_TEXTURE_COUNT][512]; // [tipo][index] - uma fila por tipo de textura. 
static int g_tileCount[TYPES_TEXTURE_COUNT]; // 
// custo de memoria 13 * 512 * 4 = 26624 bytes = 26 kb

float PosVehicleX = 2.0f;
float PosVehicleZ = 2.0f;
float angleVehicle = 0.0f;
float speedVehicle = 0.0f;

float camDist = 3.0f;
float camHeight = 1.5f;

ScePspFVector3 g_eye, g_center;

PSP_MODULE_INFO("CityMap", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

unsigned int __attribute__((aligned(16))) list[262144];

Objeto3D buildingAModel;
Objeto3D buildingBModel;
Objeto3D buildingCModel;
Objeto3D carModel;

// codigos que definem o tipo do elemento em uma celula (igual referencia OpenGL)
#define VAZIO  0
#define PREDIO 5
#define RUA    0   // no seu mapa atual, 0 == rua/vazio

PspTexture textureStreets[13];

void* fbp0 = (void*)0;
u64 t0, t1, t2;
float res = (float)sceRtcGetTickResolution();

struct VertexColor
{
    unsigned int color;
    float nx, ny, nz;
    float x, y, z;
};

struct VertexTextured {
    float u, v;
    unsigned int color;
    float x, y, z;
};

#define MAX_VERTS 40000
static VertexColor __attribute__((aligned(16))) g_verts[MAX_VERTS];
static int g_vertCount = 0;

//void DrawFloor(float posX, float posZ, int type);

// define filas para as texturas 
static void QueueStreetTile(int x, int z, int type) {
    if (g_tileCount[type] < 512) {
        g_tiles[type][g_tileCount[type]].x = x;
        g_tiles[type][g_tileCount[type]].z = z;
        g_tileCount[type]++;
    }
}

// flush eh basicamente descarregar/ativar a textura pra GPU (bind) e chamar um drawcall c todos os tiels daquele tipo de uma vez.
void FlushStreetTiles(void) {
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    sceGumUpdateMatrix();

    for(int type = 1; type <= 12; type++) {
        int tileCount = g_tileCount[type];
        if (tileCount == 0) continue; 
        // uso a displaylist (definida em cima) como memoria temporaria
        VertexTextured* v = (VertexTextured*)sceGuGetMemory(tileCount * 6 * sizeof(VertexTextured));
        // add msg de erro em caso de falha
        for(int i = 0; i < tileCount; i++) {
            float x = (float)g_tiles[type][i].x;
            float z = (float)g_tiles[type][i].z;
            float y = 0.01f; // p/ 1cm acima do chao
            VertexTextured * q = &v[i*6];

            q[0].u =0; 
            q[0].v = 1; 
            q[0].color = 0xFFFFFFFF; 
            q[0].x = x - 0.5f; 
            q[0].y = y; 
            q[0].z = z - 0.5f;
            
            q[1].u = 1; 
            q[1].v = 1; 
            q[1].color = 0xFFFFFFFF; 
            q[1].x = x + 0.5f; 
            q[1].y = y; 
            q[1].z = z - 0.5f;

            q[2].u = 1; 
            q[2].v = 0; 
            q[2].color = 0xFFFFFFFF; 
            q[2].x = x + 0.5f; 
            q[2].y = y; 
            q[2].z = z + 0.5f;

            q[3] = q[0];
            q[4] = q[2];
            
            q[5].u = 0; 
            q[5].v = 0; 
            q[5].color = 0xFFFFFFFF; 
            q[5].x = x - 0.5f; 
            q[5].y = y; 
            q[5].z = z + 0.5f;
        }

        UseTexturePsp(textureStreets[type]); // bind p N tiles
        // draw call = scegumdrawaaray (para N tiles)
        sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, tileCount*6, 0, v);
        g_tileCount[type] = 0; // limpando a fila p proximo frame
    }
    DisableTexturePsp();
}

// ---------- Helpers de geometria ----------
static void PushVertex(float x, float y, float z, float nx, float ny, float nz, unsigned int color)
{
    if (g_vertCount >= MAX_VERTS) return;
    VertexColor* v = &g_verts[g_vertCount++];
    v->color = color;
    v->nx = nx; v->ny = ny; v->nz = nz;
    v->x = x; v->y = y; v->z = z;
}

static void PushQuad(
    float x0, float y0, float z0,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float nx, float ny, float nz,
    unsigned int color)
{
    PushVertex(x0, y0, z0, nx, ny, nz, color);
    PushVertex(x1, y1, z1, nx, ny, nz, color);
    PushVertex(x2, y2, z2, nx, ny, nz, color);

    PushVertex(x0, y0, z0, nx, ny, nz, color);
    PushVertex(x2, y2, z2, nx, ny, nz, color);
    PushVertex(x3, y3, z3, nx, ny, nz, color);
}

// ---------------------------------------------------
u64 g_lastTick = 0;

float GetDeltaTime() 
{
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    u32 tickResolution = sceRtcGetTickResolution();
    if (g_lastTick == 0) g_lastTick = tick;
    float dt = (float)(tick - g_lastTick) / (float)tickResolution;
    g_lastTick = tick;
    return dt;
}

void FindInitialPosition(float& outX, float& outZ)
{
    for(int z = 0; z < mapHeight; z++) {
        for(int x = 0; x < mapWidth; x++) {
            if (mapData[z][x] != 0) {
                outX = (float)x;
                outZ = (float)z;
                return;
            }
        }
    }
    outX = 0.0f;
    outZ = 0.0f;
}

void UpdateVehicle(float dt)
{
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);

    // analogico esquerdo: -128 a 127, centralizado em 0 (offset de 128)
    float analogX = (float)(pad.Lx - 128) / 128.0f;

    const float turnSpeed = 90.0f;    // graus por segundo
    const float maxSpeed  = 4.0f;     // unidades por segundo
    const float accel     = 3.0f;

    // pspDebugScreenSetXY(0, 0);
    // pspDebugScreenPrintf("pos=(%.1f,%.1f) ang=%.1f spd=%.2f cell=%d   \n",
    // PosVehicleX, PosVehicleZ, angleVehicle, speedVehicle,
    // mapData[(int)roundf(PosVehicleZ)][(int)roundf(PosVehicleX)]);

    // vira o carro com o analogico ou D-pad
    if (fabsf(analogX) > 0.2f)
        angleVehicle -= analogX * turnSpeed * dt;

    if (pad.Buttons & PSP_CTRL_LEFT)  angleVehicle += turnSpeed * dt;
    if (pad.Buttons & PSP_CTRL_RIGHT) angleVehicle -= turnSpeed * dt;

    // acelera com X, freia/re com quadrado
    if (pad.Buttons & PSP_CTRL_CROSS)
    {
        speedVehicle += accel * dt;
        if (speedVehicle > maxSpeed) speedVehicle = maxSpeed;
    }
    else if (pad.Buttons & PSP_CTRL_SQUARE)
    {
        speedVehicle -= accel * dt;
        if (speedVehicle < -maxSpeed * 0.5f) speedVehicle = -maxSpeed * 0.5f;
    }
    else
    {
        // atrito natural, para o carro aos poucos
        if (speedVehicle > 0) speedVehicle = fmaxf(0.0f, speedVehicle - accel * 2.0f * dt);
        else if (speedVehicle < 0) speedVehicle = fminf(0.0f, speedVehicle + accel * 2.0f * dt);
    }

    if (speedVehicle != 0.0f)
    {
        float angleRads = angleVehicle * M_PI / 180.0f;
        float dist = speedVehicle * dt;

        float nextX = PosVehicleX + dist * sinf(angleRads);
        float nextZ = PosVehicleZ + dist * cosf(angleRads);

        int gridX = (int)roundf(nextX);
        int gridZ = (int)roundf(nextZ);

        // colisao: so anda se a celula de destino for rua (!=0, na sua convencao atual)
        if (gridX >= 0 && gridX < mapWidth && gridZ >= 0 && gridZ < mapHeight)
        {
            if (mapData[gridZ][gridX] != 0)
            {
                PosVehicleX = nextX;
                PosVehicleZ = nextZ;
            }
            else
            {
                speedVehicle = 0.0f; // bateu num predio
            }
        }
        else
        {
            speedVehicle = 0.0f; // fora do mapa
        }
    }
}

void SetupCameraFollowingCar()
{
    // psp roda a 222MHz por padrao. Da pra forçar.
    // Isso da aprox 50% de cpu e 66% mais de banda de barramento (antes era 111MHZ).
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(60.0f, 16.0f / 9, 0.5f, 20.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();

    float angleRads = angleVehicle * M_PI / 180.0f;

    // camera fica atras do carro, na direcao oposta ao heading
    g_eye.x = PosVehicleX - sinf(angleRads) * camDist;
    g_eye.y = camHeight;
    g_eye.z = PosVehicleZ - cosf(angleRads) * camDist;

    g_center.x = PosVehicleX;
    g_center.y = 0.3f;
    g_center.z = PosVehicleZ;

    ScePspFVector3 up = { 0.0f, 1.0f, 0.0f };

    sceGumLookAt(&g_eye, &g_center, &up);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}

void DrawVehicle()
{
    ScePspFVector3 pos = { PosVehicleX, 0.0f, PosVehicleZ };
    ScePspFVector3 scale = { 0.2f, 0.2f, 0.2f };

    sceGumPushMatrix();
        sceGumTranslate(&pos);
        sceGumRotateY(angleVehicle * M_PI / 180.0f);
        sceGumScale(&scale);
        carModel.desenha();
    sceGumPopMatrix();
}

// Equivalente ao DesenhaLadrilho() da referencia: 1 quad de piso
static void DesenhaLadrilho(float x, float z, unsigned int color)
{
    PushQuad(
        x,       0.0f, z,
        x + 1.0f, 0.0f, z,
        x + 1.0f, 0.0f, z + 1.0f,
        x,       0.0f, z + 1.0f,
        0, 1, 0,
        color
    );
}

// // Equivalente ao DesenhaCubo() da referencia: prisma completo (predio)
static void DesenhaCubo(float x, float z, float altura, unsigned int color)
{
    const float H = altura;

    // topo
    PushQuad(
        x,       H, z,
        x + 1.0f, H, z,
        x + 1.0f, H, z + 1.0f,
        x,       H, z + 1.0f,
        0, 1, 0,
        color
    );
    // frente (-z)
    PushQuad(
        x,       0, z,   x + 1.0f, 0, z,   x + 1.0f, H, z,   x, H, z,
        0, 0, -1, color
    );
    // tras (+z)
    PushQuad(
        x + 1.0f, 0, z + 1.0f, x, 0, z + 1.0f, x, H, z + 1.0f, x + 1.0f, H, z + 1.0f,
        0, 0, 1, color
    );
    // esquerda (-x)
    PushQuad(
        x, 0, z + 1.0f, x, 0, z, x, H, z, x, H, z + 1.0f,
        -1, 0, 0, color
    );
    // direita (+x)
    PushQuad(
        x + 1.0f, 0, z, x + 1.0f, 0, z + 1.0f, x + 1.0f, H, z + 1.0f, x + 1.0f, H, z,
        1, 0, 0, color
    );
}

void DrawBuilding(Objeto3D& modelo, float x, float z, float scale, float rotY)
{
    float offsetX = modelo.getCenterX() * scale;
    float offsetZ = modelo.getCenterZ() * scale;

    // ScePspFVector3 pos = { x - offsetX, 0.0f, z - offsetZ};
    ScePspFVector3 pos = { x , 0.0f, z };
    ScePspFVector3 scl = { scale, scale, scale };

    sceGumPushMatrix();
        sceGumTranslate(&pos);
        sceGumRotateY(rotY * M_PI / 180.0f);
        sceGumScale(&scl);
        modelo.desenha();
    sceGumPopMatrix();
}

// void BuildFloor()
// {
//     g_vertCount = 0;
//     const unsigned int defaultColor = 0xFF004400;

//     for (int z = 0; z < mapHeight; z++)
//         for (int x = 0; x < mapWidth; x++) {
//             // desenha gramam ou qualquer outra coisa sos se a celula nao for a rua.
//             if (mapData[z][x] < 1 || mapData[z][x] > 12) {
//                 DesenhaLadrilho((float)x, (float)z, defaultColor);
//             }
//         }
// }

#define VIEW_RADIUS 14.0f
#define LOD_RADIUS 7.0f
#define COS_HALF_FOV 0.30f

// o predios eh desenhado a cada loop.
void DrawBuildingsAndStreets(float scaleBuildingA, float scaleBuildingB, float scaleBuildingC)
{   
    // passando graus pra radianos
    float a = angleVehicle * M_PI / 180.0f; 
    // versor forward que aponta na direcao de onde o carro e camera esta olhando
    float versorFowardDX = sinf(a); // componente x do versor
    float versorFowardDZ = cosf(a);

    int cx = (int)PosVehicleX;
    int cz = (int)PosVehicleZ;
    int r = (int)VIEW_RADIUS + 1; 

    // essa parte é de clamp pra add limites do array
    int x0 = cx - r;
    int x1 = cx + r;

    int z0 = cz - r;
    int z1 = cz + r;

    if (x0 < 0) x0 = 0;
    if (x1 >= mapWidth) {
        x1 = mapWidth - 1;
    }

    if (z0 < 0) z0 = 0;
    if (z1 >= mapHeight) {
        z1 = mapHeight - 1;
    }

    for (int z = z0; z <= z1; z++)
    {
        for (int x = x0; x <= x1; x++)
        {
            float dx = (x + 0.5f) - g_eye.x;
            float dz = (z + 0.5f) - g_eye.z;
            float d2 = dx*dx + dz*dz; // pro calculo do versor (a parte do denominador) é a distancia || (raiz de x^2 + z^2) ||

            if (d2 > VIEW_RADIUS*VIEW_RADIUS) continue;
            // teste de culling de cone - cria um cone e o que esta dentro é renderizado
            if (d2 > 4.0f) {
                float inverse = 1.0f / sqrtf(d2); // inverso do versor 1/modulo de d
                if ((dx*versorFowardDX + dz*versorFowardDZ) * inverse < COS_HALF_FOV) continue;
            }

            // desenhando na tela o mapa
            int coord = mapData[z][x];
            if (coord == 0)
            {
                int seed = (x * 13) + (z * 17);
                float rotY = (float)((seed % 4) * 90);
                if (d2 > LOD_RADIUS*LOD_RADIUS) {   
                    DesenhaCubo((float)x, (float)z, 2.0f + (seed % 4), 0XFF808090);
                } else {
                    int buildingType = seed % 3;
                    if (buildingType == 0) DrawBuilding(buildingAModel, x, z, scaleBuildingA, rotY);
                    else if (buildingType == 1) DrawBuilding(buildingBModel, x, z, scaleBuildingB, rotY);
                    else DrawBuilding(buildingCModel, x, z, scaleBuildingC, rotY);
                }
            }
            else if (coord >= 1 && coord <= 12) // eh rua 
            {
                QueueStreetTile(x, z, coord);
            }
        }
    }
}

void DrawGround(void)
{
    struct V { unsigned int c; float x, y, z; };
    V* v = (V*)sceGuGetMemory(6 * sizeof(V));
    const unsigned int C = 0xFF004400;

    // um quadrado de VIEW_RADIUS ao redor do carro -- sempre dentro do far plane
    const float R  = VIEW_RADIUS + 2.0f;
    const float x0 = PosVehicleX - R, x1 = PosVehicleX + R;
    const float z0 = PosVehicleZ - R, z1 = PosVehicleZ + R;

    v[0] = (V){C, x0, 0.0f, z0};
    v[1] = (V){C, x1, 0.0f, z0};
    v[2] = (V){C, x1, 0.0f, z1};
    v[3] = v[0];
    v[4] = v[2];
    v[5] = (V){C, x0, 0.0f, z1};

    sceGuDisable(GU_TEXTURE_2D);          // nao herdar textura do frame anterior
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 6, 0, v);
}

// void DrawFloor(float posX, float posZ, int type) {
//     if (type < 1 || type > 12) return;

//     VertexTextured* v = (VertexTextured*)sceGuGetMemory(4*sizeof(VertexTextured));
//     float t = 0.5f;
//     unsigned int c = 0xFFFFFFFF;
//     float y = 0.01f;

//     v[0].u = 0.0f; v[0].v = 1.0f; v[0].color = c; v[0].x = -t; v[0].y = y; v[0].z = -t;
//     v[1].u = 1.0f; v[1].v = 1.0f; v[1].color = c; v[1].x =  t; v[1].y = y; v[1].z = -t;
//     v[2].u = 1.0f; v[2].v = 0.0f; v[2].color = c; v[2].x =  t; v[2].y = y; v[2].z =  t;
//     v[3].u = 0.0f; v[3].v = 0.0f; v[3].color = c; v[3].x = -t; v[3].y = y; v[3].z =  t;

//     UseTexturePsp(textureStreets[type]);
//     sceGumPushMatrix();
//     ScePspFVector3 pos = {posX, 0.0f, posZ};
//     sceGumTranslate(&pos);
//     sceGumDrawArray(GU_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 4, 0, v);
//     sceGumPopMatrix();

//     DisableTexturePsp();
// }

void InitGU()
{
    sceGuInit();
    sceGuStart(GU_DIRECT, list);

    sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
    sceGuDispBuffer(480, 272, (void*)0x88000, 512);
    sceGuDepthBuffer((void*)0x110000, 512);

    sceGuOffset(2048 - (480 / 2), 2048 - (272 / 2));
    sceGuViewport(2048, 2048, 480, 272);

    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDepthRange(65535, 0);
    sceGuClearDepth(0);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);

    sceGuEnable(GU_CULL_FACE);
    sceGuFrontFace(GU_CW);

    /*
    ok, com tecnica de culling o cenario a frente vai sumir e aparecer do nada na borda do raio (pop-in);
    o hardware do PSP tem opcao de fog. o custo eh zero pra GE do PSP.
    0xFF303030 usei pois a cor do FOG tem que ser a cor do céu também.
    */
   sceGuEnable(GU_FOG);
   sceGuFog(8.0f, VIEW_RADIUS, 0xFF303030);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

// void SetupCamera()
// {
//     sceGumMatrixMode(GU_PROJECTION);
//     sceGumLoadIdentity();
//     sceGumPerspective(60.0f, 16.0f / 9.0f, 0.5f, 100.0f);

//     sceGumMatrixMode(GU_VIEW);
//     sceGumLoadIdentity();

//     float cx = mapWidth * 0.5f;
//     float cz = mapHeight * 0.5f;

//     ScePspFVector3 eye    = { cx, mapHeight * 0.5f, cz + mapHeight * 1.2f };
//     ScePspFVector3 center = { cx, 0.0f, cz };
//     ScePspFVector3 up     = { 0.0f, 1.0f, 0.0f };

//     sceGumLookAt(&eye, &center, &up);

//     sceGumMatrixMode(GU_MODEL);
//     sceGumLoadIdentity();
// }

int main()
{
    scePowerSetClockFrequency(333, 333, 166); // CPU=333, PLL=333, BUS=166

    float scaleBuildingA, scaleBuildingB, scaleBuildingC;
    const char* roadTexNames[13] = {
        "", "CROSS", "DL", "DLR", "DR", "LR", "None", 
        "UD", "UDL", "UDR", "UL", "ULR", "UR"
    };

    char roadTextPath[100];
    for(int i = 1; i <= 12; i++) {
        sprintf(roadTextPath, "assets/%s.raw", roadTexNames[i]);
        if (!LoadTexturePsp(roadTextPath, textureStreets[i])) {
            // pelo menos loga/pisca algo em debug pra você saber que falhou
           // pspDebugScreenPrintf("Falha ao carregar %s\n", roadTextPath);
        }
    }
    
    SetupCallbacks();

    if (!LoadMap("mapa.txt"))
    {
        // erro real, sem mapa nao ha o que desenhar
        return 1;
    }
    // pspDebugScreenInit();
    // pspDebugScreenPrintf("mapWidth=%d mapHeight=%d\n", mapWidth, mapHeight);

    FindInitialPosition(PosVehicleX, PosVehicleZ);
    InitGU();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);


    buildingAModel.leObjeto("assets/tri/building-sample-tower-a.tri", 0xFFFFFFFF);
    buildingAModel.carregarTextura("assets/tri/colormap.raw");
    scaleBuildingA = 1.0f / fmaxf(buildingAModel.getSizeX(), buildingAModel.getSizeZ());

    buildingBModel.leObjeto("assets/tri/building-sample-tower-b.tri", 0xFFFFFFFF);
    buildingBModel.carregarTextura("assets/tri/variation-a.raw");
    scaleBuildingB = 1.0f / fmaxf(buildingBModel.getSizeX(), buildingBModel.getSizeZ());

    buildingCModel.leObjeto("assets/tri/building-sample-tower-c.tri", 0xFFFFFFFF);
    buildingCModel.carregarTextura("assets/tri/variation-b.raw");
    scaleBuildingC = 1.0f / fmaxf(buildingCModel.getSizeX(), buildingCModel.getSizeZ());

    //pspDebugScreenPrintf("scaleA=%.3f scaleB=%.3f scaleC=%.3f\n", scaleBuildingA, scaleBuildingB, scaleBuildingC);
    carModel.leObjeto("assets/tri/sedan.tri", 0xFFFFFFFF);
    carModel.carregarTextura("assets/tri/colormap.raw");
    
    //BuildFloor();

    while (1)
    {
        float dt = GetDeltaTime();
        UpdateVehicle(dt);

        u64 t0, t1, t2;
        float res = (float)sceRtcGetTickResolution();

        sceRtcGetCurrentTick(&t0);

        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xFF303030);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        SetupCameraFollowingCar();
        DrawGround();
        // sceGumDrawArray(
        //     GU_TRIANGLES,
        //     GU_COLOR_8888 |
        //     GU_NORMAL_32BITF |
        //     GU_VERTEX_32BITF |
        //     GU_TRANSFORM_3D,
        //     g_vertCount,
        //     NULL,
        //     g_verts
        // );

        DrawBuildingsAndStreets(scaleBuildingA, scaleBuildingB, scaleBuildingC);
        FlushStreetTiles();
        DrawVehicle();

        sceGuFinish();
        sceRtcGetCurrentTick(&t1);

        sceGuSync(0, 0);
        sceRtcGetCurrentTick(&t2);

        // pra debug
        float cpuMs = (float)(t1-t0)/res*1000.0f;   // cpuMs alto -> você é CPU-bound (matrizes, loops, sceGuGetMemory) -> itens 2, 3.
        float gpuMs = (float)(t2-t1)/res*1000.0f;  // gpuMs alto -> é GPU-bound (vértices, texturas, fillrate) -> itens 2, 4, 5.
        ProfilerUpdateFPS();

        pspDebugScreenSetOffset((int)fbp0);
        pspDebugScreenSetXY(0,0);
        pspDebugScreenPrintf("FPS %5.1f  cpu %5.2fms  gpu %5.2fms  verts %d   ",
                            g_fps, g_cpuMs, g_gpuMs, g_vertCount);

        sceDisplayWaitVblankStart();
        fbp0 = sceGuSwapBuffers();

        // sceDisplayWaitVblankStart();
        // sceGuSwapBuffers();
    }

    return 0;
}