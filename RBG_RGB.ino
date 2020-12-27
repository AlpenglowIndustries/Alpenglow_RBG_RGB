// Test Program for ATTiny4

#include <avr/io.h>
#include <stdint.h>

int main (void) {
  DDRB = 1;                       // PB0 as an output
  TCCR0A = 1<<COM0A0 | 0<<WGM00;  // Toggle OC0A, CTC mode
  TCCR0B = 1<<WGM02 | 3<<CS00;    // CTC mode; use OCR0A; /64
  OCR0A = 15624;                  // 1 second; ie 0.5Hz
  while (1);
}
