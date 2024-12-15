#include <ch32v003fun.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../rv003usb/rv003usb/rv003usb.h"

uint32_t count;

volatile int last = 0;
void handle_debug_input(int numbytes, uint8_t * data) {
	last = data[0];
	count += numbytes;
}

int main() {
  SystemInit();
  
  SysTick->CNT = 0;
  Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
  
  usb_setup();

  count = 0;
  
  while(1)
  {
    printf("+%lu\n", count++);
    Delay_Ms(100);
    int i;
    for (i = 0; i < 10000; i++) {
      poll_input();
    }
    printf("-%lu[%c]\n", count++, last);
    Delay_Ms(100);
  }
}