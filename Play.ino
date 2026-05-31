// ─────────────────────────────────────────────────────────────
//  Play.ino — Attacker / Defender state machine
//
//  States:
//    SEARCH   -> no ball: spin toward last known position
//    TRACK    -> ball found: PID center X + approach Y
//    ALIGN    -> ball close+centered: curve body to face goal
//    APPROACH -> aligned: drive toward goal
//    SHOOT    -> goal in range: fire + reload
//
//  Presets (called from Hardware.ino setup):
//    ATK-Yellow  playStateMachine(2, 1.4f, 40.0f, 1.5f, 15.0f)
//    ATK-Blue    playStateMachine(3, 1.2f, 60.0f, 1.5f, 20.0f)
//    DEF-Yellow  playStateMachine(2, 1.2f, 60.0f, 1.2f, 13.0f)
//    DEF-Blue    playStateMachine(3, 1.2f, 40.0f, 1.2f, 13.0f)
// ─────────────────────────────────────────────────────────────

enum PlayState { PS_SEARCH, PS_TRACK, PS_ALIGN, PS_APPROACH, PS_SHOOT };

// ── State handlers ────────────────────────────────────────────

PlayState playSearch(bool hasBall) {
  if (hasBall) { resetPID(); return PS_TRACK; }
  searchSpin();
  return PS_SEARCH;
}

PlayState playTrack(bool hasBall) {
  if (!hasBall) return PS_SEARCH;
  if (trackToBall()) return PS_ALIGN;
  return PS_TRACK;
}

PlayState playAlign(bool hasBall, float orbitRadius) {
  if (!hasBall) return PS_SEARCH;
  float dir   = (pvYaw < 0) ? 0.0f   : 180.0f;
  float omega = (pvYaw < 0) ? 25.0f  : -25.0f;
  holonomic(orbitRadius * 25.0f, dir, omega);
  if (abs(pvYaw) < alignErrorGap && abs(sp_rot - ballPosX) < rotErrorGap)
    return PS_APPROACH;
  return PS_ALIGN;
}

PlayState playApproach(bool hasBall, float driveSpeed,
                       float steerScale, float shootDist) {
  if (!hasBall) return PS_SEARCH;
  if (ballPosY < spFli - 30) { resetPID(); return PS_TRACK; }
  if (goalY < goalFli) {
    getIMU();
    heading(90, 90, 0);
    delay(555);
    return PS_TRACK;
  }
  float goalDeg = goalAngle(goalX, goalY, 1);
  float spGoal  = -constrain((goalDeg - 90.0f) * 0.9f, -50.0f, 50.0f);
  holonomic(driveSpeed, goalDeg, spGoal * steerScale);
  if (goalY >= goalFli + shootDist) return PS_SHOOT;
  return PS_APPROACH;
}

PlayState playShoot() {
  holonomic(50, 90, 0);
  delay(150);
  beep();
  shoot();
  wheel(0, 0, 0);
  reload();
  resetPID();
  return PS_TRACK;
}

// ── Orchestrator ──────────────────────────────────────────────

void playStateMachine(int   goalID,
                      float orbitRadius,
                      float driveSpeed,
                      float steerScale,
                      float shootDist) {

  PlayState state = PS_SEARCH;
  resetPID();
  int ballMissFrames    = 0;
  int ballConfirmFrames = 0;
  int goalMissFrames    = 0;

  while (1) {

    // ── Wall bounce (highest priority) ───────────────────────
    if (checkWall()) continue;

    // ── Sensor read + vision filters ─────────────────────────
    huskylens.updateBlocks();
    bool rawBall = validateBall(ballMissFrames == 0);
    if (rawBall) {
      ballMissFrames = 0;
      if (ballConfirmFrames < ballConfirmMin) ballConfirmFrames++;

      if (huskylens.blockSize[goalID]) {
        goalX = huskylens.blockInfo[goalID][0].x;
        goalY = huskylens.blockInfo[goalID][0].y;
        goalMissFrames = 0;
      } else {
        goalMissFrames++;
        if (goalMissFrames > goalMissMax) { goalX = 0; goalY = 0; }
      }

      for (int i = 0; i < 8; i++) if (getIMU()) break;
    } else {
      ballMissFrames++;
      if (ballMissFrames > ballMissMax) ballConfirmFrames = 0;
    }
    bool hasBall = (ballConfirmFrames >= ballConfirmMin) &&
                   (ballMissFrames    <= ballMissMax);

    // ── Dispatch ─────────────────────────────────────────────
    switch (state) {
      case PS_SEARCH:   state = playSearch(hasBall);                                         break;
      case PS_TRACK:    state = playTrack(hasBall);                                          break;
      case PS_ALIGN:    state = playAlign(hasBall, orbitRadius);                              break;
      case PS_APPROACH: state = playApproach(hasBall, driveSpeed, steerScale, shootDist); break;
      case PS_SHOOT:    state = playShoot();                                                 break;
    }
  }
}
