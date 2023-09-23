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

//Calculate the interval required between pulses to achieve duration
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
    chunkTravelPulses = getChunkTravelPulses(travelPulses);
}

void Pan::executeChunk()
{
    recalcFigures();
    long workpulses = chunkTravelPulses;
    for (long i = 1; i <= workpulses; i++)
    {
        digitalWrite(travStepPin, HIGH);
        delayMicroseconds(interval / 2);
        digitalWrite(travStepPin, LOW);
        delayMicroseconds(interval / 2);
    }
}