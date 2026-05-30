# Bug Fix Report — Soccer Robot
> อ้างอิง hardware stack: `STACK.md`

---

## รอบที่ 1 — General Code Review

### [FIX-01] `rot_d` Dead Code — PID D-term ไม่ทำงาน
**ไฟล์:** `ATK-Y.ino`, `ATK-B.ino`, `DEFY.ino`, `DEFB.ino`, `PENALTY.ino`

บรรทัดที่ 1–2 สะสมค่า `rot_d` แต่ถูกบรรทัดที่ 3 เขียนทับทันที ทำให้ไม่มีผลใดๆ

```cpp
// ก่อนแก้
rot_d = rot_d + rot_error;
rot_d = constrain(rot_d, -100, 100);
rot_d = rot_error - rot_pError;   // เขียนทับ 2 บรรทัดข้างบน

// หลังแก้
rot_d = rot_error - rot_pError;
```

---

### [FIX-02] Menu Index Out-of-Bounds
**ไฟล์:** `WSTPA03.ino`

`menu[k-1]` เมื่อ `k=0` → `menu[-1]` และ `menu[k+1]` เมื่อ `k=6` → `menu[7]` ออกนอกขอบเขต array

```cpp
// ก่อนแก้
oled.text(0, 1, menu[k-1]);
oled.text(5, 1, menu[k+1]);

// หลังแก้
if (k > 0) oled.text(0, 1, menu[k-1]);
if (k < 6) oled.text(5, 1, menu[k+1]);
```

---

### [FIX-03] `holonomic(60, 370, 0)` มุมผิด
**ไฟล์:** `ATK-Y.ino`, `ATK-B.ino`, `DEFY.ino`, `DEFB.ino`

370° ถูกคำนวณเป็น 10° ทำให้หุ่นวิ่งผิดทิศตอนชนขอบระหว่างยิง ควรเป็น 270° ให้ตรงกับ outer loop

```cpp
// ก่อนแก้
holonomic(60, 370, 0);

// หลังแก้
holonomic(60, 270, 0);
```

---

## รอบที่ 2 — Hardware Stack Review (อ้างอิง STACK.md)

### [FIX-04] `sin30`/`cos30` Macro เรียก `sin/cos` ทุกครั้ง
**ไฟล์:** `WSTPA03.ino`
**Hardware:** STM32F103CBT6 — **ไม่มี FPU**

`#define sin30 sin(30.f * degToRad)` คือ macro ที่แทนข้อความ — ทุกครั้งที่ `holonomic()` ถูกเรียก จะคำนวณ `sin()` และ `cos()` ใหม่ทั้งหมดด้วย software

บน STM32F103 (Cortex-M3, ไม่มี FPU) การคำนวณ `sin/cos` แบบ software float ใช้เวลา ~100–200 clock cycles ต่อครั้ง และ `holonomic()` ถูกเรียกทุก loop — เป็น performance overhead ที่ไม่จำเป็น

เนื่องจาก `sin(30°) = 0.5` และ `cos(30°) = 0.866...` เป็นค่าคงที่ ควรประกาศเป็น `const float` แทน

```cpp
// ก่อนแก้
#define sin30 sin(30.f * degToRad)   // คำนวณใหม่ทุกครั้งที่เรียกใช้
#define cos30 cos(30.f * degToRad)

// หลังแก้
const float sin30 = 0.5f;            // คำนวณครั้งเดียวตอน compile time
const float cos30 = 0.8660254f;
```

**ผล:** ลด software float operations ใน `holonomic()` จาก 4 ครั้ง (`sin`×2 + `cos`×2) เป็น 0

---

### [FIX-05] `goalAngle()` หารด้วย 360 แทน 320
**ไฟล์:** `WSTPA03.ino`
**Hardware:** HuskyLens Version 1 — **ความละเอียดกล้อง 320×240 px**

`goalAngle()` แปลง pixel X ของประตูเป็นองศาสำหรับ holonomic drive โดย normalize ด้วยความกว้างกล้อง แต่ใช้ 360 แทนที่จะเป็น 320 ซึ่งเป็นความกว้างจริงของ HuskyLens v1

