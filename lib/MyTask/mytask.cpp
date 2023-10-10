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
#include <mytask.h>
#include <iostream>
#include <Arduino.h>

/// @brief Enables motors and travels one chunk of 100
/// @return 
void MyTask::enableMotors()
{
  digitalWrite(enablePin, LOW); // enable motors   
}
/// @brief Enables motors and travels one chunk of 100
/// @return 
void MyTask::disableMotors()
{
  digitalWrite(enablePin, HIGH); // disable motors   
}

/// @brief Constructor Rotation
/// @return 
Rot::Rot(void){
    pinMode(rotDirPin, OUTPUT);   
    pinMode(rotStepPin, OUTPUT);
    pinMode(enablePin, OUTPUT);
}

/// @brief Calculates the number of pulses required to rotate a certain angle
/// @param rotateAngle Configured rotate angle
/// @param pulsesPerDeg Configured required Pulses to rotate 1 degree
/// @return 
long Rot::getRotationPulses(long rotateAngle, long pulsesPerDeg)
{
    long rotatePulses = rotateAngle * pulsesPerDeg;
    return rotatePulses;
}

/// @brief Calculates the number of pulses required to rotate a certain angle
/// @param rotateAngle Configured rotate angle
/// @param pulsesPerDeg Configured required Pulses to rotate 1 degree
/// @return 
float Rot::getRotationInterval(float rotationTime,long rotatePulses)
{
    float rotationInterval = rotationTime * 1000000 / rotatePulses;
    return rotationInterval;
}
/// @brief Splits Pulses in 100 chunks 
/// @param rotatePulses Rquired Pulses to travel distance
/// @return 
long Rot::getChunkRotationPulses(long rotationPulses)
{
    long chunkRotatePulses = rotationPulses / 100;
    return chunkRotatePulses;
}
/// @brief Recalculates numbers after change of parameters
/// @return 
void Rot::recalcFigures()
{
    rotationPulses = getRotationPulses(rotationAngle, pulsesPerDeg);
    rotationInterval = getRotationInterval(rotationTime, rotationPulses); 
    tooFast = rotationInterval < minInterval;
    chunkRotationPulses = getChunkRotationPulses(rotationPulses);

    if (rotationDir==0)
    {
       digitalWrite(rotDirPin, HIGH);
    }
    else
    {
       digitalWrite(rotDirPin, LOW);  
    }
}
/// @brief Enables motors and travels configured distance
/// @return 
void Rot::execute()
{
    recalcFigures();
    delay(200);
    //Serial.println(interval);
    for (long i = 1; i <= rotationPulses; i++)
    {
        digitalWrite(rotStepPin, HIGH);
        delayMicroseconds(rotationInterval);
        digitalWrite(rotStepPin, LOW);
    }
    pinMode(enablePin, INPUT); // disable motors 
}
/// @brief Enables motors and travels one chunk of 100
/// @return 
void Rot::executeChunk()
{
    for (long i = 1; i <= chunkRotationPulses; i++)
    {
        digitalWrite(rotStepPin, HIGH);
        delayMicroseconds(rotationInterval);
        digitalWrite(rotStepPin, LOW);
    } 
}

/// @brief Calculates the number of pulses required to move a certain distance
/// @param travDist Configured travel distance
/// @param pulsesPerMM Configured required Pulses to travel 1mm 
/// @return 
long Pan::getTravelPulses(long travDist, long pulsesPerMM)
{
    long travelPulses = travelDist * pulsesPerMM;
    return travelPulses;
}

/// @brief Calculate the interval required between pulses to achieve duration
/// @param travelTime Configured travel time to travel distance
/// @param travelPulses Rquired Pulses to travel distance
/// @return 
float Pan::getInterval(float travelTime, long travelPulses)
{
    float inter = travelTime * 1000000 / travelPulses;
    return inter;
}

