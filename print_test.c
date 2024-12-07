#include <ch32v003fun.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../rv003usb/rv003usb/rv003usb.h"
#include "swio.h"

uint8_t scratch[80];
// volatile uint8_t print_buf[8] = {0x8A, 0x64, 0x6D, 0x6C, 0x6F, 0x63, 0x6B};
volatile uint8_t print_buf[8] = {0x8A, 'd', 'm', 'l', 'o', 'c', 'k'};
volatile uint32_t something = 1;
volatile uint8_t transmission_done;
volatile uint32_t systick_cnt;
uint32_t printf_cnt;
uint32_t next_active;

volatile uint8_t* print_pos = 0xE0000000;
uint32_t usb_cnt;

uint32_t count;
bool dm_unlocked = false;

volatile int last = 0;
void handle_debug_input( int numbytes, uint8_t * data )
{
	last = data[0];
	count += numbytes;
}

void attempt_unlock(uint8_t t1coeff) {
  if (*DMDATA0 == *DMDATA1 && *DMDATA0) {
    AFIO->PCFR1 &= ~(AFIO_PCFR1_SWJ_CFG);
    funPinMode(PD1, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP);
    MCFWriteReg32(DMSHDWCFGR, 0x5aa50000 | (1<<10), t1coeff); // Shadow Config Reg
    MCFWriteReg32(DMCFGR, 0x5aa50000 | (1<<10), t1coeff); // CFGR (1<<10 == Allow output from slave)
    MCFWriteReg32(DMCFGR, 0x5aa50000 | (1<<10), t1coeff); // Bug in silicon?  If coming out of cold boot, and we don't do our little "song and dance" this has to be called.
    // MCFWriteReg32(DMABSTRACTAUTO, 0x00000000, t1coeff);
    // MCFWriteReg32(DMCONTROL, 0x80000001 | (1<<10), t1coeff); // Bug in silicon?  If coming out of cold boot, and we don't do our little "song and dance" this has to be called.
    // MCFWriteReg32(DMCONTROL, 0x80000001 | (1<<10), t1coeff); // Bug in silicon?  If coming out of cold boot, and we don't do our little "song and dance" this has to be called.
    count = 0;
  }
}

void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
  // Make sure we only deal with control messages.  Like get/set feature reports.
  usb_cnt = SysTick->CNT + Ticks_from_Ms(3000);
  
  if( endp )
  {
    usb_send_empty( sendtok );
  }
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
  // if (e->custom != 0) something = e->custom;
  if (e->custom == 0xAB) {
    last = data[1];
    count += 8;
  } else {
    int offset = e->count<<3;
    int torx = e->max_len - offset;
    if( torx > len ) torx = len;
    if( torx > 0 )
    {
      memcpy( scratch + offset, data, torx );
      e->count++;
      if( ( e->count << 3 ) >= e->max_len )
      {
        transmission_done = e->max_len;
      }
    }
  }
}

void usb_handle_hid_get_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
  // You can check the lValueLSBIndexMSB word to decide what you want to do here
  // But, whatever you point this at will be returned back to the host PC where
  // it calls hid_get_feature_report. 
  //
  // Please note, that on some systems, for this to work, your return length must
  // match the length defined in HID_REPORT_COUNT, in your HID report, in usb_config.h
  if ((lValueLSBIndexMSB & 0xFF) == 0xAB) {
    if ((*DMDATA0 != *DMDATA1) && *DMDATA0) dm_unlocked = true;
    if (dm_unlocked) {
      memcpy(print_buf, (uint8_t*)DMDATA0, 4);
      memcpy(print_buf+4, (uint8_t*)DMDATA1, 4);
      *DMDATA0 = 0x0;
      *DMDATA1 = 0x0;
    }
    e->opaque = print_buf;
    e->max_len = 8;
  } else {
    if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
    e->opaque = scratch;
    e->max_len = reqLen;
  }
  
}

void usb_handle_hid_set_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
  // Here is where you get an alert when the host PC calls hid_send_feature_report.
  //
  // You can handle the appropriate message here.  Please note that in this
  // example, the data is chunked into groups-of-8-bytes.
  //
  // Note that you may need to make this match HID_REPORT_COUNT, in your HID
  // report, in usb_config.h
  if ((lValueLSBIndexMSB & 0xFF) == 0xAB) {
    reqLen = 8;  
  } else {
    if( reqLen > sizeof( scratch ) ) reqLen = sizeof( scratch );
  }

  e->custom = (lValueLSBIndexMSB & 0xFF);
  e->max_len = reqLen;
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
  LogUEvent( SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength );
}

void reboot_to_bootloader() {
  __disable_irq();
  // Unlock the ability to write to the MODE flag of STATR register.
  FLASH->BOOT_MODEKEYR = 0x45670123; // KEY1
  FLASH->BOOT_MODEKEYR = 0xCDEF89AB; // KEY2
  // Set to run BOOT flash at next reset (MODE = 1).
  FLASH->STATR = 0x4000;
  // Clear all reset status flags (RMVF = 1).
  RCC->RSTSCKR |= 0x1000000;
  // Perform software reset (KEYCODE = 0xBEEF, RESETSYS = 1).
  PFIC->CFGR = 0xBEEF0080;
}

int main() {
  SystemInit();
  
  SysTick->CNT = 0;
  Delay_Ms(1); // Ensures USB re-enumeration after bootloader or reset; Spec demand >2.5µs ( TDDIS )
  
  usb_setup();

  count = 0;
  
  while(1)
  {
    if(transmission_done) {
      switch (scratch[1]) {
        case 0xA1:
          // reboot into bootloader
          reboot_to_bootloader();
          break;
        
        case 0xA5:
          // printf("dmdmdm: %u\n", something);
          attempt_unlock(scratch[2]);
          break;

        case 0xA6:
          printf("0xA6");
          break;

        case 0xA7:
          printf("0xA7");
          //  Write user strings to flash
          break;
      }

      transmission_done = 0;
    }
  // if ((int32_t)(SysTick->CNT - printf_cnt) > 0) {
  //   printf_cnt = SysTick->CNT + Ticks_from_Ms(1000);
  //   printf("+%lu\n", count++);
  // }
  printf( "+%lu\n", count++ );
	Delay_Ms(100);
	int i;
	for( i = 0; i < 10000; i++ ) {
    poll_input();
  }
	printf( "-%lu[%c]\n", count++, last );
	Delay_Ms(100);
  }
}