```cpp
// ก่อนแก้
float normX = (float)goalX / 360.0f;   // ผิด — กล้องกว้าง 320px ไม่ใช่ 360

// หลังแก้
float normX = (float)goalX / 320.0f;   // ถูก — ตรงกับ HuskyLens v1 resolution
```

**ผลกระทบเชิงตัวเลข:**
```
ประตูอยู่ขวาสุด (goalX = 320):
  /360 → 320/360 × 180 = 160°   ← ยิงเฉียงซ้าย 20°
  /320 → 320/320 × 180 = 180°   ← ถูกต้อง

ประตูอยู่กลาง (goalX = 160):
  /360 → 160/360 × 180 = 80°    ← เฉียง 10°
  /320 → 160/320 × 180 = 90°    ← ถูกต้อง (ตรงกลาง)
```

---

### [FIX-06] `getIMU()` return void จาก bool function
**ไฟล์:** `WSTPA03.ino`
**Hardware:** MPU6050 ผ่าน UART Serial1

เมื่อ byte แรกไม่ใช่ header `0xAA` ฟังก์ชันเรียก `return;` โดยไม่คืนค่า ใน C++ การ return void จาก `bool` function คือ undefined behavior — compiler อาจคืนค่าอะไรก็ได้

```cpp
// ก่อนแก้
bool getIMU() {
    ...
    if (rxCnt == 0 && rxBuf[0] != 0xAA) return;   // UB: return void from bool

// หลังแก้
bool getIMU() {
    ...
    if (rxCnt == 0 && rxBuf[0] != 0xAA) return false;
```

**ผลกระทบ:** ถ้า compiler คืนค่า `true` โดยบังเอิญ → loop `for (int i = 0; i < 8; i++) { if (getIMU()) break; }` จะ break ออกทันที → `pvYaw` ไม่อัปเดต → หุ่นรักษาทิศผิด

---

---

## รอบที่ 3 — Deep Logic Review

### [FIX-07] `ATK-B` และ `DEFB` ตรวจ goal ID ผิดทีม
**ไฟล์:** `ATK-B.ino`, `DEFB.ino`
**Hardware:** HuskyLens v1 — ID 2 = ประตูเหลือง, ID 3 = ประตูน้ำเงิน

ทั้ง 2 ไฟล์ใช้ `blockSize[2]` (ประตูเหลือง) เพื่อตัดสินใจว่ามองเห็นประตูหรือไม่ แต่ดึงพิกัดจาก `blockInfo[3]` (ประตูน้ำเงิน) — check กับ read ใช้คนละ ID

```cpp
// ก่อนแก้ (ATK-B / DEFB)
if (huskylens.blockSize[2]) {          // ← เช็ค ID 2 (ประตูเหลือง)
  goalY = huskylens.blockInfo[3][0].y; // ← อ่าน ID 3 (ประตูน้ำเงิน)
  goalX = huskylens.blockInfo[3][0].x;
}

// หลังแก้
if (huskylens.blockSize[3]) {          // ← เช็ค ID 3 (ประตูน้ำเงิน) ให้ตรงกัน
  goalY = huskylens.blockInfo[3][0].y;
  goalX = huskylens.blockInfo[3][0].x;
}
```

**ผลกระทบ:** ทีมน้ำเงินจะไม่ยิงประตูเลย ถ้าประตูน้ำเงินมองเห็นแต่ประตูเหลืองไม่เห็น — หุ่นจะ skip การยิงทั้งหมด

---

### [FIX-08] `GOAL.ino` มุม 200° ผิด — ควรเป็น 270°
**ไฟล์:** `GOAL.ino`

analog(2) ใน mode อื่นทุก mode ใช้ 270° แต่ใน GOAL.ino ใช้ 200° ซึ่งไม่ตรงกับทิศที่ sensor ติดตั้ง (ดู STACK.md: analog(2) = bump sensor ด้านข้าง → หนี 270°)

```cpp
// ก่อนแก้
holonomic(60, 200, 0);

// หลังแก้
holonomic(60, 270, 0);
```

