/*
 Copyright (c) 2014-present PlatformIO <contact@platformio.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
**/
#include <Arduino.h>
#include <mytask.h>
#include <unity.h>

MyTask task;
//long travelDist = 100;
//long pulsesPerMM = 40;
//float travTime = 1;
//uint8_t travStepPin = 22;
Rot rot;

void setUp(void) {
    // set stuff up here  

}

void tearDown(void) {
    // clean stuff up here
}

void test_rot_taskname(void) {
    rot.taskname = (char*)"test_task";
    char* taskname = rot.taskname;
    TEST_ASSERT_EQUAL("test_task", taskname);
}

void test_rot_interval(void) {
    rot.rotationAngle = 180;
    rot.rotationTime = 3;
    TEST_ASSERT_EQUAL(36.5, rot.pulsesPerDeg);
    TEST_ASSERT_EQUAL(3600, rot.rotationPulses);
    TEST_ASSERT_EQUAL(833, rot.rotationInterval);
}

void test_rot_runtime(void) {
    rot.rotationAngle = 90;
    rot.rotationTime = 1.5;
    rot.recalcFigures();
     TEST_ASSERT_EQUAL(3240, rot.rotationPulses);
    TEST_ASSERT_EQUAL(462, rot.rotationInterval);
}

void test_rot_rotation_pulses(void) {
    rot.pulsesPerDeg = 45; 
    rot.recalcFigures();
    TEST_ASSERT_EQUAL(4050, rot.rotationPulses);
}

void test_rot_execute(void) {
    rot.rotationTime = 1;
    rot.rotationAngle = 90;
    uint32_t start = esp_timer_get_time();
    //u8g2.drawBox(12, 29, 107, 10); -> 100%
    volatile int progress = 0;

    while (progress < 100) {
        rot.executeChunk();
        progress += 1; // for demonstration only
    }
    //delay(1000);
    uint32_t end = esp_timer_get_time();
    uint32_t runtime = (end - start)/1000;

    //TEST_ASSERT_EQUAL(25000,pan.interval);

    TEST_ASSERT_UINT32_WITHIN(10,1484,runtime);
}

void RUN_UNITY_TESTS() {
    UNITY_BEGIN();
    RUN_TEST(test_rot_taskname);
    RUN_TEST(test_rot_interval);     
    RUN_TEST(test_rot_runtime); 
    RUN_TEST(test_rot_rotation_pulses);     
    RUN_TEST(test_rot_execute); 
    UNITY_END();
}


void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    RUN_UNITY_TESTS();
}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(500);
}