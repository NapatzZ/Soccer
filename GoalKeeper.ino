// ─────────────────────────────────────────────────────────────
//  GoalKeeper.ino — Goalkeeper state machine
//
//  States:
//    SEARCH  → no ball: hold position, face forward (yaw = 0°)
//    TRACK   → ball visible: slide left/right to follow ball X
//
//  Robot always faces forward — no rotation during play.
// ─────────────────────────────────────────────────────────────

void goalkeeperStateMachine() {
  enum State { SEARCH, TRACK };
  State state = SEARCH;

  while (1) {

    // ── Wall bounce ──────────────────────────────────────────
    if (checkWall()) continue;

    bool hasBall = huskylens.updateBlocks() && huskylens.blockSize[1];
    if (hasBall) getIMU();

    switch (state) {

      // ── SEARCH: hold position, keep facing forward ────────
      case SEARCH:
        if (hasBall) { state = TRACK; break; }
        heading(0, 90, 0);   // speed=0, yaw maintained at 0°
        break;

      // ── TRACK: slide left/right tracking ball X ──────────
      case TRACK:
        if (!hasBall) { state = SEARCH; break; }
        {
          ballPosX  = huskylens.blockInfo[1][0].x;
          rot_error = sp_rot - ballPosX;
          rot_w     = constrain(rot_error * rot_Kp, -100, 100);

          float dir   = (rot_w > 0) ? 180.0f : 0.0f;
          float speed = constrain(abs(rot_w) * 1.5f, 0.0f, 100.0f);
          heading(speed, dir, 0);   // slide, keep yaw = 0°
        }
        break;
    }
  }
}
