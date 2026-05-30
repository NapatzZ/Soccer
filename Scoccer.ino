// Soccer Robot — RobotKub
// Board  : POP32 (STM32F103CBT6)
// IMU    : MPU6050 via UART Serial1
// Camera : HuskyLens v1 via I2C
// Motors : 3x Omni (holonomic) + 1x Kicker
//
// Required libraries (install via Arduino Library Manager):
//   - POP32
//   - POP32_Huskylens
//
// Sketch tabs:
//   WSTPA03.ino  — setup, hardware functions (holonomic, IMU, shoot)
//   UI.ino       — OLED screen helpers
//   ATK-Y.ino    — Attacker Yellow  (FootballYellow1)
//   ATK-B.ino    — Attacker Blue    (FootballYellow2)
//   DEFY.ino     — Defender Yellow  (FootballYellow3)
//   DEFB.ino     — Defender Blue    (FootballYellow4)
//   GOAL.ino     — Goalkeeper       (GOAL)
//   PENALTY.ino  — Penalty kick     (PENALTY)
