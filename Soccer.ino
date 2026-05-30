// Soccer Robot — RobotKub
// Board  : POP32 (STM32F103CBT6)
// Camera : HuskyLens v1 (I2C)
// IMU    : MPU6050 (UART Serial1)
// Drive  : 3-wheel Holonomic + Motor 4 kicker
//
// ── This file is compiled FIRST by Arduino IDE ───────────────
// All shared #includes, #defines, and globals live here so
// every other .ino tab can see them regardless of sort order.
// ─────────────────────────────────────────────────────────────

#include <POP32.h>
#include <POP32_Huskylens.h>

POP32_Huskylens huskylens;

// ── Drive ─────────────────────────────────────────────────────
#define degToRad    0.0174f
const float sin30 = 0.5f;
const float cos30 = 0.8660254f;

// ── Wall sensors ──────────────────────────────────────────────
int low[3]  = { 400, 390, 590 };
int high[3] = { 3800, 2700, 3800 };
int mid[3]  = { (low[0]+high[0])/2, (low[1]+high[1])/2, (low[2]+high[2])/2 };

// ── Rotation PID (ball X centering) ──────────────────────────
#define rot_Kp      0.6
#define rot_Ki      0.0
#define rot_Kd      0.6
#define sp_rot      160
#define rotErrorGap 30
#define idleSpd     65
float rot_error, rot_pError, rot_i, rot_d, rot_w;

// ── Approach PID (ball Y distance) ───────────────────────────
#define fli_Kp        0.9
#define fli_Ki        0.0
#define fli_Kd        0.0
#define flingErrorGap 12
float spFli   = 220;
float goalFli = 23;
float fli_error, fli_pError, fli_i, fli_d, fli_spd;

// ── Heading PID (yaw hold) ────────────────────────────────────
#define alignErrorGap 4
#define head_Kp       1.5f
#define head_Ki       0.0f
#define head_Kd       0.0f
float head_error, head_pError, head_w, head_d, head_i;

// ── Shared vision state ───────────────────────────────────────
int   ballPosX, ballPosY;
int   goalX, goalY;

// ── IMU ───────────────────────────────────────────────────────
float   pvYaw, lastYaw;
uint8_t rxCnt = 0, rxBuf[8];

// ── Kicker ────────────────────────────────────────────────────
#define limPin    A0
#define reloadSpd -100

