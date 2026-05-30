// ─────────────────────────────────────────────────────────────
//  UI.ino  —  OLED display helper functions
//  128×64 px  |  row = y/8  |  col = x pixel offset
//
//  Team visual language (monochrome OLED):
//    YELLOW team  →  === borders  +  "(Y)" label
//    BLUE   team  →  --- borders  +  "(B)" label
//    Neutral      →  === borders
// ─────────────────────────────────────────────────────────────

// Team info per menu index (k=0..6)
//   0=READZX  1=ATK-Y  2=ATK-B  3=DEF-Y  4=DEF-B  5=GK  6=P
static const char* _teamName[]   = { "",  "YELLOW", "BLUE", "YELLOW", "BLUE", "", "" };
static const char* _teamTag[]    = { "",  "(Y)",    "(B)",  "(Y)",    "(B)",  "", "" };
static const char* _teamBorder[] = {
  "====================",   // READZX
  "== YELLOW TEAM =====",   // ATK-Y
  "-- BLUE   TEAM -----",   // ATK-B
  "== YELLOW TEAM =====",   // DEF-Y
  "-- BLUE   TEAM -----",   // DEF-B
  "====================",   // GK
  "===================="    // PENALTY
};
static const int   _goalID[]     = { 0, 2, 3, 2, 3, 0, 0 };

// ─── Progress bar builder ─────────────────────────────────────
static void makeBar(char* out, int pct, int slots) {
  int filled = pct * slots / 100;
  filled = filled < 0 ? 0 : (filled > slots ? slots : filled);
  out[0] = '[';
  for (int i = 0; i < slots; i++) out[i + 1] = (i < filled) ? '=' : '-';
  out[slots + 1] = ']';
  out[slots + 2] = '\0';
}

// ─── SPLASH ──────────────────────────────────────────────────
void drawSplash() {
  oled.clearDisplay();
  oled.textSize(1);
  oled.text(0, 0, "====================");
  oled.textSize(2);
  oled.text(1, 0, "SOCCER BOT");
  oled.textSize(1);
  oled.text(3, 20, "by RobotKub");
  oled.text(5, 2, ">> HuskyLens v1");
  oled.text(6, 2, ">> MPU6050 | POP32");
  oled.text(7, 0, "====================");
  oled.show();
}

// ─── ROBOT READY ─────────────────────────────────────────────
void drawReadyUI() {
  oled.clearDisplay();
  oled.textSize(1);
  oled.text(0, 0, "====================");
  oled.text(1, 4, "ROBOT  READY");
  oled.text(2, 0, "");
  oled.text(3, 2, "[SW_B]  Start IMU");
  oled.text(4, 0, "");
  oled.text(5, 2, "[SW_A]  Test kick");
  oled.text(6, 0, "");
  oled.text(7, 0, "====================");
  oled.show();
}

// ─── MENU (with team color) ───────────────────────────────────
//
//  Layout (128x64):
//  Row 0  │  ^ prev item
//  Row 1  │  SELECTED ITEM      ← textSize(2), 16px tall
//  Row 4  │  Team: YELLOW/BLUE  ← team indicator
//  Row 5  │  v next item
//  Row 6  │  ===YELLOW=== or ---BLUE---  ← team border
//  Row 7  │  [SW_OK] confirm
//
void drawMenuUI(unsigned int k, const char* items[], unsigned int count) {
  char buf[22];
  oled.clearDisplay();

  // ── prev item ──
  oled.textSize(1);
  if (k > 0) {
    snprintf(buf, sizeof(buf), " ^ %s %s", items[k - 1], _teamTag[k - 1]);
    oled.text(0, 0, buf);
  } else {
    oled.text(0, 28, "^ TOP ^");
  }

  // ── selected item (large) ──
  oled.textSize(2);
  oled.text(1, 0, items[k]);

  // ── team indicator ──
  oled.textSize(1);
  if (_teamName[k][0] != '\0') {
    snprintf(buf, sizeof(buf), "  Team : %s", _teamName[k]);
    oled.text(4, 0, buf);
  } else {
    oled.text(4, 0, "");
  }

  // ── next item ──
  if (k < count - 1) {
    snprintf(buf, sizeof(buf), " v %s %s", items[k + 1], _teamTag[k + 1]);
    oled.text(5, 0, buf);
  } else {
    oled.text(5, 28, "v END v");
  }

  // ── team border ──
  oled.text(6, 0, _teamBorder[k]);
  oled.text(7, 4, "[SW_OK]  confirm");
  oled.show();
}

