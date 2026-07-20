#include "common.h"
#include "map.h"
#include "object3d.h"
#include "texture.h"
#include "hud.h"
#include "psptexture.h"
#include "screen.h"

PSP_MODULE_INFO("CityMap", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-256); // -256 = garante tudo menos 256kb ou seja to pegando o maximo possivel

#define SKIP_MENU 1

#define FAR_DIST     (VIEW_RADIUS + 10.0f)
#define VIEW_RADIUS 14.0f
#define LOD_RADIUS 7.0f
#define COS_HALF_FOV 0.30f
#define NEAR_GUARD 4.0f

#define MAX_TILES_PER_TYPE 128 // limitando a fuka de acordo com a capacidade do display list

// --------- variaveis do veiculo (player)
float PosVehicleX = 2.0f;
float PosVehicleZ = 2.0f;
float angleVehicle = 0.0f;
float speedVehicle = 0.0f;

// --------- camera 
float camDist = 1.2f;
float camHeight = 0.6f;
ScePspFVector3 g_eye, g_center;

// --------- GPU
unsigned int __attribute__((aligned(16))) list[262144];
void* fbp0 = (void*)0;

// ---------- modelos
Objeto3D buildingAModel;
Objeto3D buildingBModel;
Objeto3D buildingCModel;
Objeto3D carModel;

// -------- HUD e telemetria 
static int showHud = 1;
static int g_drawCalls = 0;
static int g_trisDrawn = 0;

// ----- filas baching pra textura
// short chega ate 32767. Ele usa 16 bits de memoria (2 bytes (x = 2 bytes e z = 2 bytes; total = 4 bytes)) com range [-32767, 32767]
/*
g_tileCount - qtd tiles por fila
*/
struct TileRef { short x, z; };
static TileRef g_tiles[NUM_ENV_TEX][MAX_TILES_PER_TYPE]; // [tipo][index] - uma fila por tipo de textura. 
static int g_tileCount[NUM_ENV_TEX]; 

// ------- estruturas de vertices
struct VertexTextured {
    float u, v;
    unsigned int color;
    float x, y, z;
};

/*
static void QueueStreetTile(int x, int z, int mapCode)
---------------------------------------------------------------------
Recebe o codigo do mapa (numero do mapa.txt, a entrada da matriz) e 
converte para um slot de entrada. Caso tenha algum codigo que nao existe
só ignora.
*/
static void QueueStreetTile(int x, int z, int mapCode) 
{
    
    int slot = TexSlotFromMapCode(mapCode);
    if (slot < 0) return;

    if (g_tileCount[slot] < MAX_TILES_PER_TYPE) {
        g_tiles[slot][g_tileCount[slot]].x = (short)x;
        g_tiles[slot][g_tileCount[slot]].z = (short)z;
        g_tileCount[slot]++;
    }
}

/* 
void FlushStreetTiles(void)
----------------------------------------------------------------
flush eh basicamente descarregar/ativar a textura pra GPU (bind) 
e chamar um drawcall c todos os tiels daquele tipo de uma vez.
Em resumo, faz 1 bind + 1 draw call por tipo de textura, em vez de 1 por tile.
*/
void FlushStreetTiles(void) 
{
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    sceGumUpdateMatrix();

    for(int slot = 0; slot < NUM_ENV_TEX; slot++) 
    {
        int tileCount = g_tileCount[slot];
        if (tileCount == 0) continue; 
        // uso a displaylist (definida em cima) como memoria temporaria
        VertexTextured* v = (VertexTextured*)sceGuGetMemory(tileCount * 6 * sizeof(VertexTextured));
        if (!v) { g_tileCount[slot] = 0; continue; } // prevencao de corrumpcao d memoria

        for(int i = 0; i < tileCount; i++) {
            float x = (float)g_tiles[slot][i].x;
            float z = (float)g_tiles[slot][i].z;
            float y = 0.05f; // p/ 1cm acima do chao;  evita z-fighting
            VertexTextured * q = &v[i * 6];

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

        UseTexturePsp(EnvTexture(slot)); // bind p N tiles
        // draw call = scegumdrawaaray (para N tiles)
        sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, tileCount*6, 0, v);
        
        g_drawCalls++;
        g_trisDrawn += tileCount * 2;
        
        g_tileCount[slot] = 0; // limpando a fila p proximo frame
    }
    DisableTexturePsp();
}

//  TIME ---------------------------------------------------
static u64 g_lastTick = 0;

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

