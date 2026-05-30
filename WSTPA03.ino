// ── goalAngle ─────────────────────────────────────────────────
// Maps HuskyLens goal pixel X (0–320) to holonomic angle (0–180°)
float goalAngle(int gX, int gY, bool isFront) {
  float angle = (float)gX / 320.0f * 180.0f;
  if (!isFront) angle = -(170.0f - angle);
  return angle;
}

// ── IMU ───────────────────────────────────────────────────────
void zeroYaw() {
  Serial1.begin(115200);
  delay(50);
  Serial1.write(0XA5); delay(50); Serial1.write(0X54); delay(50);
  Serial1.write(0XA5); delay(50); Serial1.write(0X55); delay(50);
  Serial1.write(0XA5); delay(50); Serial1.write(0X52); delay(50);
}

bool getIMU() {
  while (Serial1.available()) {
    rxBuf[rxCnt] = Serial1.read();
    if (rxCnt == 0 && rxBuf[0] != 0xAA) return false;
    rxCnt++;
    if (rxCnt == 8) {
      rxCnt = 0;
      if (rxBuf[0] == 0xAA && rxBuf[7] == 0x55) {
        int16_t angle = (int16_t)(rxBuf[1] << 8 | rxBuf[2]);
        angle = angle % 36000;
        if (angle >  17900) angle -= 36000;
        if (angle < -17900) angle += 36000;
        pvYaw = angle / 100.f;
        return true;
      }
    }
  }
  return false;
}

void Auto_zero() {
  bool retrying = false;
  zeroYaw();
  getIMU();
  unsigned long timer = millis();
  pvYaw = 90.0f;
  while (abs(pvYaw) > 0.3) {
    if (getIMU()) {
      drawAutoZeroUI(pvYaw, retrying);
      if (millis() - timer > 5000) {
        zeroYaw();
        timer = millis();
        retrying = true;
      }
    }
  }
  drawAutoZeroDone();
  oled.clear();
  oled.show();
}

// ── PID / wall helpers ────────────────────────────────────────
void resetPID() {
  rot_error = rot_pError = rot_i = rot_d = rot_w = 0.0f;
  fli_error = fli_pError = fli_i = fli_d = fli_spd = 0.0f;
}

bool checkWall() {
  if (analog(2) > mid[1]) { beep(); holonomic(60, 270, 0); delay(500); return true; }
  if (analog(1) > mid[0]) { beep(); holonomic(60,   0, 0); delay(500); return true; }
  if (analog(3) > mid[2]) { beep(); holonomic(60, 180, 0); delay(500); return true; }
  return false;
}

// ── Drive ─────────────────────────────────────────────────────
void wheel(int s1, int s2, int s3) {
  motor(1, s1);
  motor(2, s2);
  motor(3, s3);
}

void holonomic(float spd, float theta, float omega) {
  float rad = theta * degToRad;
  float vx  = spd * cos(rad);
  float vy  = spd * sin(rad);
  wheel( vy * cos30 - vx * sin30 + omega,
        -vy * cos30 - vx * sin30 + omega,
         vx + omega);
}

void heading(float spd, float theta, float spYaw) {
  head_error  = spYaw - pvYaw;
  head_i      = constrain(head_i + head_error, -180, 180);
  head_d      = head_error - head_pError;
  head_w      = constrain((head_error * head_Kp) + (head_i * head_Ki) + (head_d * head_Kd), -100, 100);
  head_pError = head_error;
  holonomic(spd, theta, head_w);
}

// ── Kicker ────────────────────────────────────────────────────
void shoot() {
  motor(4, reloadSpd);
  delay(150);
  motor(4, 0);
  delay(50);
}

int reloadTimer = 0;
void reload() {
  motor(4, reloadSpd);
  reloadTimer = 0;
  for (int i = 0; i < 2000; i++) {
    reloadTimer++;
    if (analogRead(limPin) > 1000) break;
    delay(1);
  }
  if (reloadTimer == 2000) {
    motor(4, -reloadSpd);
    delay(500);
    motor(4, reloadSpd);
    reloadTimer = 0;
    for (int i = 0; i < 2000; i++) {
      reloadTimer++;
      if (analogRead(limPin) > 1000) break;
      delay(1);
    }
  }
  motor(4, 0);
}

// ── Setup / Loop ──────────────────────────────────────────────
void setup() {
  SW_A();
  drawSplash();
  delay(2300);
  reload();
  Wire.setClock(400000);
  while (!huskylens.begin(Wire)) {
    oled.clearDisplay();
    oled.textSize(1);
    oled.text(0, 0, "====================");
    oled.text(3, 10, "!! ERROR !!");
    oled.text(5, 2, "HuskyLens failed");
    oled.text(6, 2, "Check I2C cable");
    oled.text(7, 0, "====================");
    oled.show();
    delay(100);
  }
  delay(3000);
  pvYaw = 90.0f;
  drawReadyUI();
  while (!SW_B()) {
    if (SW_A()) { shoot(); reload(); }
  }
  oled.clear();
  Auto_zero();
  getIMU();

  unsigned int k = 0;
  const char* menu[] = {
    "READZX", "ATK-Yellow", "ATK-blue",
    "DEF-yellow", "DEF-blue", "GK", "P",
  };
  while (!SW_OK()) {
    k = knob(0, 6);
    drawMenuUI(k, menu, 7);
  }

  if (k == 0) {
    while (1) drawSensorDebugUI(analog(1), analog(2), analog(3));
  } else if (k == 1) {
    drawModeStart(1, "ATK-YEL");
    playStateMachine(2, 1.0f, 40.0f, 1.5f, 15.0f);
  } else if (k == 2) {
    drawModeStart(2, "ATK-BLU");
    playStateMachine(3, 1.2f, 60.0f, 1.5f, 20.0f);
  } else if (k == 3) {
    drawModeStart(3, "DEF-YEL");
    delay(3200);
    playStateMachine(2, 1.2f, 60.0f, 1.2f, 13.0f);
  } else if (k == 4) {
    drawModeStart(4, "DEF-BLU");
    delay(3200);
    playStateMachine(3, 1.2f, 40.0f, 1.2f, 13.0f);
  } else if (k == 5) {
    drawModeStart(5, "GOALKEEP");
    goalkeeperStateMachine();
  } else if (k == 6) {
    drawModeStart(6, "PENALTY");
    penaltyStateMachine();
  }
}

void loop() {
  sound(2000, 1000);
}
