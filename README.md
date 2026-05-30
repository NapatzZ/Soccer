# Soccer Robot — README

**Platform:** POP32 (STM32F103CBT6) · **Vision:** HuskyLens v1 · **IMU:** MPU6050 · **Drive:** 3-wheel Holonomic · **Kicker:** Motor 4

---

## การเลือก Mode (Startup)

เมื่อเปิดเครื่อง หุ่นจะเข้าสู่ขั้นตอนนี้ตามลำดับ:

```
1. รอกด SW_A เพื่อเริ่ม (พร้อม test shoot ได้)
2. reload ก้านยิงให้อยู่ตำแหน่ง home
3. รอ HuskyLens connect
4. รอกด SW_B → เริ่ม calibrate IMU (Auto_zero)
5. หมุน knob เลือก mode → กด SW_OK เพื่อยืนยัน
```

| Knob | Mode | ฟังก์ชัน | หมายเหตุ |
|------|------|----------|---------|
| 0 | READZX | แสดงค่า sensor analog 1–3 | ใช้ calibrate bump sensor |
| 1 | ATK-Yellow | `FootballYellow1()` | Attacker ทีมเหลือง (ประตู ID 2) |
| 2 | ATK-Blue | `FootballYellow2()` | Attacker ทีมน้ำเงิน (ประตู ID 3) |
| 3 | DEF-Yellow | `FootballYellow3()` | Defender ทีมเหลือง (delay 4.7 วิ) |
| 4 | DEF-Blue | `FootballYellow4()` | Defender ทีมน้ำเงิน (delay 4.7 วิ) |
| 5 | GK | `GOAL()` | Goalkeeper |
| 6 | P | `PENALTY()` | Penalty kick |

> DEF mode มี delay 4.7 วินาทีก่อนเริ่ม เพื่อให้ Attacker เข้าตำแหน่งก่อน

---

## Attacker — `FootballYellow1()` / `FootballYellow2()`

**บทบาท:** วิ่งหาบอล → ดักบอลให้อยู่ด้านหลัง → หันท้ายหุ่นหาประตู → วิ่งเข้าชนยิง

### State Machine

```
┌─────────────────────────────────────────────────────────────────┐
│  ทุก loop: ตรวจ bump sensor → ถ้าชนขอบ → วิ่งหนีทิศตรงข้าม     │
└─────────────────────────────────────────────────────────────────┘
                          ↓
          ┌───────────────────────────────┐
          │  มองเห็นบอล (HuskyLens ID 1)?  │
          └───────────────────────────────┘
               ↙ ใช่                  ↘ ไม่
    ┌──────────────────┐       ┌────────────────────┐
    │  discoveState=1  │       │  หมุนหาบอล          │
    │  (ไปหาบอล)       │       │  ทิศตาม lastYaw     │
    └──────────────────┘       └────────────────────┘
           ↓
    ┌─────────────────────────────────────────────────────┐
    │  PID เข้าหาบอล                                       │
    │  • rot PID: ปรับหมุนให้บอลอยู่กลางกล้อง (X = 155)   │
    │  • fli PID: ปรับระยะให้บอลอยู่ใกล้พอ (Y = 220)      │
    │  → holonomic(fli_spd, 90°, rot_w)                   │
    └─────────────────────────────────────────────────────┘
           ↓ บอลอยู่กลาง + ใกล้พอ
    ┌──────────────────────────────────────────────────────┐
    │  discoveState=0 (เข้าสู่ align phase)                 │
    │  หมุนตัวให้ท้ายหุ่นหาบอล โดยใช้ lastYaw             │
    │  → holonomic(55, vecCurve, radCurve)                 │
    └──────────────────────────────────────────────────────┘
           ↓ yaw < alignErrorGap (4°) + บอลอยู่กลาง
    ┌──────────────────────────────────────────────────────┐
    │  เห็นประตูหรือไม่?                                    │
    │  • เห็น (goalY ≥ 23): คำนวณ goalAngle → วิ่งเข้าหา  │
    │    ถ้า goalY ≥ 36: shoot() + reload()                │
    │  • ไม่เห็น: heading(90, 0) → เดินไปข้างหน้า 555ms   │
    └──────────────────────────────────────────────────────┘
```

### พารามิเตอร์ที่ปรับได้

| ตัวแปร | ค่า | ความหมาย |
|--------|-----|----------|
| `sp_rot` | 155 | pixel X ที่ถือว่าบอลอยู่กลางกล้อง |
| `spFli` | 220 | pixel Y ที่ถือว่าบอลอยู่ใกล้พอ (0=ไกล, 240=ชิด) |
| `rotErrorGap` | 15 | tolerance X (px) ก่อนเปลี่ยน state |
| `flingErrorGap` | 12 | tolerance Y (px) ก่อนเปลี่ยน state |
| `alignErrorGap` | 4 | tolerance yaw (°) ก่อนยิง |
| `goalFli` | 23 | pixel Y ขั้นต่ำที่ต้องเห็นประตูก่อนยิง |
| `rot_Kp/Kd` | 0.6 / 0.6 | PID หมุนหาบอล |
| `fli_Kp` | 0.9 | PID เข้าหาบอล |
| `idleSpd` | 65 | ความเร็วหมุนเมื่อไม่เห็นบอล |