void UpdateVehicle(float dt)
{
    static SceCtrlData oldPad = { 0 }; // aparentemente, oldPad precisa ser STATIC, o pq n sei.
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);

    unsigned int pressed = pad.Buttons & ~oldPad.Buttons;
    oldPad = pad;

    if (pressed & PSP_CTRL_SELECT) showHud = !showHud;

    float analogX = (float)(pad.Lx - 128) / 128.0f;

    const float turnSpeed = 90.0f;    // graus por segundo
    const float maxSpeed  = 4.0f;     // unidades por segundo
    const float accel     = 3.0f;

    // vira o carro com o analogico ou D-pad
    if (fabsf(analogX) > 0.2f)
        angleVehicle -= analogX * turnSpeed * dt;

    if (pad.Buttons & PSP_CTRL_LEFT)  angleVehicle += turnSpeed * dt;
    if (pad.Buttons & PSP_CTRL_RIGHT) angleVehicle -= turnSpeed * dt;

    // acelera com X, freia/re com quadrado
    if (pad.Buttons & PSP_CTRL_CROSS) {
        speedVehicle += accel * dt;
        if (speedVehicle > maxSpeed) speedVehicle = maxSpeed;
    }
    else if (pad.Buttons & PSP_CTRL_SQUARE) {
        speedVehicle -= accel * dt;
        if (speedVehicle < -maxSpeed * 0.5f) speedVehicle = -maxSpeed * 0.5f;
    }
    else {
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
        if (gridX >= 0 && gridX < mapWidth && gridZ >= 0 && gridZ < mapHeight) {
            if (mapData[gridZ][gridX] != 0) { PosVehicleX = nextX; PosVehicleZ = nextZ; }
            else { speedVehicle = 0.0f; } // bateu num predio 
        } else { speedVehicle = 0.0f; } // fora do mapa 
    }
}

/*
float GridAdjancey(int x, int z, int seed);
Identifica a celula vizinha, necessiarmanete pra rotacionar os predios pra rua.

*/
float GridAdjancey(int x, int z, int seed) {

    #define IS_ROAD(cx, cz) \
        ( (cx) >= 0 && (cx) < mapWidth && (cz) >= 0 && (cz) < mapHeight && \
          mapData[cz][cx] >= 1 && mapData[cz][cx] <= 12 )

    bool roadUp    = IS_ROAD(x,     z - 1);
    bool roadDown  = IS_ROAD(x,     z + 1);
    bool roadLeft  = IS_ROAD(x - 1, z);
    bool roadRight = IS_ROAD(x + 1, z);
    #undef IS_ROAD
    // definindo a rotacao em graus apontado pra rua. forward=(sin,cos): a=0 olha +z 
    if (roadDown) return 0.0f;
    if (roadUp) return 180.0f;
    if (roadRight) return 90.0f;
    if (roadLeft) return 270.0f;
    return (float)((seed % 4)*90);
}

// ============= CAMERA
/*
void SetupCamera()
fovy: O ângulo de visão vertical, em graus.
aspect: A proporção da tela (largura dividida pela altura).
near: A distância mínima de renderização (evita que objetos muito próximos cortem a tela).
far: A distância máxima de renderização (limita até onde a câmera consegue ver).

*/
void SetupCamera()
{
    float fov = 60.0f; // quanto menos graus mais perto
    float near_dist = 0.8f;

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(fov, 16.0f / 9.0f, near_dist, FAR_DIST);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();

    float angleRads = angleVehicle * M_PI / 180.0f;

    g_eye.x = PosVehicleX - sinf(angleRads) * camDist;
    g_eye.y = camHeight;
    g_eye.z = PosVehicleZ - cosf(angleRads) * camDist;

    g_center.x = PosVehicleX;
    g_center.y = 0.5f;
    g_center.z = PosVehicleZ;

    ScePspFVector3 up = { 0.0f, 1.0f, 0.0f };
    sceGumLookAt(&g_eye, &g_center, &up);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}

void DrawVehicle()
{
    ScePspFVector3 pos = { PosVehicleX, 0.0f, PosVehicleZ };
    ScePspFVector3 scale = { 0.15f, 0.15f, 0.15f };

    sceGumPushMatrix();
        sceGumTranslate(&pos);
        sceGumRotateY(angleVehicle * M_PI / 180.0f);
        sceGumScale(&scale);
        carModel.desenha();
    sceGumPopMatrix();
    
    g_drawCalls++;
    g_trisDrawn += carModel.getNFaces();
}

