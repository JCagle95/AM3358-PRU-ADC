#include <stdio.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

int main(void){
  prussdrv_init();
  if(prussdrv_open(PRU_EVTOUT_0) == -1){
    printf("prussdrv_open() failed\n");
    return 1;
  }

  tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
  prussdrv_pruintc_init(&pruss_intc_initdata);

  char clockProgram[] = "pru_clock.bin";
  printf("Program Begin\n");
  prussdrv_exec_program(1, clockProgram);

  prussdrv_pru_wait_event(PRU_EVTOUT_0);
  printf("Program Complete\n");
  prussdrv_pru_disable(1);
  prussdrv_exit();

  return 0;
}
