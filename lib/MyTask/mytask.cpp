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

/// @brief Enables motors and travels configured distance
/// @return 
void Rot::execute()
{

}
/// @brief Enables motors and travels one chunk of 100
/// @return 
void Rot::executeChunk()
{

}
/// @brief Recalculates numbers after change of parameters
/// @return 
void Pan::recalcFigures()
{
    panSpeedInHz = getPanSpeedInHz();
    panTargetPos = getPanTargetPosition();  

    if (travelDir==0)
    {
       panTargetPos = panTargetPos;
    }
    else
    {
       panTargetPos = -panTargetPos;
    }
}
/// @brief Recalculates numbers after change of parameters
/// @return 
void Rot::recalcFigures()
{
    rotSpeedInHz = getRotSpeedInHz();
    rotTargetPos = getRotTargetPosition();
    
    if (rotationDir==0)
    {
       rotTargetPos = rotTargetPos;
    }
    else
    {
       rotTargetPos = -rotTargetPos;
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
}

/// @brief Enables motors and travels one chunk of 100
/// @return 
void Pan::executeChunk()
{

}
/// @brief executePanRotateChunk
/// @return 
void Job::executePanRotateChunk(double progress)
{
    panStepper->moveTo(pan.panTargetPos/100*progress);
    rotStepper->moveTo(rot.rotTargetPos/100*progress);
    jobProgress <- progress;    
}
/// @brief executePanChunk
/// @return 
void Job::executePanChunk(double progress)
{
    panStepper->moveTo(pan.panTargetPos/100*progress);
    jobProgress <- progress;
}
/// @brief executePanChunk
/// @return 
void Job::executeRotateChunk(double progress)
{
    rotStepper->moveTo(rot.rotTargetPos/100*progress);
    jobProgress <- progress;
}
/// @brief recalcFigures
/// @return 
void Job::recalcFigures()
{
    pan.recalcFigures();
    rot.recalcFigures();

    panStepper->setSpeedInHz(pan.panSpeedInHz);
    rotStepper->setSpeedInHz(rot.rotSpeedInHz);

    // TODO: improve acceleration value
    panStepper->setAcceleration(pan.panSpeedInHz/2);
    rotStepper->setAcceleration(rot.rotSpeedInHz/2);
    
    // TODO: makes stepper loose information where relative position is
    panStepper->setCurrentPosition(0);
    rotStepper->setCurrentPosition(0);

    jobProgress = 0;
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

/// @brief Constructor
/// @return 
Job::Job(void){
    rot = Rot();
    pan = Pan(); 
    pan.recalcFigures();
    rot.recalcFigures();
}

int Pan::getPanSpeedInHz()
{
    return  (int)round(travelDist * pulsesPerMM / travelTime);
}

int Rot::getRotSpeedInHz()
{ 
    return (int)round(rotationAngle * pulsesPerDeg / rotationTime);    
}

int Pan::getPanTargetPosition()
{
    return (int)round(travelDist * pulsesPerMM);
}

int Rot::getRotTargetPosition()
{ 
    return (int)round(rotationAngle * pulsesPerDeg);
}

// Takes 2 Window coords(points), turns them into vectors using the origin and calculates the angle around the xaxis between them.
// This function can be used for any hdc window. Ie 2 mouse points.
float Job::get_angle_2points(int p1x, int p1y, int p2x, int p2y)
{
    //Make point1 the origin, make point2 relative to the origin so we do point1 - point1, and point2-point1,
    //since we dont need point1 for the equation to work, the equation works correctly with the origin 0,0.
    int deltaY = p2y - p1y;
    int deltaX = p2x - p1x; //Vector 2 is now relative to origin, the angle is the same, we have just transformed it to use the origin.

    float angleInDegrees = atan2(deltaY, deltaX) * 180 / 3.141;

    //angleInDegrees *= -1; // Y axis is inverted in computer windows, Y goes down, so invert the angle.

    //Angle returned as:
    //                      90
    //            135                45
    //
    //       180          Origin           0
    //
    //           -135                -45
    //
    //                     -90


    // returned angle can now be used in the c++ window function used in text angle alignment. ie plf->lfEscapement = angle*10;
    return angleInDegrees;
}