# Bug Fix Log — Soccer Robot

---

## Round 1 — General Review

### [FIX-01] `rot_d` Dead Code — PID D-term broken
**Files:** `ATK-Y`, `ATK-B`, `DEF-Y`, `DEF-B`, `PENALTY`

Lines 1–2 accumulated into `rot_d` but line 3 immediately overwrote it, so D-term was always zero.

```cpp
// before
rot_d = rot_d + rot_error;
rot_d = constrain(rot_d, -100, 100);
rot_d = rot_error - rot_pError;   // overwrites above — dead code

// after
rot_d = rot_error - rot_pError;
```

---

### [FIX-02] Menu Index Out-of-Bounds
**File:** `Hardware.ino` (was `WSTPA03.ino`)

`menu[k-1]` when `k=0` -> `menu[-1]`, and `menu[k+1]` when `k=6` -> `menu[7]`.

```cpp
// before
oled.text(0, 1, menu[k-1]);
oled.text(5, 1, menu[k+1]);

// after
if (k > 0) oled.text(0, 1, menu[k-1]);
if (k < 6) oled.text(5, 1, menu[k+1]);
```

---

### [FIX-03] `holonomic(60, 370, 0)` wrong angle
**Files:** `ATK-Y`, `ATK-B`, `DEF-Y`, `DEF-B`

370° is interpreted as ~10°, wrong direction. `analog(2)` sensor escapes at 270° everywhere else.

```cpp
holonomic(60, 270, 0);   // fixed
```

---

## Round 2 — Hardware Stack Review

### [FIX-04] `sin/cos` macro recalculated every call (no FPU)
**File:** `Hardware.ino`

`#define sin30 sin(30.f * degToRad)` recomputes on every `holonomic()` call. STM32F103 has no FPU — software float `sin/cos` costs ~100–200 cycles each. Values are constants so were changed to `const float`.

```cpp
const float sin30 = 0.5f;
const float cos30 = 0.8660254f;
```

---

### [FIX-05] `goalAngle()` divided by 360 instead of 320
**File:** `Hardware.ino`

HuskyLens v1 is 320px wide, not 360. Caused a systematic angle error on every shot.

```
goalX = 320 (far right):  /360 -> 160°,  /320 -> 180° ✓
goalX = 160 (center):     /360 ->  80°,  /320 ->  90° ✓
```

---

### [FIX-06] `getIMU()` returned void from bool function
**File:** `Hardware.ino`

`return;` inside a `bool` function is undefined behavior — compiler may return `true`, causing the IMU loop to break early without updating `pvYaw`.

```cpp
if (rxCnt == 0 && rxBuf[0] != 0xAA) return false;   // fixed
```

---

## Round 3 — Deep Logic Review

### [FIX-07] Blue team checked wrong goal ID
**Files:** `ATK-B`, `DEF-B`

`blockSize[2]` (yellow goal) was used to guard reading from `blockInfo[3]` (blue goal). Blue team would never shoot if only the blue goal was visible.

```cpp
// after
if (huskylens.blockSize[3]) {
  goalY = huskylens.blockInfo[3][0].y;
  goalX = huskylens.blockInfo[3][0].x;
}
```

---

### [FIX-08] GK wrong escape angle
**File:** `GoalKeeper.ino`

`analog(2)` triggered `holonomic(60, 200, 0)` — should be 270° to match sensor mounting direction.

---

### [FIX-09] GK slide speed exceeded 100
**File:** `GoalKeeper.ino`

`abs(rot_w) * 1.5f` can reach 150, beyond POP32 motor range. Added `constrain(..., 0, 100)`.

---

### [FIX-10] GK didn't spin when ball not visible
**File:** `GoalKeeper.ino`

Missing `else` clause — goalkeeper froze when ball left frame. Fixed by adding idle spin.

---

### [FIX-11] `millis()` stored in `int` instead of `unsigned long`
**File:** `Hardware.ino`

`millis()` returns `unsigned long`. Storing in `int` causes overflow after ~25 days, breaking the 5s timeout check in `Auto_zero()`.

---

### [FIX-12] `k` uninitialized before menu loop
**File:** `Hardware.ino`

If SW_OK was already held on boot, the `while (!SW_OK())` loop never ran, leaving `k` undefined.

```cpp
unsigned int k = 0;   // fixed
```

---

## Summary

| ID | Severity | File | Issue |
|----|----------|------|-------|
| FIX-01 | medium | ATK/DEF/PENALTY | `rot_d` dead code, D-term broken |
| FIX-02 | high | Hardware | menu array out-of-bounds |
| FIX-03 | medium | ATK/DEF | angle 370° -> wrong direction |
| FIX-04 | low | Hardware | `sin/cos` macro on no-FPU MCU |
| FIX-05 | high | Hardware | `goalAngle()` divides by 360 not 320 |
| FIX-06 | high | Hardware | `getIMU()` void return UB |
| FIX-07 | critical | ATK-B / DEF-B | blue team checked wrong goal ID |
| FIX-08 | medium | GoalKeeper | wrong escape angle 200° -> 270° |
| FIX-09 | medium | GoalKeeper | slide speed uncapped, exceeds 100 |
| FIX-10 | medium | GoalKeeper | no idle spin when ball lost |
| FIX-11 | low | Hardware | `int timer` should be `unsigned long` |
| FIX-12 | low | Hardware | `k` uninitialized |