/// @brief Splits Pulses in 100 chunks 
/// @param travelPulses Rquired Pulses to travel distance
/// @return 
long Pan::getChunkTravelPulses(long travelPulses)
{
    long chunkTravelPulses = travelPulses / 100;
    return chunkTravelPulses;
}

/// @brief Recalculates numbers after change of parameters
/// @return 
void Pan::recalcFigures()
{
    travelPulses = getTravelPulses(travelDist, pulsesPerMM);
    interval = getInterval(travelTime, travelPulses); 
    tooFast = interval < minInterval;
    chunkTravelPulses = getChunkTravelPulses(travelPulses);

    if (travelDir==0)
    {
       digitalWrite(travDirPin, HIGH);
    }
    else
    {
       digitalWrite(travDirPin, LOW);  
    }
}
/// @brief Constructor
/// @return 
Pan::Pan(void){
    pinMode(travDirPin, OUTPUT);   
    pinMode(travStepPin, OUTPUT);
    pinMode(enablePin, OUTPUT);
    recalcFigures();
}
/// @brief Enables motors and travels configured distance
/// @return 
void Pan::execute()
{
    recalcFigures();
    delay(200);
    //Serial.println(interval);
    for (long i = 1; i <= travelPulses; i++)
    {
        digitalWrite(travStepPin, HIGH);
        delayMicroseconds(interval);
        digitalWrite(travStepPin, LOW);
    }
    pinMode(enablePin, INPUT); // disable motors 
}

/// @brief Enables motors and travels one chunk of 100
/// @return 
void Pan::executeChunk()
{
    for (long i = 1; i <= chunkTravelPulses; i++)
    {
        digitalWrite(travStepPin, HIGH);
        delayMicroseconds(interval);
        digitalWrite(travStepPin, LOW);
    } 
}
/// @brief executePanRotateChunk
/// @return 
void Job::executePanRotateChunk()
{
    double pulseAdjustFactor = getPulseAdjustFactor(pan.chunkTravelPulses,rot.chunkRotationPulses);
    long longPulses = max(pan.chunkTravelPulses, rot.chunkRotationPulses);
    double rotAdjust = 1;
    double panAdjust = 1;

    if (pan.chunkTravelPulses >= longPulses)
    {
        rotAdjust = pulseAdjustFactor;
    }
    if (rot.chunkRotationPulses >= longPulses)
    {
        panAdjust = pulseAdjustFactor;
    }

    for (long i = 1; i <= longPulses; i++)
    {
        digitalWrite(rot.rotStepPin, HIGH);
        delayMicroseconds(pan.interval*rotAdjust);
        digitalWrite(rot.rotStepPin, LOW); 
       
        digitalWrite(pan.travStepPin, HIGH);
        delayMicroseconds(pan.interval*panAdjust);
        digitalWrite(pan.travStepPin, LOW); 
    }
}
/// @brief executePanChunk
/// @return 
void Job::executePanChunk()
{
    pan.executeChunk();
}
/// @brief executePanChunk
/// @return 
void Job::executeRotateChunk()
{
    rot.executeChunk();
}
/// @brief getPulseAdjustFactor
/// @return 
double Job::getPulseAdjustFactor(long chunkTravelPulses,long chunkRotationPulses)
{
    long shortPulses;
    long longPulses;
    shortPulses = min(chunkTravelPulses, chunkRotationPulses);
    longPulses = max(chunkTravelPulses, chunkRotationPulses);
    double pulseAdjustFactor = (double)shortPulses/(double)longPulses;
    return pulseAdjustFactor;
}
/// @brief recalcFigures
/// @return 
void Job::recalcFigures()
{
    pan.recalcFigures();
    rot.recalcFigures();
}
/// @brief jobToStr
/// @return 
char* Job::jobToStr(Job::Jobs job)
{
    switch (job)
    {
        case Job::doPan : return (char*)"Pan";
        case Job::doRotate : return (char*)"Rot";
        case Job::doPanRotate : return (char*)"panRot";
        case Job::doTrack : return (char*)"Track";
        default : return (char*)"";
    }
}