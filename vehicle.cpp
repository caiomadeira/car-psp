#include "vehicle.h"
#include "map.h"
#include "texture.h"

extern float PosVehicleX, PosVehicleZ, angleVehicle, speedVehicle;
extern int g_drawCalls, g_trisDrawn;

Objeto3D chassiModel;
Objeto3D wheelModel;


void LoadVehicleModels(void) {
    chassiModel.leObjeto("assets/tri/uno_chassi.tri", 0xFFFFFFFF);
    chassiModel.carreharTextura("assets/tex/props-texture.raw", 0xFFFFFFFF);
    wheelModel.leObjeto("assets/tri/wheel.tri", 0xFFFFFFFF);
    wheelModel.carregarTextura("assets/tex/props-texture.raw");
    sceKernelDcacheWritebackInvalidateAll();
}

/* ============================================================================
   FISICA
   ========================================================================= */
void UpdateVehiclePhysics(float dt, unsigned int buttons, int analogXRaw)
{
    /* ---- 1. entrada de direcao: -1 (esquerda) a +1 (direita) ---- */
    float steerInput = 0.0f;
    float analogX = (float)(analogXRaw - 128) / 128.0f;
    if (fabsf(analogX) > 0.2f) steerInput = -analogX;       /* zona morta */
    if (buttons & PSP_CTRL_LEFT)  steerInput =  1.0f;
    if (buttons & PSP_CTRL_RIGHT) steerInput = -1.0f;
 
    /* ---- 2. direcao progressiva: menos esterco em alta velocidade ----
       carros reais fazem isso. Sem isso, o carro fica intratavel rapido. */
    float vFrac = fabsf(speedVehicle) / CAR_MAX_SPEED;
    if (vFrac > 1.0f) vFrac = 1.0f;
    float steerMax = STEER_MAX * (1.0f - STEER_SPEED_K * vFrac);
 
    /* ---- 3. o volante gira com velocidade FINITA ----
       sem isto o esterco salta de 0 para o maximo num frame, e o carro
       da um tranco. E a auto-centragem reproduz o caster da suspensao. */
    float steerTarget = steerInput * steerMax;
    if (steerInput != 0.0f) {
        float d = steerTarget - steerAngle;
        float passo = STEER_RATE * dt;
        if (fabsf(d) <= passo) steerAngle = steerTarget;
        else                   steerAngle += (d > 0 ? passo : -passo);
    } else {
        float passo = STEER_RETURN * dt;
        if (fabsf(steerAngle) <= passo) steerAngle = 0.0f;
        else                            steerAngle -= (steerAngle > 0 ? passo : -passo);
    }
 
    /* ---- 4. acelerador / freio ---- */
    if (buttons & PSP_CTRL_CROSS) {
        speedVehicle += CAR_ACCEL * dt;
        if (speedVehicle > CAR_MAX_SPEED) speedVehicle = CAR_MAX_SPEED;
    } else if (buttons & PSP_CTRL_SQUARE) {
        speedVehicle -= CAR_BRAKE * dt;
        if (speedVehicle < -CAR_MAX_SPEED * 0.4f) speedVehicle = -CAR_MAX_SPEED * 0.4f;
    } else {
        /* atrito: desacelera sozinho */
        if (speedVehicle > 0)      speedVehicle = fmaxf(0.0f, speedVehicle - CAR_ACCEL*1.5f*dt);
        else if (speedVehicle < 0) speedVehicle = fminf(0.0f, speedVehicle + CAR_ACCEL*1.5f*dt);
    }
 
    /* ---- 5. MODELO BICICLETA ---- */
    if (speedVehicle != 0.0f)
    {
        float deltaRad = steerAngle * M_PI / 180.0f;
        float yawRate  = (speedVehicle / WHEELBASE) * tanf(deltaRad);   /* rad/s */
 
        /* gira PRIMEIRO, move depois (Euler semi-implicito): se inverter,
           o carro desliza para fora da curva e o erro se acumula */
        angleVehicle += yawRate * (180.0f / M_PI) * dt;
 
        float a    = angleVehicle * M_PI / 180.0f;
        float dist = speedVehicle * dt;
        float nextX = PosVehicleX + dist * sinf(a);
        float nextZ = PosVehicleZ + dist * cosf(a);
 
        /* rolagem das rodas: theta += d / r */
        wheelSpin += (dist / WHEEL_RADIUS) * (180.0f / M_PI);
        while (wheelSpin >= 360.0f) wheelSpin -= 360.0f;
        while (wheelSpin <    0.0f) wheelSpin += 360.0f;
 
        /* ---- 6. colisao com o mapa ---- */
        int gx = (int)roundf(nextX), gz = (int)roundf(nextZ);
        if (gx >= 0 && gx < mapWidth && gz >= 0 && gz < mapHeight
            && mapData[gz][gx] != 0) {
            PosVehicleX = nextX;
            PosVehicleZ = nextZ;
        } else {
            speedVehicle = 0.0f;
        }
    }
}
 
/* ============================================================================
   DrawVehicleWithWheels: chassi + 4 rodas
   ========================================================================= */
void DrawVehicleWithWheels(void)
{
    ScePspFVector3 pos = { PosVehicleX, 0.0f, PosVehicleZ };
    ScePspFVector3 scl = { CAR_SCALE, CAR_SCALE, CAR_SCALE };
 
    sceGumPushMatrix();
        /* transformacao do CARRO INTEIRO */
        sceGumTranslate(&pos);
        sceGumRotateY(angleVehicle * M_PI / 180.0f);
        sceGumScale(&scl);
 
        /* --- chassi --- */
        chassiModel.desenha();
        g_drawCalls++;
        g_trisDrawn += chassiModel.getNFaces();
 
        /* --- as 4 rodas, em espaco LOCAL do carro ---
           como estamos DENTRO do bloco escalado, as posicoes da tabela
           (que estao no espaco do modelo) sao usadas diretamente. */
        for (int i = 0; i < 4; i++)
        {
            sceGumPushMatrix();
                ScePspFVector3 wp = { WHEELS[i].x, WHEELS[i].y, WHEELS[i].z };
                sceGumTranslate(&wp);
 
                /* esterco: so as dianteiras, em torno do eixo VERTICAL */
                if (WHEELS[i].steers)
                    sceGumRotateY(steerAngle * M_PI / 180.0f);
 
                /* rolagem: todas, em torno do eixo DA RODA.
                   RotateY antes de RotateX faz a roda esterçada rolar no
                   eixo certo -- as matrizes se aplicam de baixo para cima. */
                sceGumRotateX(wheelSpin * M_PI / 180.0f);
 
                wheelModel.desenha();
                g_drawCalls++;
                g_trisDrawn += wheelModel.getNFaces();
            sceGumPopMatrix();
        }
    sceGumPopMatrix();
}