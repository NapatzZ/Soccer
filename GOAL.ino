void GOAL(){
  while(1){
    if(analog(2)>mid[1]){
      beep();
     holonomic(60, 270, 0);
      delay(500);
    }
    else if(analog(1)>mid[0]){
      beep();
      holonomic(60, 0, 0);
      delay(200);
    }
    else if(analog(3)>mid[2]){
      beep();
      holonomic(60,180, 0);
      delay(200);
    }
    if (huskylens.updateBlocks() && huskylens.blockSize[1]) {
      ballPosX = huskylens.blockInfo[1][0].x;  // เลือกใช้แกน x  ball ID 1 บล็อกแรก
      ballPosY = huskylens.blockInfo[1][0].y;
      rot_error = (sp_rot - ballPosX);
      rot_w = rot_error * rot_Kp;
      rot_w = constrain(rot_w, -100, 100);
      float dir = (rot_w > 0 ? 180:0);
      getIMU();
    heading(constrain(abs(rot_w) * 1.5f, 0.0f, 100.0f), dir, 0);
  } else {
    holonomic(0, 0, idleSpd);
  }
}
}