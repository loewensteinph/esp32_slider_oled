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

//Calculates the number of pulses required to move a certain distance
long Pan::getTravelPulses(long travDist, long pulsesPerMM)
{
    long travelPulses = travelDist * pulsesPerMM;
    return travelPulses;
}


/// @brief Calculate the interval required between pulses to achieve duration
/// @param travelTime Configured travel time to travel distance
/// @param travelPulses 
/// @return 
float Pan::getInterval(float travelTime, long travelPulses)
{
    float inter = travelTime * 1000000 / travelPulses;
    return inter;
}

long Pan::getChunkTravelPulses(long travelPulses)
{
    long chunkTravelPulses = travelPulses / 100;
    return chunkTravelPulses;
}
// runs 100 steps from outside loop 1 to 100
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

Pan::Pan(void){
    pinMode(enablePin, INPUT); // disable motors 
    pinMode(travDirPin, OUTPUT);   
    pinMode(travStepPin, OUTPUT);
    recalcFigures();
}

void Pan::execute()
{
    recalcFigures();
    pinMode(enablePin, OUTPUT); // enable motors 
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

void Pan::executeChunk()
{
    //recalcFigures();
    //pinMode(enablePin, OUTPUT); // enable motors 
    //Serial.println(interval);    
    for (long i = 1; i <= chunkTravelPulses; i++)
    {
        digitalWrite(travStepPin, HIGH);
        delayMicroseconds(interval);
        digitalWrite(travStepPin, LOW);
    } 
    //pinMode(enablePin, INPUT); // disable motors     
}

void Pan::enableMotors()
{
    pinMode(enablePin, OUTPUT); // enable motors 
}
void Pan::disableMotors()
{
   pinMode(enablePin, INPUT); // disable motors   
}