---

## Defender — `FootballYellow3()` / `FootballYellow4()`

**บทบาท:** เหมือน Attacker แต่เริ่มช้ากว่า 4.7 วินาที และ tune ค่า gain ต่างกันเล็กน้อย

| ความแตกต่าง | Attacker | Defender |
|------------|----------|---------|
| radCurve multiplier | ×1.2 (ATK-B) | ×1.2 |
| holonomic speed ตอนยิง | 60 | 40–60 |
| goalFli threshold | +20, +15 | +13 |
| delay ก่อนเริ่ม | ไม่มี | 4700ms |

> Logic state machine เหมือน Attacker ทุกอย่าง — ดูหัวข้อ Attacker ด้านบน

---

## Goalkeeper — `GOAL()`

**บทบาท:** ยืนรักษาเส้นประตู ติดตามบอลในแกน X เท่านั้น ไม่วิ่งเข้าหาบอล

### Control Flow

```
ทุก loop:
  1. ตรวจ bump sensor → วิ่งหนีถ้าชนขอบ
  2. มองเห็นบอล?
       ใช่ → คำนวณ rot_error = sp_rot - ballPosX
              rot_w = rot_error × rot_Kp
              dir = (rot_w > 0 ? 180° : 0°)   ← ทิศที่วิ่ง
              heading(speed, dir, 0)           ← วิ่งซ้าย/ขวาตามบอล
       ไม่  → holonomic(0, 0, idleSpd)        ← หมุนหาบอล
```

**ข้อสังเกต:** goalkeeper ใช้แค่ P-control (ไม่มี I, D) และเคลื่อนที่เฉพาะแกน X (ซ้าย-ขวา) ไม่เข้าหาบอล

---

## Penalty Kick — `PENALTY()`

**บทบาท:** จัดตำแหน่งท้ายหุ่นหาบอลแล้ววิ่งชนยิงตรงๆ (ไม่ต้องหาประตู)

### Control Flow

```
ทุก loop:
  1. ตรวจ bump sensor
  2. มองเห็นบอล?
       ใช่ → PID เข้าหาบอล (เหมือน Attacker phase 1)
              เมื่อบอลอยู่กลาง + yaw ตรง:
                holonomic(90, 90°, 0)  ← วิ่งเข้าหาบอลเต็มสปีด 300ms
                shoot()
                reload()
                wheel(0,0,0)
                waitSW_A_bmp()         ← รอกดปุ่มก่อน penalty ลูกถัดไป
       ไม่  → หมุนหาบอล
```

**ความแตกต่างจาก Attacker:** ไม่ต้องหาประตู — วิ่งตรงเข้าหาบอลเลย (สำหรับ penalty ที่รู้ทิศอยู่แล้ว)

---

## ฟังก์ชัน Support

### `holonomic(spd, theta, omega)`
ขับ 3 ล้อ omni ให้วิ่งทิศทางใดก็ได้

```
theta = 0°   → วิ่งขวา
theta = 90°  → วิ่งขึ้น (หน้าหุ่น)
theta = 180° → วิ่งซ้าย
theta = 270° → วิ่งลง (หลังหุ่น)
omega > 0    → หมุนซ้าย
omega < 0    → หมุนขวา
```

### `heading(spd, theta, spYaw)`
เหมือน `holonomic()` แต่มี PID รักษาทิศ IMU ไว้ที่ `spYaw` ระหว่างวิ่ง

### `getIMU()` → `bool`
อ่าน yaw จาก MPU6050 ผ่าน UART — ต้องเรียกหลายรอบเพราะ packet มา 8 bytes ทีละ byte

### `shoot()`
สั่ง Motor 4 ยิงบอล (150ms) แล้วหยุด

### `reload()`
ดึงก้านยิงกลับ home โดยตรวจ limit switch ที่ A0 มี timeout 2 วินาทีและ retry อัตโนมัติถ้าก้านติด

### `goalAngle(goalX, goalY, isFront)` → องศา
แปลง pixel X ของประตู (0–320) เป็นองศาทิศทาง holonomic (0°–180°)

---

## HuskyLens Block ID

| ID | วัตถุ | ใช้ใน |
|----|-------|-------|
| 1 | ลูกบอล | ทุก mode |
| 2 | ประตูทีมเหลือง | ATK-Yellow, DEF-Yellow |
| 3 | ประตูทีมน้ำเงิน | ATK-Blue, DEF-Blue |

---

## Wiring Summary

| ชิ้นส่วน | Interface | Pin |
|----------|-----------|-----|
| MPU6050 (IMU) | UART | Serial1 (115200 baud) |
| HuskyLens | I2C | Wire (SDA/SCL) |
| Kicker limit switch | Analog | A0 |
| Bump sensor ด้านหน้า | Analog | analog(1) |
| Bump sensor ด้านข้าง | Analog | analog(2) |
| Bump sensor ด้านหลัง | Analog | analog(3) |
| Motor 1–3 (ล้อ) | PWM | motor(1–3) |
| Motor 4 (kicker) | PWM | motor(4) |
