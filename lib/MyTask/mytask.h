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
    float pulsesPerDeg = 27.8;
    uint8_t enablePin = 19;

    void enableMotors();
    void disableMotors();    
};

// Derived class
class Rot : public MyTask {
    public:
        uint8_t rotStepPin = 17;
        uint8_t rotDirPin = 16;

        bool tooFast = false;
        float minInterval = 150;
        float rotationTime = 3;
        int rotationDir = 0; // 0 forward  1 backward
        int rotationAngle = 90;

        long rotationPulses = getRotationPulses(rotationAngle, pulsesPerMM);
        float rotationInterval = getRotationInterval(rotationTime, rotationPulses);
        long chunkRotationPulses = getChunkRotationPulses(rotationPulses);
        
        Rot(void);
        void recalcFigures();
        long getRotationPulses(long rotateAngle, long pulsesPerDeg);
        float getRotationInterval(float rotationTime, long rotationPulses);
        long getChunkRotationPulses(long rotationPulses);

        void execute();
        void executeChunk();
};

// Derived class
class Pan : public MyTask {
public:
    uint8_t travStepPin = 18;
    uint8_t travDirPin = 5;
          
    //long travelPulses;
    bool tooFast = false;
    float minInterval = 125;
    float travelTime = 3;
    float initialtravTime = travelTime; // 5 Sec initial Time
    int minTravelDist = 25;
    int maxTravelDist = 50;
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
    void execute();
    Pan(void);
    Pan(long travDist, float travelTime) :
        travelDist{ travelDist }, travelTime{ travelTime }, travelPulses{ travelPulses }
    { 
        taskname = (char*)"Pan";
    }
};

// Base class
class Job {
public:
    Rot rot; //To just have a pointer to another object.
    Pan pan; //To just have a pointer to another object.    
    enum Jobs { doPan, doRotate, doPanRotate, doTrack };
    Jobs jobtype = doPan;    
    double pulseAdjustFactor = getPulseAdjustFactor(pan.chunkTravelPulses,rot.chunkRotationPulses);
    double getPulseAdjustFactor(long chunkTravelPulses,long chunkRotationPulses);
    void executePanChunk();
    void executeRotateChunk();    
    void executePanRotateChunk();    
    void recalcFigures();
    char* jobToStr(Job::Jobs job);
};

#endif