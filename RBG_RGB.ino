// Test Program for ATTiny4

#include <avr/io.h>
#include <stdint.h>

#define MAXBRITE  75

volatile uint32_t counter;

void delay (uint16_t time) {
  counter = 46 * time;
  do counter--; while (counter != 0);
}

int main (void) {
  DDRB = (1 << DDB0 | 1 << DDB1 | 1 << DDB2);  // PB0, 1, 2 as outputs
  OCR0A = 3;
  OCR0B = 255;
  TCCR0A = (1 << COM0A1 | 1 << COM0B1 | 1 << WGM00);  // Phase-correct PWM 8-bit mode, A and B
  TCCR0B = (1 << CS01);  // clk/8, timer started


  while (1) {

    uint8_t i;
    for (i = 3; i < MAXBRITE; i++){
      OCR0A = i;
      OCR0B = MAXBRITE-i;
      delay(50);
    }
    OCR0A = MAXBRITE;
    OCR0B = 0;  // special case, means output = 0
    delay(50);

    for (i = MAXBRITE; i > 3; i--){
      OCR0A = i;
      OCR0B = MAXBRITE-i;
      delay(50);
    }
    OCR0A = 0;  // special case, means output = 0
    OCR0B = MAXBRITE;
    delay(50);
  }
}
