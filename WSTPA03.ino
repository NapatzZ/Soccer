#include <POP32.h>
#include <POP32_Huskylens.h>
POP32_Huskylens huskylens;
#define degToRad 0.0174f
const float sin30 = 0.5f;
const float cos30 = 0.8660254f;
int low[3] = { 400, 390, 590 };
int high[3] = { 3800, 2700, 3800 };
int mid[3] = { (low[0] + high[0]) / 2, (low[1] + high[1]) / 2, (low[2] + high[2]) / 2 };
// ค่าที่ใช้ปรับหุ่นให้เข้าด้านหน้าตรงลูกบอล
#define rot_Kp 0.6
#define rot_Ki 0.0
#define rot_Kd 0.6
#define sp_rot 155      // ค่า setpoint ที่ลูกบอลอยู่ตรงกลางกล้องแกน x (ปรับเอียงซ้ายนิดหน่อยจาก 160)
#define rotErrorGap 15  // ค่า Error ที่ยอมให้หุ่นหยุดทำงาน
#define idleSpd 65      // ค่าความเร็วการหมุนเมื่อไม่เจอลูกบอล
float rot_error, rot_pError, rot_i, rot_d, rot_w;
int ballPosX;
int goalX;
int goalY;
float goalAngle(int goalX, int goalY, bool isFront) {
  // Normalize x: 0.0 (ซ้าย) → 1.0 (ขวา)
  float normX = (float)goalX / 320.0f;

  // แปลงเป็นองศา: ขวา=0, กลาง=90, ซ้าย=180
  float angle = normX * 180.0f;  // 0–180°

  if (!isFront) {
    // ถ้าอยู่ข้างหลัง: กลับด้าน ซ้าย=180, กลาง=-90, ขวา=-180
    angle = -(170.0f - angle);
  }

  return angle;  // ช่วง: -180 ถึง 180°
}

// ค่าที่ใช้ปรับหุ่นให้เข้าใกล้ลูกบอล
#define fli_Kp 0.9
#define fli_Ki 0.0
#define fli_Kd 0.0
#define flingErrorGap 12  // ค่า Error ที่ยอมให้หุ่นหยุดทำงาน
float spFli = 220;        // ค่า setpoint ที่ยอมให้ลูกบอลอยู่ใกล้หุ่นมากที่สุด อาจเริ่มที่จุดกลางจอ แกน Y
float goalFli = 23;
float fli_error, fli_pError, fli_i, fli_d, fli_spd;
int ballPosY;
// ค่าที่ใช้ปรับหุ่นให้ตรงทิศอ้างอิง
#define alignErrorGap 4
float vecCurve, radCurve;
int discoveState = 1;
// ค่าที่ใช้รักษาทิศหุ่นยนต์
#define head_Kp 1.5f
#define head_Ki 0.0f
#define head_Kd 0.0f
float head_error, head_pError, head_w, head_d, head_i;
/* >> ball shooting <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
#define limPin A0
#define reloadSpd -100

float thetaRad, vx, vy, spd1, spd2, spd3;
void zeroYaw() {
  Serial1.begin(115200);
  delay(100);
  // Sets data rate to 115200 bps
  Serial1.write(0XA5);
  delay(100);
  Serial1.write(0X54);
  delay(100);
  // pitch correction roll angle
  Serial1.write(0XA5);
  delay(100);
  Serial1.write(0X55);
  delay(100);
  // zero degree heading
  Serial1.write(0XA5);
  delay(100);
  Serial1.write(0X52);
  delay(100);
  // automatic mode
}
float pvYaw, lastYaw;
uint8_t rxCnt = 0, rxBuf[8];
bool getIMU() {
  while (Serial1.available()) {
    rxBuf[rxCnt] = Serial1.read();
    if (rxCnt == 0 && rxBuf[0] != 0xAA) return false;
    rxCnt++;
    if (rxCnt == 8) {  // package is complete
      rxCnt = 0;
      if (rxBuf[0] == 0xAA && rxBuf[7] == 0x55) {  // data package is correct
        int16_t angle = (int16_t)(rxBuf[1] << 8 | rxBuf[2]);
        angle = angle % 36000;
        if (angle > 17900) angle -= 36000;
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
  while (abs(pvYaw) > 0.05) {
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

void wheel(int s1, int s2, int s3) {
  motor(1, s1);
  motor(2, s2);
  motor(3, s3);
}
void holonomic(float spd, float theta, float omega) {
  thetaRad = theta * degToRad;
  vx = spd * cos(thetaRad);
  vy = spd * sin(thetaRad);
  spd1 = vy * cos30 - vx * sin30 + omega;
  spd2 = -vy * cos30 - vx * sin30 + omega;
  spd3 = vx + omega;
  wheel(spd1, spd2, spd3);
}
void heading(float spd, float theta, float spYaw) {
  head_error = spYaw - pvYaw;
  head_i = head_i + head_error;
  head_i = constrain(head_i, -180, 180);
  head_d = head_error - head_pError;
  head_w = (head_error * head_Kp) + (head_i * head_Ki) + (head_d * head_Kd);
  head_w = constrain(head_w, -100, 100);
  holonomic(spd, theta, head_w);
  head_pError = head_error;
}
void shoot() {
  motor(4, reloadSpd);
  delay(150);
  motor(4, 0);
  delay(50);
}
int timer = 0;
void reload() {
  motor(4, reloadSpd);
  timer = 0;
  for (int i = 0; i < 2000; i++) {
    timer++;
    if (analogRead(limPin) > 1000) break;
    delay(1);
  }
  if (timer == 2000) {     // ถ้าก้านยิงติด
    motor(4, -reloadSpd);  // เลื่อนก้านยิงไปข้างหน้า
    delay(500);            //ก่อน 0.5 วินาที
    motor(4, reloadSpd);
    timer = 0;
    for (int i = 0; i < 2000; i++) {
      timer++;
      if ((analogRead(limPin) > 1000)) break;
      delay(1);
    }
  }
  motor(4, 0);
}
void setup() {
  SW_A();
  drawSplash();
  delay(2300);
  reload();
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
    if (SW_A()) {
      shoot();
      reload();
    }
  }
  oled.clear();
  Auto_zero();
  getIMU();
  unsigned int k = 0;
  const char* menu[] = {
    "READZX",
    "ATK-Yellow",
    "ATK-blue",
    "DEF-yellow",
    "DEF-blue",
    "GK",
    "P",
  };
  while (!SW_OK()) {
    k = knob(0, 6);
    drawMenuUI(k, menu, 7);
  }
  if (k == 0) {
    while (1) {
      drawSensorDebugUI(analog(1), analog(2), analog(3));
    }
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