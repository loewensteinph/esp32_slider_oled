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
#include "FastAccelStepper.h"

#ifndef MYTASK_H
#define MYTASK_H

// Base class
class MyTask {
public:
    char* taskname;  // Attribute
    long pulsesPerMM = 40;
    double pulsesPerDeg = 27.8;
    uint8_t enablePin = 19;

    void enableMotors();
    void disableMotors();
};

// Derived class
class Rot : public MyTask {
    public:
        uint8_t rotStepPin = 18;
        uint8_t rotDirPin = 5;

        bool tooFast = false;
        float minInterval = 150;
        float rotationTime = 3;
        int rotationDir = 0; // 0 forward  1 backward
        double rotationAngle = 90;

        int rotTargetPos;        
        int rotSpeedInHz;

        Rot(void);

        int getRotTargetPosition();   
        int getRotSpeedInHz();   
        void recalcFigures();

        void execute();
        void executeChunk();
};

// Derived class
class Pan : public MyTask {
public:
    uint8_t travStepPin = 16;
    uint8_t travDirPin = 17;
          
    bool tooFast = false;
    float minInterval = 125;
    float travelTime = 3;
    float initialtravTime = travelTime; // 5 Sec initial Time
    int minTravelDist = 25;
    int maxTravelDist = 600;
    int travelDist = maxTravelDist;
    int travelDistInc = 50;
    int travelDistIncFine = 5;
    int travelDir = 0; // 0 forward  1 backward

    int panTargetPos;
    int panSpeedInHz;

    int getPanTargetPosition();
    int getPanSpeedInHz();
    void recalcFigures();

    void executeChunk();
    void execute();
    Pan(void);
    Pan(long travDist, float travelTime) :
        travelDist{ travelDist }, travelTime{ travelTime }
    { 
        taskname = (char*)"Pan";
    }
};

// Base class
class Job {
public:

    Rot rot; //To just have a pointer to another object.
    Pan pan; //To just have a pointer to another object.

    FastAccelStepperEngine engine;
    FastAccelStepper *panStepper = NULL;
    FastAccelStepper *rotStepper = NULL;

    enum Jobs { doPan, doRotate, doPanRotate, doTrack };
    Jobs jobtype = doPan;    

    int jobProgress;
    void executePanChunk(double progress);
    void executeRotateChunk(double progress);    
    void executePanRotateChunk(double progress);      
    void recalcFigures();
    char* jobToStr(Job::Jobs job);

    Job(void);

    float get_angle_2points(int p1x, int p1y, int p2x, int p2y);
};

#endif