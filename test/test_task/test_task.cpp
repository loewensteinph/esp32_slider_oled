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
#include <menuIO/u8g2Out.h>
#include <menuIO/chainStream.h>
#include <menuIO/rotaryEventIn.h>

MyTask task;
//long travelDist = 100;
//long pulsesPerMM = 40;
//float travTime = 1;
//uint8_t travStepPin = 22;
Pan pan;


U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);//allow contrast change

void setUp(void) {
    // set stuff up here
    

}

void tearDown(void) {
    // clean stuff up here
}

void test_pan_taskname(void) {
    pan.taskname = (char*)"test_task";
    char* taskname = pan.taskname;
    TEST_ASSERT_EQUAL("test_task", taskname);
}

void test_pan_interval(void) {
    pan.travelDist = 100;
    pan.travelTime = 1;
    TEST_ASSERT_EQUAL(40, pan.pulsesPerMM);
    pan.recalcFigures();
    TEST_ASSERT_EQUAL(4000, pan.travelPulses);
    TEST_ASSERT_EQUAL(250, pan.interval);
}

void test_pan_runtime(void) {
    pan.travelDist = 600;
    pan.travelTime = 2;
    pan.recalcFigures();
 
    TEST_ASSERT_EQUAL(4000, pan.travelPulses);
    TEST_ASSERT_EQUAL(250, pan.interval);
}

void test_pan_travel_pulses(void) {
    pan.travelDist = 100; 
    pan.recalcFigures();
    TEST_ASSERT_EQUAL(4000, pan.travelPulses);
}

void test_pan_execute(void) {
    pan.travelTime = 1;
    pan.travelDist = 100;
    uint32_t start = esp_timer_get_time();
    //u8g2.drawBox(12, 29, 107, 10); -> 100%
    volatile int progress = 0;

    while (progress < 100) {
        pan.executeChunk();
        progress += 1; // for demonstration only
    }
    //delay(1000);
    uint32_t end = esp_timer_get_time();
    uint32_t runtime = (end - start)/1000;

    //TEST_ASSERT_EQUAL(25000,pan.interval);

    TEST_ASSERT_UINT32_WITHIN(10,1000,runtime);
}

void RUN_UNITY_TESTS() {
    UNITY_BEGIN();
    RUN_TEST(test_pan_taskname);
    RUN_TEST(test_pan_interval);     
    RUN_TEST(test_pan_travel_pulses); 
    RUN_TEST(test_pan_execute); 
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