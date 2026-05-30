// void FootballYellow6() {
//   if (huskylens.updateBlocks() && huskylens.blockSize[1]) {
//       ballPosX = huskylens.blockInfo[1][0].x;
//       ballPosY = huskylens.blockInfo[1][0].y;
//       if (huskylens.blockSize[2]) {
//         goalY = huskylens.blockInfo[2][0].y;
//         goalX = huskylens.blockInfo[2][0].x;
//       }
//     if (bpcX = ballposX) {
//       holonomic(0, 0, 0);
//       delay(5000);
//     }else if (bpcY = ballPosY) {
//       holonomic(0, 0, 0);
//       delay(5000);
//     int bpcX = huskylens.blockInfo[1][0].x;
//     int bpcY = huskylens.blockInfo[1][0].y;
//   while (1) {
//     if (analog(2) > mid[1]) {
//       beep();
//       holonomic(60, 270, 0);
//       delay(500);
//     } else if (analog(1) > mid[0]) {
//       beep();
//       holonomic(60, 0, 0);
//       delay(500);
//     } else if (analog(3) > mid[2]) {
//       beep();
//       holonomic(60, 180, 0);
//       delay(500);
//     }
//     else{

//         for (int i = 0; i < 8; i++) {
//           if (getIMU()) break;
//         }

//         if (discoveState) {  //วิ่งปรับหุ่นยนต์ให้ใกล้ลูกบอลมากที่สุด discoveState=1
//           rot_error = sp_rot - ballPosX;
//           rot_d = rot_d + rot_error;
//           rot_d = constrain(rot_d, -100, 100);
//           rot_d = rot_error - rot_pError;
//           rot_pError = rot_error;
//           rot_w = (rot_error * rot_Kp) + (rot_i * rot_Ki) + (rot_d * rot_Kd);
//           rot_w = constrain(rot_w, -100, 100);

//           fli_error = spFli - ballPosY;
//           fli_i = fli_i + fli_error;
//           fli_i = constrain(fli_i, -100, 100);
//           fli_d = fli_error - fli_pError;
//           fli_pError = fli_error;
//           fli_spd = fli_error * fli_Kp + fli_i * fli_Ki + fli_d * fli_Kd;
//           fli_spd = constrain(fli_spd, -100, 100);



//           holonomic(fli_spd, 90, rot_w);
//           if ((abs(rot_error) < rotErrorGap) && (abs(fli_error) < flingErrorGap)) {
//             wheel(0, 0, 0);
//             lastYaw = pvYaw;  //บันทึกทิศล่า
//             discoveState = 0;
//           }

//         } else {  //จะทำงานเมื่อเจอลูกบอลแต่ทิศไม่ตรงที่จะยิ่ง
//           // หุ่นเลือกทิศทางที่ใกล้ที่สุด ที่จะปรับท้ายหุ่นหาลูกบอล
//           if (lastYaw < 0) {
//             vecCurve = 0;
//             radCurve = 15;
//           } else {
//             vecCurve = 180;
//             radCurve = -15;
//           }
          
//           holonomic(60, vecCurve, radCurve*2);
//           if (abs(pvYaw) < alignErrorGap) {      //เมื่อทิศอยู่ในค่าที่รับได้
//             rot_error = sp_rot - ballPosX;       //คำนวนหาค่า Error ว่าลูกบอลอยู่ตรงกลางหรือไม่
//             if (abs(rot_error) < rotErrorGap) {  //ถ้าลูกบอลอยู่ตรงกลางให้ทำการยิง
//               while (1) {

//                 getIMU();
//                 if (analog(2) > mid[1]) {
//                   beep();
//                   holonomic(60, 370, 0);
//                   delay(700);
//                 } else if (analog(1) > mid[0]) {
//                     beep();
//                     holonomic(60, 0, 0);
//                     delay(700);
//                 } else if (analog(3) > mid[2]) {
//                     beep();
//                     holonomic(60, 180, 0);
//                     delay(700);
//                 }
//                 lastYaw = pvYaw;
//                 discoveState = 0;
//                 break;
//                 }
//                 if (huskylens.updateBlocks() && huskylens.blockSize[1]) {
//                   ballPosX = huskylens.blockInfo[1][0].x;
//                   ballPosY = huskylens.blockInfo[1][0].y;

//                   if (huskylens.blockSize[2] > 0) {  // เช็คว่าเห็นประตู (ID 2) หรือไม่
//                     goalY = huskylens.blockInfo[2][0].y;
//                     goalX = huskylens.blockInfo[2][0].x;
//                   } else {
//                     goalY = 0;
//                     goalX = 0;
//                   }

//                   if (goalY >= goalFli) {
//                     float goalDeg = goalAngle(goalX, goalY, 1);
//                     float spGoal = -constrain((goalDeg - 90) *0.9, -50, 50);
//                     holonomic(60, goalDeg, spGoal*1.2);
//                     if (goalY >= goalFli+9) {
//                       beep();
//                       shoot();  //ยิง
//                       wheel(0,0,0);
//                       reload();  //เก็บก้านยิง
//                       discoveState = 1;
//                     }
//                   } else {
//                     heading(90, 90, 0);
//                     delay(1300);
//                   }

//                 } else {
//                   int sideRot = sp_rot - ballPosX;
//                   wheel(0, 0, 0);
//                 }
//               }
//               discoveState = 1;
//             }
//           }
//         } else {                            //หมุนตัวหาลูกบอล
//           int sideRot = sp_rot - ballPosX;  //คำนวนทิศการหมุนหาลูกบอลเมื่อเจอล่าสุด
//           holonomic(0, 0, sideRot / abs(sideRot) * idleSpd);
//           discoveState = 1;  //เตรียมพร้อมไปหาลูกบอลเมื่อเจอบอลอีกครั้ง
//       }
//     }
//   }
// }