---

### [FIX-09] `GOAL.ino` speed เกิน 100 — motor อาจทำงานผิดปกติ
**ไฟล์:** `GOAL.ino`

`rot_w` ถูก constrain ไว้ที่ ±100 แต่เมื่อนำมาคูณ 1.5 ก่อนส่งเป็น speed → ค่าสูงสุดที่ส่งได้คือ 150 ซึ่งเกิน range ของ motor บน POP32

```cpp
// ก่อนแก้
heading(abs(rot_w) * 1.5, dir, 0);    // speed สูงสุด 150

// หลังแก้
heading(constrain(abs(rot_w) * 1.5f, 0.0f, 100.0f), dir, 0);  // cap ที่ 100
```

---

### [FIX-10] `GOAL.ino` ไม่มี else — หยุดนิ่งเมื่อไม่เห็นบอล
**ไฟล์:** `GOAL.ino`

ทุก mode มี else clause หมุนตัวหาบอลเมื่อ `blockSize[1] == 0` แต่ GOAL ไม่มี — goalkeeper จะหยุดนิ่งกลางสนามเมื่อมองไม่เห็นบอล

```cpp
// ก่อนแก้
if (huskylens.updateBlocks() && huskylens.blockSize[1]) {
  ...
}
// ไม่มี else → หยุดนิ่ง

// หลังแก้
if (huskylens.updateBlocks() && huskylens.blockSize[1]) {
  ...
} else {
  holonomic(0, 0, idleSpd);  // หมุนหาบอล
}
```

---

### [FIX-11] `Auto_zero()` ใช้ `int` เก็บค่า `millis()`
**ไฟล์:** `WSTPA03.ino`

`millis()` คืนค่า `unsigned long` (32-bit) แต่เก็บใน `int` (32-bit signed) — เมื่อ millis เกิน ~25 วัน ค่าจะล้น และ `millis() - timer > 5000` จะให้ผลผิด แม้จะไม่เกิดในสนามแข่ง แต่เป็น type ที่ผิดต้องแก้

```cpp
// ก่อนแก้
int timer = millis();

// หลังแก้
unsigned long timer = millis();
```

---

### [FIX-12] `unsigned int k` ไม่ได้ initialize
**ไฟล์:** `WSTPA03.ino`

ถ้า `SW_OK()` เป็น true ตั้งแต่ต้น (ปุ่มค้าง) while loop จะไม่รัน → `k` ยังไม่มีค่า → `if (k == 0)` เป็น undefined behavior

```cpp
// ก่อนแก้
unsigned int k;

// หลังแก้
unsigned int k = 0;
```

---

## สรุปทั้งหมด

| ID | รอบ | ไฟล์ | ปัญหา | ความรุนแรง |
|----|-----|------|--------|------------|
| FIX-01 | 1 | ATK-Y/B, DEF-Y/B, PENALTY | `rot_d` dead code | medium |
| FIX-02 | 1 | WSTPA03 | menu index out-of-bounds | high |
| FIX-03 | 1 | ATK-Y/B, DEF-Y/B | มุม 370° ผิด | medium |
| FIX-04 | 2 | WSTPA03 | `sin/cos` macro บน MCU ไม่มี FPU | low (performance) |
| FIX-05 | 2 | WSTPA03 | `goalAngle()` หาร 360 แทน 320 | high |
| FIX-06 | 2 | WSTPA03 | `getIMU()` return void จาก bool | high |
| FIX-07 | 3 | ATK-B, DEFB | goal blockSize check ผิด ID (2 แทน 3) | critical |
| FIX-08 | 3 | GOAL | มุม 200° ผิด ควรเป็น 270° | medium |
| FIX-09 | 3 | GOAL | speed เกิน 100 เข้า motor | medium |
| FIX-10 | 3 | GOAL | ไม่หมุนหาบอลเมื่อมองไม่เห็น | medium |
| FIX-11 | 3 | WSTPA03 | `int timer` แทน `unsigned long` | low |
| FIX-12 | 3 | WSTPA03 | `k` ไม่ initialize → undefined behavior | low |