void DrawBuilding(Objeto3D& model, float x, float z, float scale, float rotY)
{
    ScePspFVector3 pos = { x , 0.0f, z };
    ScePspFVector3 scl = { scale, scale, scale };
    ScePspFVector3 center = { -model.getCenterX(), 0.0f, -model.getCenterZ() };

    sceGumPushMatrix();
        sceGumTranslate(&pos);
        sceGumRotateY(rotY * M_PI / 180.0f);
        sceGumScale(&scl);
        sceGumTranslate(&center);
        model.desenha();
    sceGumPopMatrix();

    g_drawCalls++;
    g_trisDrawn += model.getNFaces();
}

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
    if (x1 >= mapWidth) x1 = mapWidth - 1;
    if (z0 < 0) z0 = 0;
    if (z1 >= mapHeight) z1 = mapHeight - 1;

    for (int z = z0; z <= z1; z++)
    {
        for (int x = x0; x <= x1; x++)
        {
            float dx = (x + 0.5f) - g_eye.x;
            float dz = (z + 0.5f) - g_eye.z;
            float d2 = dx*dx + dz*dz; // pro calculo do versor (a parte do denominador) é a distancia || (raiz de x^2 + z^2) ||

            if (d2 > VIEW_RADIUS*VIEW_RADIUS) continue;
            // teste de culling de cone - cria um cone e o que esta dentro é renderizado
            if (d2 > NEAR_GUARD) {
                float inverse = 1.0f / sqrtf(d2); // inverso do versor 1/modulo de d
                if ((dx*versorFowardDX + dz*versorFowardDZ) * inverse < COS_HALF_FOV) continue;
            }

            // desenhando na tela o mapa
            int cell = mapData[z][x];
            if (cell == 0) {
                QueueStreetTile(x, z, MAPCODE_ASPHALT);
                int seed = (x * 13) + (z * 17);
                float rotY = GridAdjancey(x, z, seed);
                int sortedbuildingType = seed % 3;
                if (sortedbuildingType == 0) DrawBuilding(buildingAModel, x, z, scaleBuildingA, rotY);
                else if (sortedbuildingType == 1) DrawBuilding(buildingBModel, x, z, scaleBuildingB, rotY);
                else DrawBuilding(buildingCModel, x, z, scaleBuildingC, rotY);
            } else { QueueStreetTile(x, z, cell); }
        }
    }
}

void DrawGround(void)
{
    struct V { unsigned int c; float x, y, z; };
    V* v = (V*)sceGuGetMemory(6 * sizeof(V));
    if (!v) return;
    const unsigned int color = 0xFF303030;  // MESMA cor do fog e do clear
    const float R = VIEW_RADIUS + 4.0f;
    const float x0 = PosVehicleX - R, x1 = PosVehicleX + R;
    const float z0 = PosVehicleZ - R, z1 = PosVehicleZ + R;

    v[0] = (V){color, x0, 0.0f, z0};
    v[1] = (V){color, x1, 0.0f, z0};
    v[2] = (V){color, x1, 0.0f, z1};
    v[3] = v[0];
    v[4] = v[2];
    v[5] = (V){color, x0, 0.0f, z1};

    sceGuDisable(GU_TEXTURE_2D);
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 6, 0, v);
    sceGuEnable(GU_TEXTURE_2D);  // reabilita pro resto do frame
    g_drawCalls++;
    g_trisDrawn += 2;
}

static void DrawHud(void)
{
    if (!showHud) return;

    char buf[96];
    float budget = 1000.0f / 60.0f;   /* 16.67 ms por frame a 60 FPS */

    int cellX = (int)roundf(PosVehicleX);
    int cellZ = (int)roundf(PosVehicleZ);
    int cellType = (cellX >= 0 && cellX < mapWidth && cellZ >= 0 && cellZ < mapHeight) ? mapData[cellZ][cellX] : -1;
    Begin2D();
    snprintf(buf, sizeof(buf), "FPS %.0f CPU %.0f%% GPU %.0f%%", g_fps, g_cpuMs / budget * 100.0f, g_gpuMs / budget * 100.0f);
    DrawText2D(buf, 8, 6, 1.0f, 0xFF00FFFF);

    snprintf(buf, sizeof(buf), "RAM %dKB DRAW %d TRI %d", sceKernelTotalFreeMemSize() / 1024, g_drawCalls, g_trisDrawn);
    DrawText2D(buf, 8, 24, 1.0f, 0xFFFFFFFF);

    snprintf(buf, sizeof(buf), "POS %.1f %.1f CELL %d VEL %.1f",PosVehicleX, PosVehicleZ, cellType, speedVehicle);
    DrawText2D(buf, 8, 42, 1.0f, 0xFFFFFFFF);

    snprintf(buf, sizeof(buf), "CLK %dMHz BUS %dMHz BAT %d%%", scePowerGetCpuClockFrequency(), scePowerGetBusClockFrequency(), scePowerGetBatteryLifePercent());
    DrawText2D(buf, 8, 60, 1.0f, 0xFFAAAAAA);

    End2D();
}

