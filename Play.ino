// ─────────────────────────────────────────────────────────────
//  Play.ino — Unified Attacker / Defender state machine
//
//  States:
//    SEARCH   → no ball: spin toward last known position
//    TRACK    → ball found: PID center X + approach Y
//    ALIGN    → ball close+centered: curve body to face goal
//    APPROACH → aligned: drive toward goal
//    SHOOT    → goal in range: fire + reload
//
//  Call:
//    playStateMachine(goalID, curveScale, approachSpd, omegaScale, shootOffset)
//
//  Presets (use in WSTPA03.ino):
//    ATK-Yellow  playStateMachine(2, 1.0f, 40.0f, 1.5f, 15.0f)
//    ATK-Blue    playStateMachine(3, 1.2f, 60.0f, 1.5f, 20.0f)
//    DEF-Yellow  playStateMachine(2, 1.2f, 60.0f, 1.2f, 13.0f)
//    DEF-Blue    playStateMachine(3, 1.2f, 40.0f, 1.2f, 13.0f)
// ─────────────────────────────────────────────────────────────

void playStateMachine(int   goalID,
                      float curveScale,
                      float approachSpd,
                      float omegaScale,
                      float shootOffset) {

  enum State { SEARCH, TRACK, ALIGN, APPROACH, SHOOT };
  State state = SEARCH;
  resetPID();

  while (1) {

    // ── Wall bounce (highest priority) ──────────────────────
    if (checkWall()) continue;

    // ── Sensor read ─────────────────────────────────────────
    bool hasBall = huskylens.updateBlocks() && huskylens.blockSize[1];
    if (hasBall) {
      ballPosX = huskylens.blockInfo[1][0].x;
      ballPosY = huskylens.blockInfo[1][0].y;
      goalX = 0;  goalY = 0;
      if (huskylens.blockSize[goalID]) {
        goalX = huskylens.blockInfo[goalID][0].x;
        goalY = huskylens.blockInfo[goalID][0].y;
      }
      for (int i = 0; i < 8; i++) if (getIMU()) break;
    }

    // ── State machine ────────────────────────────────────────
    switch (state) {

      // ── SEARCH: spin to find ball ─────────────────────────
      case SEARCH:
        if (hasBall) { resetPID(); state = TRACK; break; }
        holonomic(0, 0, (sp_rot - ballPosX >= 0 ? 1 : -1) * idleSpd);
        break;

      // ── TRACK: PID approach ball ──────────────────────────
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

        holonomic(fli_spd, 90, rot_w);

        if (abs(rot_error) < rotErrorGap && abs(fli_error) < flingErrorGap) {
          wheel(0, 0, 0);
          lastYaw = pvYaw;
          resetPID();
          state = ALIGN;
        }
        break;

      // ── ALIGN: curve body to aim toward goal ──────────────
      case ALIGN:
        if (!hasBall) { state = SEARCH; break; }
        {
          float dir   = (lastYaw < 0) ? 0.0f  : 180.0f;
          float omega = (lastYaw < 0) ? 25.0f : -25.0f;
          holonomic(35, dir, omega * curveScale);
        }
        if (abs(pvYaw) < alignErrorGap && abs(sp_rot - ballPosX) < rotErrorGap)
          state = APPROACH;
        break;

      // ── APPROACH: drive toward goal + shoot when close ────
      case APPROACH:
        if (!hasBall) { state = SEARCH; break; }
        // ball drifted too far during ALIGN — re-approach before shooting
        if (ballPosY < spFli - 30) { resetPID(); state = TRACK; break; }
        if (goalY < goalFli) {
          // goal not visible yet — push forward to find it
          getIMU();
          heading(90, 90, 0);
          delay(555);
          state = TRACK;
          break;
        }
        {
          float goalDeg = goalAngle(goalX, goalY, 1);
          float spGoal  = -constrain((goalDeg - 90.0f) * 0.9f, -50.0f, 50.0f);
          holonomic(approachSpd, goalDeg, spGoal * omegaScale);
          if (goalY >= goalFli + shootOffset) state = SHOOT;
        }
        break;

      // ── SHOOT: fire + reload + back to TRACK ─────────────
      case SHOOT:
        beep();
        shoot();
        wheel(0, 0, 0);
        reload();
        resetPID();
        state = TRACK;
        break;
    }
  }
}
