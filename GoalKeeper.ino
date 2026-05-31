// ─────────────────────────────────────────────────────────────
//  GoalKeeper.ino — Goalkeeper state machine
//
//  States:
//    SEARCH  → no ball: hold position, face forward (yaw = 0°)
//    TRACK   → ball visible: slide left/right to follow ball X
//
//  Robot always faces forward — no rotation during play.
//  Vision buffer: ball must be missing 3+ frames before hasBall
//  flips false.
// ─────────────────────────────────────────────────────────────

enum GKState   { GK_SEARCH, GK_TRACK };

// ── State handlers ────────────────────────────────────────────

GKState gkSearch(bool hasBall) {
  if (hasBall) return GK_TRACK;
  heading(0, 90, 0);
  return GK_SEARCH;
}

GKState gkTrack(bool hasBall) {
  if (!hasBall) return GK_SEARCH;
  rot_error   = sp_rot - ballPosX;
  rot_w       = constrain(rot_error * rot_Kp, -100, 100);
  float dir   = (rot_w > 0) ? 180.0f : 0.0f;
  float speed = preventDeadzone(constrain(abs(rot_w) * 1.5f, 0.0f, 100.0f));
  heading(speed, dir, 0);
  return GK_TRACK;
}

// ── Orchestrator ──────────────────────────────────────────────

void goalkeeperStateMachine() {
  GKState state = GK_SEARCH;
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
      getIMU();
    } else {
      ballMissFrames++;
      if (ballMissFrames > ballMissMax) ballConfirmFrames = 0;
    }
    bool hasBall = (ballConfirmFrames >= ballConfirmMin) &&
                   (ballMissFrames    <= ballMissMax);

    // ── Dispatch ─────────────────────────────────────────────
    switch (state) {
      case GK_SEARCH: state = gkSearch(hasBall); break;
      case GK_TRACK:  state = gkTrack(hasBall);  break;
    }
  }
}
