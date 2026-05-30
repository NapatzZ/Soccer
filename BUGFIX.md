# Bug Fix Report — Soccer Robot

## 1. Menu Index Out-of-Bounds (`WSTPA03.ino`)

**ปัญหา:**  
ตัวแปร `k` มาจาก `knob(0, 6)` ซึ่งคืนค่า 0–6 แต่โค้ดใช้ `menu[k-1]` และ `menu[k+1]` โดยไม่ตรวจสอบขอบเขต

- เมื่อ `k = 0` → `menu[-1]` → อ่านหน่วยความจำนอก array (undefined behavior)
- เมื่อ `k = 6` → `menu[7]` → array มีแค่ index 0–6, ออกนอกขอบเขต

```cpp
// ก่อนแก้
oled.text(0, 1, menu[k-1]);  // crash เมื่อ k=0
oled.text(5, 1, menu[k+1]);  // crash เมื่อ k=6

// หลังแก้
if (k > 0) oled.text(0, 1, menu[k-1]);
if (k < 6) oled.text(5, 1, menu[k+1]);
```

---

## 2. Dead Code ใน PID ของ `rot_d` (ATK-Y, ATK-B, DEFY, DEFB, PENALTY)

**ปัญหา:**  
มีการคำนวณ `rot_d` ผิดลำดับ — บรรทัดที่ 1 และ 2 สะสมค่า แต่บรรทัดที่ 3 เขียนทับทันที ทำให้บรรทัดแรกสองบรรทัดไม่มีผลใดๆ และค่า D-term ที่ได้ไม่ถูกต้อง

```cpp
// ก่อนแก้
rot_d = rot_d + rot_error;        // ← สะสมค่า...
rot_d = constrain(rot_d, -100, 100);
rot_d = rot_error - rot_pError;   // ← เขียนทับทันที สองบรรทัดข้างบนไม่มีผล

// หลังแก้
rot_d = rot_error - rot_pError;   // D-term ที่ถูกต้อง
```

**ไฟล์ที่แก้:** `ATK-Y.ino`, `ATK-B.ino`, `DEFY.ino`, `DEFB.ino`, `PENALTY.ino`

---

## 3. มุมผิดใน Wall-Bounce Code — `370°` (ATK-Y, ATK-B, DEFY, DEFB)

**ปัญหา:**  
ในโค้ดตรวจจับขอบ ภายใน `while(1)` ช่วงยิง sensor `analog(2)` ถูกกำหนดให้วิ่งที่ 370° แต่ `sin/cos` จะคำนวณเป็น ~10° ซึ่งผิดทิศกับที่ควรจะเป็น

ดูจากโค้ด loop ด้านนอกที่ `analog(2)` ใช้ `270°` อยู่แล้ว ภายใน loop ก็ควรใช้ทิศเดียวกัน

```cpp
// ก่อนแก้
holonomic(60, 370, 0);  // 370° → ถูกตีความเป็น ~10° (ผิดทิศ)

// หลังแก้
holonomic(60, 270, 0);  // ตรงกับ sensor analog(2) ที่ใช้ใน outer loop
```

**ไฟล์ที่แก้:** `ATK-Y.ino`, `ATK-B.ino`, `DEFY.ino`, `DEFB.ino`

---

## สรุป

| # | ไฟล์ | ปัญหา | ผลกระทบ |
|---|------|--------|---------|
| 1 | `WSTPA03.ino` | `menu[-1]` / `menu[7]` out-of-bounds | หน่วยความจำเสียหาย / crash |
| 2 | ATK-Y, ATK-B, DEFY, DEFB, PENALTY | `rot_d` dead code | D-term ของ PID ไม่ทำงานถูกต้อง |
| 3 | ATK-Y, ATK-B, DEFY, DEFB | `370°` ผิดทิศ | หุ่นวิ่งผิดทิศเมื่อชนขอบในช่วงยิง |
