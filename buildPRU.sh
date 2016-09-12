#bin/bash
pasm -b pru_clock.p
gcc -o pru_clock pru_clock.c -lprussdrv
