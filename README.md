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
| 3 | DEF-Yellow | Defender — Yellow team, 4.6s delayed start |
| 4 | DEF-Blue | Defender — Blue team, 4.6s delayed start |
| 5 | GK | Goalkeeper |
| 6 | P | Penalty kick |

---

## Game Modes

| Mode | Description |
|------|-------------|
| **Attacker** | Chases ball, orbits to align body with goal, drives in and shoots |
| **Defender** | Same as attacker with different tuning, starts 4.6s late |
| **Goalkeeper** | Slides left/right only to track ball X axis, always faces forward |
| **Penalty** | Aligns body straight ahead, charges full speed and shoots |

---

## Files

| File | Purpose |
|------|---------|
| `Soccer.ino` | Globals, #defines, enums — compiled first |
| `Hardware.ino` | IMU, drive, kicker, shared helpers, setup/loop |
| `Play.ino` | Attacker/Defender state machine |
| `GoalKeeper.ino` | Goalkeeper state machine |
| `PenaltyKick.ino` | Penalty kick state machine |
| `UI.ino` | OLED display screens |
| `docs/HARDWARE.md` | IMU protocol reference, wiring, hardware constraints |
| `docs/BUGFIX.md` | Bug fix history |

---

## Tuning Parameters

### Soccer.ino — Shared

| Parameter | Default | Effect |
|-----------|---------|--------|
| `sp_rot` | 160 | Ball center X setpoint (camera 0–320 px) |
| `spFli` | 220 | Ball approach Y setpoint — higher = closer to ball |
| `rotErrorGap` | 30 px | X tolerance before state transitions |
| `flingErrorGap` | 12 px | Y tolerance before state transitions |
| `alignErrorGap` | 4° | Yaw tolerance before entering APPROACH |
| `goalFli` | 23 px | Min goal Y (size) to attempt shot |
| `minApproachSpd` | 40 | Min motor speed to overcome dead zone |
| `idleSpd` | 65 | Spin speed when searching for ball |
| `rot_Kp / rot_Kd` | 0.6 / 0.6 | Rotation PID gains |
| `fli_Kp` | 0.9 | Approach PID gain |
| `head_Kp` | 1.5 | Heading hold PID gain |

### Soccer.ino — Vision Filters

| Parameter | Default | Effect |
|-----------|---------|--------|
| `ballSizeMin` | 10 px | Reject blocks smaller than this (noise / too far) |
| `ballSizeMax` | 200 px | Reject blocks larger than this (false positive up close) |
| `ballMaxJump` | 80 px | Reject position jumps larger than this per frame |
| `ballConfirmMin` | 2 frames | Consecutive detections required before accepting as ball |
| `ballMissMax` | 3 frames | Hold last ball position this many frames after losing sight |
| `goalMissMax` | 5 frames | Hold last goal position this many frames after losing sight |

### Play.ino — Per-mode presets (in Hardware.ino setup)

| Parameter | Effect |
|-----------|--------|
| `goalID` | Which HuskyLens block ID is the target goal (2=Yellow, 3=Blue) |
| `orbitRadius` | Orbit circle size around ball during ALIGN — higher = wider arc |
| `driveSpeed` | Speed driving toward goal in APPROACH |
| `steerScale` | How aggressively to steer toward goal center |
| `shootDist` | Goal Y threshold to trigger shoot — higher = shoot from farther |

| Mode | goalID | orbitRadius | driveSpeed | steerScale | shootDist |
|------|--------|------------|-----------|-----------|----------|
| ATK-Yellow | 2 | 1.4 | 40 | 1.5 | 15 |
| ATK-Blue | 3 | 1.2 | 60 | 1.5 | 20 |
| DEF-Yellow | 2 | 1.2 | 60 | 1.2 | 13 |
| DEF-Blue | 3 | 1.2 | 40 | 1.2 | 13 |

---

## Wiring

| Part | Interface | Pin |
|------|-----------|-----|
| MPU6050 | UART | Serial1 (115200 baud) |
| HuskyLens | I2C | SDA / SCL |
| Kicker limit switch | Analog | A0 |
| IR sensor front | Analog | analog(1) |
| IR sensor side | Analog | analog(2) |
| IR sensor rear | Analog | analog(3) |
| Wheels | PWM | motor(1–3) |
| Kicker | PWM | motor(4) |

---

## HuskyLens Object IDs

| ID | Object | Used by |
|----|--------|---------|
| 1 | Ball | All modes |
| 2 | Yellow goal | ATK-Yellow, DEF-Yellow |
| 3 | Blue goal | ATK-Blue, DEF-Blue |

---

## Hardware Improvement Roadmap

Ordered by impact vs effort.

### 1. Camera angle — tilt down 10–15°
Cut background clutter (audience, objects outside field) from HuskyLens field of view.
Before: camera level → sees background. After: tilted down → sees only field.
**Impact: high / Effort: low**

### 2. Hood / baffle around lens
Black plastic or cardboard shield around the lens — limits side and top field of view, cuts glare from overhead lights.
**Impact: medium / Effort: low**

### 3. LED ring around camera
White LED ring around HuskyLens — provides consistent illumination on the ball regardless of ambient light. Makes detection stable across indoor/outdoor environments.
**Impact: high / Effort: medium**

### 4. Anti-vibration camera mount
Rubber or foam between camera and chassis — reduces motion blur from motor vibration, especially after wall bounces.
**Impact: medium / Effort: low**
