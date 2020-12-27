// Test Program for ATTiny4

#include <avr/io.h>
#include <stdint.h>

volatile uint32_t counter;

void delay (uint16_t time) {
  counter = 46 * time;
  do counter--; while (counter != 0);
}

int main (void) {
  DDRB = (1 << DDB0 | 1 << DDB1 | 1 << DDB2);  // PB0, 1, 2 as outputs
  // TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Fast PWM 8-bit mode, A and B
  // TCCR0B = (1 << WGM02);    // Fast PWM 8-bit mode, no clock (timer stopped)
  // OCR0A = 1;
  // OCR0B = 1;
  // TCCR0B |= (3 << CS00);  // clock/64, timer started


  while (1) {

    // uint8_t i;
    // for (i = 0; i < 255; i+=4){
    //   OCR0A = i;
    //   delay(12);
    // }
    // OCR0A = 255;
    PORTB = 1;
    delay(100);
    // OCR0A = 0;
    PORTB = 0;
    delay(100);



  }
}