void InitGU()
{
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    g_drawCalls = 0;
    g_trisDrawn = 0;

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

    // o CLIP_PLANES acaba com o problema de corte/flickering do chao perto da camera
    /*
    O chão é um quad gigante entao ele sempre se estende pra atras da camera (ele cruza o near plane).
    A cada frame é decidido, atras do near plane, se um triangulo vai ser descartado ou não.

    */
    sceGuEnable(GU_CLIP_PLANES); 

    /*
    ok, com tecnica de culling o cenario a frente vai sumir e aparecer do nada na borda do raio (pop-in);
    o hardware do PSP tem opcao de fog. o custo eh zero pra GE do PSP.
    0xFF303030 usei pois a cor do FOG tem que ser a cor do céu também.
    */
   sceGuEnable(GU_FOG);
   sceGuFog(7.0f, VIEW_RADIUS, 0xFF303030);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void DefSpawnPosition(float& outX, float& outZ) {
    for(int z = 1; z < mapHeight - 1; z++) {
        for (int x = 1; x < mapWidth - 1; x++) {
            if (mapData[z][x] >= 1 && mapData[z][x] <= 12) {
                outX = (float)x;
                outZ = (float)z;
                return;
            }
        }
    }
    outX = mapWidth * 0.5f;
    outZ = mapHeight * 0.5f;
}

int main() {
    scePowerSetClockFrequency(333, 333, 166); // CPU=333, PLL=333, BUS=166
    SetupCallbacks();

    InitGU();
    pspDebugScreenInit();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    char msg[64];

    LoadingScreen("Carregando fontes e menu", list, fbp0);
    InitHUD();

    int nTextures = LoadAllEnvironmentTextures();
    snprintf(msg, sizeof(msg), "Texturas: %d/%d", nTextures, NUM_ENV_TEX);
    LoadingScreen(msg, list, fbp0);
    if (!LoadMap("mapa.txt")) { 
        LoadingScreen("error: mapa.txt n carregou", list, fbp0);
        sceKernelDelayThread(3*1000*1000);
        return 1; 
    }

    LoadingScreen("loading buildings", list, fbp0);
    float scaleBuildingA, scaleBuildingB, scaleBuildingC;
    buildingAModel.leObjeto("assets/tri/building-sample-tower-a.tri", 0xFFFFFFFF);
    buildingAModel.carregarTextura("assets/tri/variation-a.raw");
    scaleBuildingA = 1.0f / fmaxf(buildingAModel.getSizeX(), buildingAModel.getSizeZ());

    buildingBModel.leObjeto("assets/tri/building-sample-tower-b.tri", 0xFFFFFFFF);
    buildingBModel.carregarTextura("assets/tri/variation-b.raw");
    scaleBuildingB = 1.0f / fmaxf(buildingBModel.getSizeX(), buildingBModel.getSizeZ());

    buildingCModel.leObjeto("assets/tri/house1.tri", 0xFFFFFFFF);
    buildingCModel.carregarTextura("assets/tex/house_textures.raw");
    scaleBuildingC = 1.0f / fmaxf(buildingCModel.getSizeX(), buildingCModel.getSizeZ());

    LoadingScreen("loading carro.", list, fbp0);
    carModel.leObjeto("assets/tri/uno_complete.tri", 0xFFFFFFFF);
    carModel.carregarTextura("assets/tri/colormap.raw");

    sceKernelDcacheWritebackInvalidateAll();
    DefSpawnPosition(PosVehicleX, PosVehicleZ);
    LoadingScreen("Done.", list, fbp0);

#if !SKIP_MENU
    if (Menu(list, fbp0) == STATE_QUIT) {
        sceGuTerm();
        sceKernelExitGame();
        return 0;
    }
#endif

    while (1)
    {
        float dt = GetDeltaTime();
        UpdateVehicle(dt);

        u64 t0, t1, t2;
        float res = (float)sceRtcGetTickResolution();

        g_drawCalls = 0;
        g_trisDrawn = 0;

        sceRtcGetCurrentTick(&t0);

        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xFF303030);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        SetupCamera();
        DrawGround();
        DrawBuildingsAndStreets(scaleBuildingA, scaleBuildingB, scaleBuildingC);
        FlushStreetTiles();
        DrawVehicle();
        DrawHud();

        sceGuFinish();
        sceRtcGetCurrentTick(&t1);

        sceGuSync(0, 0);
        sceRtcGetCurrentTick(&t2);

        // pra debug
        g_cpuMs = (float)(t1-t0)/res*1000.0f;   // cpuMs alto -> você é CPU-bound (matrizes, loops, sceGuGetMemory) -> itens 2, 3.
        g_gpuMs = (float)(t2-t1)/res*1000.0f;  // gpuMs alto -> é GPU-bound (vértices, texturas, fillrate) -> itens 2, 4, 5.
        ProfilerUpdateFPS();

        sceDisplayWaitVblankStart();
        fbp0 = sceGuSwapBuffers();
    }

    return 0;
}