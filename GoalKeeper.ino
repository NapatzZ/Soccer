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

  while (1) {

    // ── Wall bounce ──────────────────────────────────────────
    if (checkWall()) continue;

    bool hasBall = huskylens.updateBlocks() && huskylens.blockSize[1];

    switch (state) {

      // ── SEARCH: spin until ball found ─────────────────────
      case SEARCH:
        if (hasBall) { state = TRACK; break; }
        holonomic(0, 0, idleSpd);
        break;

      // ── TRACK: P-control on ball X axis ──────────────────
      case TRACK:
        if (!hasBall) { state = SEARCH; break; }
        {
          ballPosX = huskylens.blockInfo[1][0].x;
          getIMU();

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
