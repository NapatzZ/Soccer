// ─────────────────────────────────────────────────────────────
//  GoalKeeper.ino — Goalkeeper state machine
//
//  States:
//    SEARCH  → no ball: spin to scan
//    TRACK   → ball visible: slide left/right to track ball X
// ─────────────────────────────────────────────────────────────

void goalkeeperStateMachine() {
  enum State { SEARCH, TRACK };
  State state = SEARCH;
  float scanDir = 1.0f;

  while (1) {

    // ── Wall bounce ──────────────────────────────────────────
    if (checkWall()) continue;

    bool hasBall = huskylens.updateBlocks() && huskylens.blockSize[1];
    if (hasBall) getIMU();

    switch (state) {

      // ── SEARCH: sweep 0–180° (bounce at ±85°) ────────────
      case SEARCH:
        if (hasBall) { state = TRACK; break; }
        if (pvYaw >=  85.0f) scanDir = -1.0f;
        if (pvYaw <= -85.0f) scanDir =  1.0f;
        holonomic(0, 0, scanDir * idleSpd);
        break;

      // ── TRACK: P-control on ball X axis ──────────────────
      case TRACK:
        if (!hasBall) { state = SEARCH; break; }
        {
          ballPosX = huskylens.blockInfo[1][0].x;

          rot_error = sp_rot - ballPosX;
          rot_w     = constrain(rot_error * rot_Kp, -100, 100);

          float dir   = (rot_w > 0) ? 180.0f : 0.0f;
          float speed = constrain(abs(rot_w) * 1.5f, 0.0f, 100.0f);
          heading(speed, dir, 0);
        }
        break;
    }
  }
}
