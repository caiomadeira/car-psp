#include "common.h"

float g_fps = 0.0f;
float g_cpuMs = 0.0f;
float g_gpuMs = 0.0f;
int g_frames = 0;
u64 g_fpsTick = 0;

// callback classico pra sair do jogo apertando o botão home do PSP
int exit_callback(int arg1, int arg2, void *common) { sceKernelExitGame(); return 0; }
int CallbackThread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

void SetupCallbacks(void) {
    int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if(thid >= 0) sceKernelStartThread(thid, 0, 0);
}

void ProfilerUpdateFPS(void) {
    u64 now;
    sceRtcGetCurrentTick(&now);
    float res = (float)sceRtcGetTickResolution();
    if (g_fpsTick == 0) {
        g_fpsTick = now;
        return;
    }

    g_frames++;
    float elapsed = (float)(now - g_fpsTick) / res;
    if (elapsed >= 0.5f) {
        g_fps = g_frames / elapsed;
        g_frames = 0;
        g_fpsTick = now;
    }
}