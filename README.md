# Soccer Robot

**Board:** POP32 (STM32F103CBT6) · **Camera:** HuskyLens v1 · **IMU:** MPU6050 · **Drive:** 3-wheel Holonomic · **Kicker:** Motor 4

**Required libraries:** POP32, POP32_Huskylens

---

## Startup

1. Power on → wait for HuskyLens
2. Press **SW_A** to test kicker, **SW_B** to start IMU calibration
3. Turn knob to select mode → press **SW_OK**

| Knob | Mode | What it does |
|------|------|-------------|
| 0 | READZX | Show raw analog sensor values (for calibration) |
| 1 | ATK-Yellow | Attacker — Yellow team (goal ID 2) |
| 2 | ATK-Blue | Attacker — Blue team (goal ID 3) |
| 3 | DEF-Yellow | Defender — Yellow team, 4.7s delayed start |
| 4 | DEF-Blue | Defender — Blue team, 4.7s delayed start |
| 5 | GK | Goalkeeper |
| 6 | P | Penalty kick |

---

## Game Modes

| Mode | Description |
|------|-------------|
| **Attacker** | Chases ball, aligns body to goal, drives in and shoots |
| **Defender** | Same as attacker with different tuning, starts 4.7s late |
| **Goalkeeper** | Slides left/right to track ball on the X axis only |
| **Penalty** | Aligns to ball then charges straight in to shoot |

---

## Files

| File | Purpose |
|------|---------|
| `WSTPA03.ino` | Setup, hardware functions (drive, IMU, kicker) |
| `Play.ino` | Unified attacker/defender state machine |
| `GoalKeeper.ino` | Goalkeeper state machine |
| `PenaltyKick.ino` | Penalty kick state machine |
| `UI.ino` | OLED display screens |
| `Scoccer.ino` | Sketch entry point (Arduino IDE) |

---

## Tuning Parameters

| Parameter | Default | Effect |
|-----------|---------|--------|
| `sp_rot` | 155 | Ball center X setpoint (camera 0–320 px) |
| `spFli` | 220 | Ball approach Y setpoint (closer = higher) |
| `rotErrorGap` | 15 px | X tolerance before state change |
| `flingErrorGap` | 12 px | Y tolerance before state change |
| `alignErrorGap` | 4° | Yaw tolerance before shooting |
| `goalFli` | 23 px | Min goal Y to attempt shot |
| `rot_Kp / rot_Kd` | 0.6 / 0.6 | Rotation PID gains |
| `fli_Kp` | 0.9 | Approach PID gain |
| `idleSpd` | 65 | Spin speed when no ball visible |

---

## Wiring

| Part | Interface | Pin |
|------|-----------|-----|
| MPU6050 | UART | Serial1 (115200 baud) |
| HuskyLens | I2C | SDA / SCL |
| Kicker limit switch | Analog | A0 |
| Bump sensor front | Analog | analog(1) |
| Bump sensor side | Analog | analog(2) |
| Bump sensor rear | Analog | analog(3) |
| Wheels (Motor 1–3) | PWM | motor(1–3) |
| Kicker (Motor 4) | PWM | motor(4) |

---

## HuskyLens Object IDs

| ID | Object | Used by |
|----|--------|---------|
| 1 | Ball | All modes |
| 2 | Yellow goal | ATK-Yellow, DEF-Yellow |
| 3 | Blue goal | ATK-Blue, DEF-Blue |
