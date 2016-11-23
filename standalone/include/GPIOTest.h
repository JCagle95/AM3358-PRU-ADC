#include <iostream>
#include <fstream>
#include "GPIO.h"

class GPIOTest : public GPIO {
private:
    int pinID[32][2] =  {{30,60},{31,50},{48,51},{5,4},{3,2},{49,15},{117,14},{115,113},{111,112},
        {20,7},{38,39},{34,35},{66,67},{69,68},{45,44},{23,26},{47,46},{27,65},{22,63},{62,37},
        {36,33},{32,61},{86,88},{87,89},{10,11},{9,81},{8,80},{78,79},{76,77},{74,75},{72,73},{70,71}};

    int counter = 0;
    std::ofstream GPIO_fid;

public:
    GPIOTest(void);
    ~GPIOTest(void);
    void TogglePin(bool direction);
    void FlashLEDs(bool direction);
};
