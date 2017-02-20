#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <chrono>

#include <pthread.h>
#include <time.h>

#include "SPI.h"

using namespace std;
typedef chrono::high_resolution_clock Time;     // Typedef chrono to simplify useage.
int DIVISION = 1;                               // Division is variable to scale program duration

int main(int argc, char *argv[]){

    SPI test;
    test.init();
    test.transfer();

    printf("Hello World!\n");
    return 0;
}
