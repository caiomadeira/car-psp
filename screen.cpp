#include "screen.h"
#include <string.h>

char loadingLog[16][64];
int loadingLines = 0;


void LoadingScreen(const char* msg, void* list, void*& fbp0)
{
    if (loadingLines < 16) {
        strncpy(loadingLog[loadingLines], msg, 63);
        loadingLog[loadingLines][63] = '\0';
        loadingLines++;
    }

    sceGuStart(GU_DIRECT, list);
    sceGuClearColor(0xFF000000);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    sceGuFinish();
    sceGuSync(0, 0);

    pspDebugScreenSetOffset((int)fbp0);
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("=== CARREGANDO JOGO ===\n\n");
    for (int i = 0; i < loadingLines; i++)
        pspDebugScreenPrintf("  %s\n", loadingLog[i]);

    sceDisplayWaitVblankStart();
    fbp0 = sceGuSwapBuffers();
}

int Menu(void* list, void*& fbp0)
{
    int selected = 0;
    const char* items[2] = {"Play", "Quit"};
    SceCtrlData pad, oldPad;
    sceCtrlReadBufferPositive(&oldPad, 1);
    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        unsigned int pressed = pad.Buttons & ~oldPad.Buttons;
        oldPad = pad;

        if (pressed & (PSP_CTRL_UP | PSP_CTRL_DOWN)) selected ^= 1;
        if (pressed & PSP_CTRL_CROSS)
            return (selected == 0) ? STATE_PLAYING : STATE_QUIT;


        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(0xFF000000);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
        Begin2D();
        DrawFullScreen(bgTex);

        DrawText2D("Car Game Psp", 150, 60, 3.0f, 0xFFFFFFFF);


        for (int i = 0; i < 2; i++) {
            unsigned int cor = (i == selected) ? 0xFF00FFFF : 0xFFAAAAAA;
            float px = (i == selected) ? 180 : 200;       // item ativo desloca
            DrawText2D(items[i], px, 150 + i * 40, 2.0f, cor);
        }

        DrawText2D("up/down: move  X: select", 120, 250, 1.0f, 0xFFCCCCCC);
        End2D();
        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        fbp0 = sceGuSwapBuffers();
    }
}