// ─── IMU CALIBRATION ─────────────────────────────────────────
void drawAutoZeroUI(float yaw, bool retrying) {
  char bar[13];
  int pct = (int)((1.0f - abs(yaw) / 90.0f) * 100.0f);
  pct = pct < 0 ? 0 : (pct > 100 ? 100 : pct);
  makeBar(bar, pct, 10);

  oled.clearDisplay();
  oled.textSize(1);
  oled.text(0, 0, "= IMU  CALIBRATE  =");
  oled.textSize(2);
  oled.text(1, 0, "%.2f", yaw);
  oled.textSize(1);
  oled.text(3, 2, "deg   target: 0.00");
  oled.text(4, 0, bar);
  if (retrying)
    oled.text(6, 0, "!! Retrying IMU !!");
  else
    oled.text(6, 0, "  Calibrating...");
  oled.text(7, 0, "--------------------");
  oled.show();
}

// ─── IMU DONE ────────────────────────────────────────────────
void drawAutoZeroDone() {
  oled.clearDisplay();
  oled.textSize(1);
  oled.text(0, 0, "= IMU  CALIBRATE  =");
  oled.textSize(2);
  oled.text(2, 28, "DONE!");
  oled.textSize(1);
  oled.text(5, 4, "Yaw: 0.00 deg");
  oled.text(7, 0, "====================");
  oled.show();
  delay(800);
}

// ─── SENSOR DEBUG (READZX) ───────────────────────────────────
void drawSensorDebugUI(int a1, int a2, int a3) {
  char b1[13], b2[13], b3[13];
  char ln1[22], ln2[22], ln3[22];
  char lb1[22], lb2[22], lb3[22];

  makeBar(b1, a1 * 100 / 4095, 10);
  makeBar(b2, a2 * 100 / 4095, 10);
  makeBar(b3, a3 * 100 / 4095, 10);

  snprintf(ln1, sizeof(ln1), "A1:%4d T:%4d %s", a1, mid[0], a1 > mid[0] ? "!!" : "  ");
  snprintf(ln2, sizeof(ln2), "A2:%4d T:%4d %s", a2, mid[1], a2 > mid[1] ? "!!" : "  ");
  snprintf(ln3, sizeof(ln3), "A3:%4d T:%4d %s", a3, mid[2], a3 > mid[2] ? "!!" : "  ");
  snprintf(lb1, sizeof(lb1), "A1 %s", b1);
  snprintf(lb2, sizeof(lb2), "A2 %s", b2);
  snprintf(lb3, sizeof(lb3), "A3 %s", b3);

  oled.clearDisplay();
  oled.textSize(1);
  oled.text(0, 0, "=== SENSOR  READ ===");
  oled.text(1, 0, ln1);
  oled.text(2, 0, ln2);
  oled.text(3, 0, ln3);
  oled.text(4, 0, lb1);
  oled.text(5, 0, lb2);
  oled.text(6, 0, lb3);
  oled.text(7, 0, "--------------------");
  oled.show();
}

// ─── MODE START (with team color) ────────────────────────────
//
//  Layout:
//  Row 0  │  ===YELLOW=== or ---BLUE---
//  Row 1  │  ATK-YEL (Y)    ← large
//  Row 3  │  Role: ATK / DEF / GK / PK
//  Row 4  │  Team: YELLOW / BLUE
//  Row 5  │  Goal: ID 2 / ID 3
//  Row 6  │  Kicker: READY
//  Row 7  │  ===YELLOW=== or ---BLUE---
//
void drawModeStart(unsigned int k, const char* title) {
  char buf[22];
  const char* border = _teamBorder[k];

  // role label
  const char* role = "";
  if      (k == 1 || k == 2) role = "Role : ATTACKER";
  else if (k == 3 || k == 4) role = "Role : DEFENDER";
  else if (k == 5)            role = "Role : GOALKEEPER";
  else if (k == 6)            role = "Role : PENALTY";

  oled.clearDisplay();
  oled.textSize(1);
  oled.text(0, 0, border);

  // title + team tag (large)
  oled.textSize(2);
  if (_teamTag[k][0] != '\0') {
    snprintf(buf, sizeof(buf), "%s%s", title, _teamTag[k]);
    oled.text(1, 0, buf);
  } else {
    oled.text(1, 0, title);
  }

  oled.textSize(1);
  oled.text(3, 2, role);
  if (_teamName[k][0] != '\0') {
    snprintf(buf, sizeof(buf), " Team : %s", _teamName[k]);
    oled.text(4, 2, buf);
    snprintf(buf, sizeof(buf), " Goal : ID %d", _goalID[k]);
    oled.text(5, 2, buf);
  } else {
    oled.text(4, 0, "");
    oled.text(5, 0, "");
  }
  oled.text(6, 2, " Kicker: READY");
  oled.text(7, 0, border);
  oled.show();
  delay(1500);
}
