# Hardware Stack — Soccer Robot

## Main Board
| ชิ้นส่วน | รายละเอียด |
|----------|------------|
| Board | POP32 |
| MCU | STM32F103CBT6 (Cortex-M3, 72MHz, ไม่มี FPU) |
| Framework | Arduino (via PlatformIO) |
| Flash | 128KB |
| RAM | 20KB |

> **หมายเหตุ:** STM32F103CBT6 ไม่มี FPU (Floating Point Unit) — การคำนวณ `sin/cos` ทำด้วย software ทั้งหมด ควรหลีกเลี่ยงการเรียก `sin/cos` ซ้ำซ้อนใน loop

---

## IMU
| ชิ้นส่วน | รายละเอียด |
|----------|------------|
| Sensor | MPU6050 |
| Interface | UART (Serial1, 115200 baud) |
| ข้อมูลที่ใช้ | Yaw angle (หน่วย: องศา × 100, format: `int16_t`) |

**Packet format (8 bytes):**
```
[0xAA][YAW_H][YAW_L][?][?][?][?][0x55]
```
- Byte 0: Header `0xAA`
- Byte 1-2: Yaw × 100 (big-endian int16)
- Byte 7: Footer `0x55`
- ค่าจะถูก normalize ให้อยู่ในช่วง −179.99° ถึง +179.99°

**Startup sequence (zeroYaw):**
```
0xA5 0x54  → set baud 115200
0xA5 0x55  → pitch correction
0xA5 0x52  → zero heading
```

---

## Motors
| หมายเลข | ใช้ทำอะไร | ชนิด |
|---------|-----------|------|
| Motor 1 | ล้อ Omni (holonomic) | Gearbox DC |
| Motor 2 | ล้อ Omni (holonomic) | Gearbox DC |
| Motor 3 | ล้อ Omni (holonomic) | Gearbox DC |
| Motor 4 | Kicker (ก้านยิงบอล) | Gearbox DC |

**Holonomic drive (3 ล้อ 120°):**
```
spd1 = vy·cos30 − vx·sin30 + ω   (Motor 1)
spd2 = −vy·cos30 − vx·sin30 + ω  (Motor 2)
spd3 = vx + ω                     (Motor 3)
```
- `sin30 = 0.5`, `cos30 = 0.866`
- input: speed (0–100), theta (องศา), omega (−100 ถึง 100)

**Kicker (Motor 4):**
- ยิง: `motor(4, -100)` นาน 150ms
- reload: วิ่งถอย + ตรวจ limit switch ที่ pin `A0`
- ถ้า limit switch ไม่ทริกใน 2 วินาที → ถือว่าก้านติด → ดันไปข้างหน้า 500ms แล้ว reload ใหม่

---

## Vision
| ชิ้นส่วน | รายละเอียด |
|----------|------------|
| Camera | HuskyLens Version 1 |
| Interface | I2C (Wire) |
| Library | `POP32_Huskylens` |
| Resolution | 320×240 px |
| Mode | Object tracking (Block mode) |

**Block ID ที่ใช้:**
| ID | วัตถุ | ใช้ใน |
|----|-------|-------|
| 1 | ลูกบอล | ทุก mode |
| 2 | ประตู ทีม Yellow | ATK-Y, DEF-Y |
| 3 | ประตู ทีม Blue | ATK-B, DEF-B |

**การอ่านค่า:**
```cpp
huskylens.blockInfo[ID][index].x   // pixel X (0–320)
huskylens.blockInfo[ID][index].y   // pixel Y (0–240, ยิ่งมาก = ยิ่งใกล้)
huskylens.blockSize[ID]            // จำนวน block ที่เห็นของ ID นั้น
```

---

## Analog Sensors (Wall Detection)
| Pin | Sensor | ทิศที่หนี |
|-----|--------|----------|
| analog(1) | bump sensor ด้านหน้า | 0° |
| analog(2) | bump sensor ด้านข้าง | 270° |
| analog(3) | bump sensor ด้านหลัง/ข้าง | 180° |

**Threshold:**
```cpp
int low[3]  = { 400, 390, 590 };
int high[3] = { 3800, 2700, 3800 };
int mid[3]  = { (low+high)/2 ... };   // ~2100, 1545, 2195
```

---

## Known Hardware Constraints

- **ไม่มี FPU** → ควรแทน `sin30`/`cos30` macro ด้วย constant `0.5f` / `0.8660254f`
- **HuskyLens v1 ความกว้างภาพ = 320px** → `goalAngle()` ต้องหารด้วย 320 ไม่ใช่ 360
- **MPU6050 ส่งข้อมูลผ่าน UART** → ต้องเรียก `getIMU()` หลายรอบใน loop เพื่อให้ได้ packet ครบ
- **Limit switch kicker อยู่ที่ pin A0** → `analogRead(A0) > 1000` = ก้านอยู่ตำแหน่ง home
