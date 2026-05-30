// ─────────────────────────────────────────────────────────────
//  PenaltyKick.ino — Penalty kick state machine
//
//  States:
//    SEARCH  → no ball: spin to find
//    TRACK   → PID center X + approach Y (same as attacker)
//    ALIGN   → curve body to aim straight
//    CHARGE  → full speed straight + shoot
//    DONE    → wait for SW_A before next kick
// ─────────────────────────────────────────────────────────────

void penaltyStateMachine() {
  enum State { SEARCH, TRACK, ALIGN, CHARGE, DONE };
  State state = SEARCH;
  resetPID();

  while (1) {

    // ── Wall bounce ──────────────────────────────────────────
    if (checkWall()) continue;

    bool hasBall = huskylens.updateBlocks() && huskylens.blockSize[1];
    if (hasBall) {
      ballPosX = huskylens.blockInfo[1][0].x;
      ballPosY = huskylens.blockInfo[1][0].y;
      while (Serial1.available()) getIMU();
    }

    switch (state) {

      // ── SEARCH ────────────────────────────────────────────
      case SEARCH:
        if (hasBall) { resetPID(); state = TRACK; break; }
        holonomic(0, 0, (sp_rot - ballPosX >= 0 ? 1 : -1) * idleSpd);
        break;

      // ── TRACK ─────────────────────────────────────────────
      case TRACK:
        if (!hasBall) { state = SEARCH; break; }

        rot_error  = sp_rot - ballPosX;
        rot_d      = rot_error - rot_pError;
        rot_pError = rot_error;
        rot_w      = constrain(rot_error * rot_Kp + rot_d * rot_Kd, -100, 100);

        fli_error  = spFli - ballPosY;
        fli_i      = constrain(fli_i + fli_error, -100, 100);
        fli_d      = fli_error - fli_pError;
        fli_pError = fli_error;
        fli_spd    = constrain(fli_error * fli_Kp
                             + fli_i     * fli_Ki
                             + fli_d     * fli_Kd, -100, 100);
        if      (fli_spd >  0 && fli_spd <  minApproachSpd) fli_spd =  minApproachSpd;
        else if (fli_spd < -0 && fli_spd > -minApproachSpd) fli_spd = -minApproachSpd;

        holonomic(fli_spd, 90, rot_w);

        if (abs(rot_error) < rotErrorGap && abs(fli_error) < flingErrorGap) {
          wheel(0, 0, 0);
          lastYaw = pvYaw;
          resetPID();
          state = ALIGN;
        }
        break;

      // ── ALIGN: rotate to straight-ahead ──────────────────
      case ALIGN:
        if (!hasBall) { state = SEARCH; break; }
        {
          float dir   = (lastYaw < 0) ? 0.0f  : 180.0f;
          float omega = (lastYaw < 0) ? 15.0f : -15.0f;
          holonomic(55, dir, omega);
        }
        if (abs(pvYaw) < alignErrorGap && abs(sp_rot - ballPosX) < rotErrorGap)
          state = CHARGE;
        break;

      // ── CHARGE: full speed + shoot ────────────────────────
      case CHARGE:
        holonomic(90, 90, 0);
        delay(300);
        beep();
        shoot();
        reload();
        wheel(0, 0, 0);
        state = DONE;
        break;

      // ── DONE: hold until SW_A pressed for next kick ───────
      case DONE:
        wheel(0, 0, 0);
        waitSW_A_bmp();
        resetPID();
        state = SEARCH;
        break;
    }
  }
}
