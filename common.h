#ifndef COMMON_H
#define COMMON_H

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psprtc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern float g_fps;
extern float g_cpuMs;
extern float g_gpuMs;
extern int g_frames;
extern u64 g_fpsTick;

void ProfilerUpdateFPS(void);
void SetupCallbacks(void);
#define print pspDebugScreenPrintf

#endif

/*
GU (sceGu) - baixo nivel - estado da GPU, texturas, memoria, envio de comandos.
GUM (sceGum) - eh o utility math. lida com matrizes(view, model, projection)
e eh equivalente ao opengl.
*/

// // --- constantes ---
// #define GU_DEPTH_TEST GL_DEPTH_TEST 
// #define GU_COLOR_BUFFER_BIT GL_COLOR_BUFFER_BIT
// #define GU_DEPTH_BUFFER_BIT GL_DEPTH_BUFFER_BIT 

// // --- funcoes ---
// #define sceGuEnable glEnable 
// #define sceGuClearColor glClearColor
// #define sceGuClearDepth glClearDepth
// #define sceGuClear glClear

// #define sceGumMatrixMode glMatrixMode
// #define sceGumLoadIdentity glLoadIdentity
// #define sceGumPushMatrix glPushMatrix
// #define sceGumPopMatrix glPopMatrix
// #define sceGumPerspective gluPerspective
// #define sceGumOrtho glOrtho
// #define sceGumLookAt gluLookAt

// #define sceGumTranslate glTranslatef // precisa criar uma ScePspFVector3 pos = {x, y, z}; e passar o endereço &pos.
// // O PSP tem funções dedicadas para cada eixo. Atenção: O OpenGL usa graus, o PSP usa radianos.
// #define sceGumRotateX glRotateX  // Essa separacao entre X Y Z nao existe, eu quem estou criando
// #define sceGumRotateY glRotateY
// #define sceGumRotateZ glRotateZ
// #define sceGumScale glScalef

// #define sceGumDrawArray glDrawArrays
// #define sceGuShadeModel glShadeModel
// #define sceGuFrontFace glFrontFace
// #define sceGuTexMode glBindTexture
// #define sceGuTexImage glBindImage
// #define sceGuTexFilter glTexParameteri
// #define sceGuTexWrap glTexParameteri
// #define sceGuTexFunc glTexEnvf

// #define sceGuLight glLightfv
// #define sceGuMaterial glMaterialfv