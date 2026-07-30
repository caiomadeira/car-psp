#ifndef VEHICLE_H
#define VEHICLE_H

#include "common.h"
#include "object3d.h"

#define CAR_SCALE 0.35f
#define WHEELBASE_MODEL 1.2643f
#define WHEEL_RADIUS_MOD 0.1850f
#define WHEELBASE (WHEELBASE_MODEL * CAR_SCALE)
#define WHEEL_RADIUS (WHEEL_RADIUS_MOD * CAR_SCALE)

// direcao ------------
#define STEER_MAX 20.0f
#define STEER_RATE 120.0f
#define STEER_RETURN 180.0f
#define STEER_SPEED_K 0.45f

// motor - aceleracao, freio, velocidade etc
#define CAR_MAX_SPEED 4.0f
#define CAR_ACCEL 3.0f
#define CAR_BRAKE 6.0f

struct WheelDef {
    float x, y, z;
    int steers;
};

static const WheelDef WHEELS[4] = {
    { -0.3143f, +0.3793f, +0.6931f, 1 },   /* dianteira esquerda -- esterca */
    { +0.4768f, +0.3709f, +0.6931f, 1 },   /* dianteira direita  -- esterca */
    { -0.3297f, +0.3847f, -0.5712f, 0 },   /* traseira esquerda             */
    { +0.4782f, +0.3866f, -0.5712f, 0 },   /* traseira direita              */
};

// state --------
extern float steerAngle;
extern float wheelSpin;

extern Objeto3D chassiModel;
extern float wheelSpin;

extern Objeto3D chassiModel;
extern Objeto3D wheelSpin;

void LoadVehicleModels(void);
void UpdateVehiclePhysics(float dt, unsigned int buttons, int analogX);
void DrawVehicleWithWheels(void);

#endif