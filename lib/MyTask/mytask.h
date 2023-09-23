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
#include <chrono>

#ifndef MYTASK_H
#define MYTASK_H
// Base class
class MyTask {

public:
    char* taskname;  // Attribute
    long pulsesPerMM = 40;
};


// Derived class
class Pan : public MyTask {
public:
    uint8_t travStepPin;
    //long travelPulses;
    float travelTime = 1;
    float initialtravTime = travelTime; // 5 Sec initial Time
    int minTravelDist = 25;
    int maxTravelDist = 1270;
    int travelDist = maxTravelDist;
    int travelDistInc = 50;
    int travelDistIncFine = 5;
    int travelDir = 0; // 0 forward  1 backward

    long travelPulses = getTravelPulses(travelDist, pulsesPerMM);
    float interval = getInterval(travelTime, travelPulses);
    long chunkTravelPulses = getChunkTravelPulses(travelPulses);

    void recalcFigures();
    long getTravelPulses(long travDist, long pulsesPerMM);
    float getInterval(float travelTime, long travelPulses);
    long getChunkTravelPulses(long travelPulses);

    //long travelPulses() { return travelPulses; }
    void executeChunk();
    Pan() {
        travelDist = maxTravelDist;
        pulsesPerMM = pulsesPerMM;
        travelTime = travelTime;
        travStepPin = travStepPin;
        taskname = (char*)"Pan";
    };
    Pan(long travDist, float travelTime) :
        travelDist{ travelDist }, travelTime{ travelTime }, travelPulses{ travelPulses }
    { // Constructor with parameters
        //travelPulses = getTravelPulses(travelDist, pulsesPerMM);
        //interval = getInterval(travelTime, travelPulses);
        taskname = (char*)"Pan";
    }
};
#endif