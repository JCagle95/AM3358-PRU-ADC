#ifndef ADS8329_H
#define ADS8329_H

#include <chrono>
#include "prussdrv.h"
#include <pruss_intc_mapping.h>

#include "Digital.h"

#define AM33XX
#define PRU_NUM0	0
#define PRU_NUM1	1
#define ARRAY_SIZE	4000
#define MAX_CYCLE	50

typedef struct {
	uint32_t RunFlag;
	uint32_t Delay;
	uint32_t MaxSize;
	uint32_t DataReady;
	uint16_t Data[ARRAY_SIZE];
} pru_data;

class ADS8329 {
private:
	PWM Timer;
    pru_data *PRU1_Memory;
    uint16_t *Shared_Memory;
	uint16_t dataPoint[ARRAY_SIZE*MAX_CYCLE];

public:
    bool init(void);
	bool setSamplingRate(int rate);
	bool begin(std::chrono::high_resolution_clock::time_point start, int duration);
};

#endif
