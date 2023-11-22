
// #include "controller.hpp"


// void performTestRun(){

//     unsigned long startedAt = millis();
//     double startWeight = weightHistory.averageSince((int64_t)millis() - 500);
//     unsigned long offTimestamp = 0;

//     while(true){

//         updateScale();
//         Serial.printf("%i;%3.2f\n",weightHistory.getHeadTimestamp()-startedAt,weightHistory.getHeadValue()-startWeight);
//         if (offTimestamp>0 && millis()>offTimestamp){
//             return;
//         }
//         if (digitalRead(GRINDER_SENSE_PIN) == 0 && offTimestamp==0){
//             offTimestamp=millis() + 5000;
//             continue;
//         }



//     }


// }
