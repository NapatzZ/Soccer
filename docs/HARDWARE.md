# Hardware Reference — Soccer Robot

---

## Main Board

| Part | Detail |
|------|--------|
| Board | POP32 |
| MCU | STM32F103CBT6 (Cortex-M3, 72MHz, no FPU) |
| Framework | Arduino (via POP32 library) |
| Flash | 128KB |
| RAM | 20KB |

> STM32F103CBT6 has no FPU — all `sin/cos` is computed in software. Avoid redundant trig calls in hot loops.

---

## IMU — MPU6050 via UART

| Part | Detail |
|------|--------|
| Chip | MPU6050 (inside a UART module) |
| Interface | UART — Serial1, 115200 baud |
| Output | Pre-computed yaw angle (no sensor fusion needed in code) |

### Pinout

| Pin | Description |
|-----|-------------|
| VCC | 3–5V |
| GND | Ground |
| TX  | Module TX -> Arduino RX |
| RX  | Module RX -> Arduino TX |

### UART Settings

| Parameter | Value |
|-----------|-------|
| Baud rate | 115200 bps |
| Data bits | 8 |
| Parity    | None |
| Stop bits | 1 |

### Command Bytes (0xA5 series)

| Command        | Effect |
|----------------|--------|
| `0xA5` `0x54` | Pitch/Roll correction |
| `0xA5` `0x55` | Heading correction (zero yaw) |
| `0xA5` `0x52` | Auto output — continuous binary stream |
| `0xA5` `0x51` | Query mode — one packet per request |

### Packet Format (8 bytes)

```
[0xAA] [Yaw_H] [Yaw_L] [Pitch_H] [Pitch_L] [Roll_H] [Roll_L] [0x55]
```

| Byte | Content |
|------|---------|
| 0    | `0xAA` header |
| 1–2  | Yaw × 100, int16 big-endian |
| 3–4  | Pitch × 100 |
| 5–6  | Roll × 100 |
| 7    | `0x55` footer |

Only Yaw (bytes 1–2) is used in this project.

**Why 2 bytes for one angle?**
UART sends 1 byte (0–255) at a time. An angle like 123.45° is stored as 12345 (×100), which exceeds 255 and needs 16 bits:

```cpp
int16_t raw = (int16_t)(rxBuf[1] << 8 | rxBuf[2]);
float yaw   = raw / 100.0f;   // -> degrees
```

### Specs

| Parameter    | Value |
|--------------|-------|
| Range        | -180° to +180° |
| Resolution   | 0.01° |
| Update rate  | 100 Hz |
| Current      | 15 mA |
| Temperature  | -20 to 85°C |

### How zeroYaw works

`zeroYaw()` sends 3 command pairs over Serial1. The module sets the current heading as 0°.
`Auto_zero()` then polls `getIMU()` until `pvYaw < 0.05°` to confirm the reset has settled.
If it takes more than 5s, `zeroYaw()` is retried.

`Serial1.begin()` is called once inside `zeroYaw()` — `getIMU()` calls `Serial1.read()` directly with no need to begin again.

---

## Motors

| Number | Purpose | Type |
|--------|---------|------|
| Motor 1 | Omni wheel (holonomic) | Gearbox DC |
| Motor 2 | Omni wheel (holonomic) | Gearbox DC |
| Motor 3 | Omni wheel (holonomic) | Gearbox DC |
| Motor 4 | Kicker | Gearbox DC |

### Holonomic Drive (3-wheel, 120°)

```
spd1 =  vy·cos30 − vx·sin30 + ω
spd2 = −vy·cos30 − vx·sin30 + ω
spd3 =  vx + ω
```

- `sin30 = 0.5f`, `cos30 = 0.8660254f`
- Inputs: speed (0–100), theta (degrees), omega (−100 to 100)

### Kicker (Motor 4)

- Shoot: `motor(4, -100)` for 150ms
- Reload: run motor + wait for limit switch at pin `A0` (`analogRead(A0) > 1000`)
- If limit switch doesn't trigger within 2s -> jam recovery: reverse 500ms then reload again

---

## Vision — HuskyLens v1

| Part | Detail |
|------|--------|
| Camera | HuskyLens Version 1 |
| Interface | I2C (Wire, 400kHz) |
| Library | `POP32_Huskylens` |
| Resolution | 320×240 px |
| Mode | Object tracking (Block mode) |

### Block IDs

| ID | Object | Used by |
|----|--------|---------|
| 1 | Ball | All modes |
| 2 | Yellow goal | ATK-Yellow, DEF-Yellow |
| 3 | Blue goal | ATK-Blue, DEF-Blue |

### Reading Values

```cpp
huskylens.blockInfo[ID][0].x   // pixel X (0–320)
huskylens.blockInfo[ID][0].y   // pixel Y (0–240, higher = closer)
huskylens.blockSize[ID]        // number of blocks visible for this ID
```

---

## Analog Sensors (Wall Detection)

| Pin | Sensor | Escape direction |
|-----|--------|-----------------|
| analog(1) | Front bump | 0° |
| analog(2) | Side bump | 270° |
| analog(3) | Rear bump | 180° |

```cpp
int low[3]  = { 400, 390, 590 };
int high[3] = { 3800, 2700, 3800 };
int mid[3]  = { ~2100, ~1545, ~2195 };
```

---

## Known Constraints

| Constraint | Impact |
|-----------|--------|
| No FPU | All float math is software — avoid redundant `sin/cos` in loops |
| HuskyLens 320px wide | `goalAngle()` divides by 320, not 360 |
| MPU6050 over UART | Call `getIMU()` in a loop (up to 8×) to assemble a full packet |
| Kicker limit switch at A0 | `analogRead(A0) > 1000` = arm at home position |
