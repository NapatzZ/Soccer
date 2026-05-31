// ─────────────────────────────────────────────────────────────
//  PenaltyKick.ino — Penalty kick state machine
//
//  States:
//    SEARCH  → no ball: spin to find
//    TRACK   → PID center X + approach Y
//    ALIGN   → curve body to aim straight (yaw → 0°)
//    CHARGE  → full speed straight + shoot
//    DONE    → hold until SW_A for next kick
//
//  Vision buffer: ball must be missing 3+ frames before hasBall
//  flips false, preventing single-frame dropout from resetting state.
// ─────────────────────────────────────────────────────────────

enum PenState  { PN_SEARCH, PN_TRACK, PN_ALIGN, PN_CHARGE,   PN_DONE  };

// ── State handlers ────────────────────────────────────────────

PenState penSearch(bool hasBall) {
  if (hasBall) { resetPID(); return PN_TRACK; }
  searchSpin();
  return PN_SEARCH;
}

PenState penTrack(bool hasBall) {
  if (!hasBall) return PN_SEARCH;
  if (trackToBall()) return PN_ALIGN;
  return PN_TRACK;
}

PenState penAlign(bool hasBall) {
  if (!hasBall) return PN_SEARCH;
  float dir   = (pvYaw < 0) ? 0.0f  : 180.0f;
  float omega = (pvYaw < 0) ? 15.0f : -15.0f;
  holonomic(55, dir, omega);
  if (abs(pvYaw) < alignErrorGap && abs(sp_rot - ballPosX) < rotErrorGap)
    return PN_CHARGE;
  return PN_ALIGN;
}

PenState penCharge() {
  holonomic(90, 90, 0);
  delay(300);
  beep();
  shoot();
  reload();
  wheel(0, 0, 0);
  return PN_DONE;
}

PenState penDone(int &ballMissFrames, int &ballConfirmFrames) {
  wheel(0, 0, 0);
  waitSW_A_bmp();
  resetPID();
  ballMissFrames    = 0;
  ballConfirmFrames = 0;
  return PN_SEARCH;
}

// ── Orchestrator ──────────────────────────────────────────────

void penaltyStateMachine() {
  PenState state = PN_SEARCH;
  resetPID();
  int ballMissFrames    = 0;
  int ballConfirmFrames = 0;

  while (1) {

    // ── Wall bounce (highest priority) ───────────────────────
    if (checkWall()) continue;

    // ── Sensor read + vision filters ─────────────────────────
    huskylens.updateBlocks();
    bool rawBall = validateBall(ballMissFrames == 0);
    if (rawBall) {
      ballMissFrames = 0;
      if (ballConfirmFrames < ballConfirmMin) ballConfirmFrames++;
      for (int i = 0; i < 8; i++) if (getIMU()) break;
    } else {
      ballMissFrames++;
      if (ballMissFrames > ballMissMax) ballConfirmFrames = 0;
    }
    bool hasBall = (ballConfirmFrames >= ballConfirmMin) &&
                   (ballMissFrames    <= ballMissMax);

    // ── Dispatch ─────────────────────────────────────────────
    switch (state) {
      case PN_SEARCH: state = penSearch(hasBall);          break;
      case PN_TRACK:  state = penTrack(hasBall);           break;
      case PN_ALIGN:  state = penAlign(hasBall);           break;
      case PN_CHARGE: state = penCharge();                 break;
      case PN_DONE:   state = penDone(ballMissFrames, ballConfirmFrames);     break;
    }
  }
}
