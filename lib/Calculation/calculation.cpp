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

#include <calculation.h>

 //Calculates the number of pulses required to move a certain distance
long Calculation::getTravelPulses(long travDist, long pulsesPerMM)
{
  long travP = travDist * pulsesPerMM;
  return travP;
}
//Calculate the number of pulses required to rotate a certain angle
long Calculation::getRotationPulses(long rotAngle, float pulsesPerDeg)
{
  long rotP = rotAngle * pulsesPerDeg;
  return rotP;
}
//Calculate the interval required between pulses to achieve duration
float Calculation::getInterval(float travTime, long numPulses)
{
  float inter = travTime*1000000/numPulses;
  return inter;
}
//Calculate the interval required between pulses to achieve duration
float Calculation::getRotInterval(float travTime, long numPulses)
{
  float inter = travTime*1000/numPulses;
  return inter;
}