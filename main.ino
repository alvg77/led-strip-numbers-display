#include <Adafruit_NeoPixel.h>

class LEDMatrix {
  const uint16_t numLayouts[10] = {
    0b111101101101111, // 0
    0b001001001001001, // 1
    0b111001111100111, // 2
    0b111001111001111, // 3
    0b101101111001001, // 4
    0b111100111001111, // 5
    0b111100111101111, // 6
    0b111001001001001, // 7
    0b111101111101111, // 8
    0b111101111001001  // 9
  };

  const unsigned int pixelsPerStrip;
  const unsigned int ledStripNum;
  const uint8_t ledPin;
  Adafruit_NeoPixel pixels;

public:
  // class constructor
  LEDMatrix(unsigned int pixelsPerStrip, unsigned int ledStripNum, uint8_t ledPin)
    : pixelsPerStrip(pixelsPerStrip), // init pixels per strip
      ledStripNum(ledStripNum), // init the number of led strips
      ledPin(ledPin), // init the led pin
      pixels(pixelsPerStrip * ledStripNum, ledPin, NEO_GRB + NEO_KHZ800) // init the Adafruit_NeoPixel object
  {
    pixels.begin();
  }

  // class deconstructor - no dynamic resouces, so it is empty
  ~LEDMatrix() {}
  
  // reverse number from DIP
  // this is required when entering double digit numbers like 15
  // the algorithm in the displayNumbers function takes the last digit and displays it first
  // so to display 15 like 15 and not 51 we need to reverse it
  unsigned int reverse(unsigned int number) {
  	int reversedNumber = 0;
    while (number > 0) {
      int digit = number % 10;
      reversedNumber = (reversedNumber * 10) + digit;
      number /= 10;
    }
    return reversedNumber;
  }
  
  
  // display algorithm
  // takes the last digit of the number and displays it first
  // because of this we need to reverse the number before displaying it
  void displayNumbers(unsigned int number, unsigned int offset) {
    offset %= (pixelsPerStrip / 4); // get the proper offset for the led strip
    
    int currentPosition = offset * 4; // calculate current pixel position 
    number = reverse(number); // reverse number to display correctly
    uint8_t isZero = 0; // flag to check if there are no more digits left in the number to display
    
    while (isZero != 1) {
      int digit = number % 10; // get the last digit
      number /= 10; // remove the last digit
	  isZero = number == 0 ? 1 : 0; // check if the number after division is 0
      
      for (int i = 0; i < ledStripNum; i++) {
        // get the number bits for the row
        uint8_t rowBits = (numLayouts[digit] >> (ledStripNum - i - 1) * 3) & 0b111;
        // iterate over the pixels with the specified ident
        for (int j = currentPosition; j < currentPosition + 4; j++) {
          // check if the leftmost nuber bit is 1 (glow) or 0 (don't glow)
          if (rowBits & 0b100) {
            pixels.setPixelColor(j + (i * pixelsPerStrip), pixels.Color(255, 0, 0));
          } else {
            pixels.setPixelColor(j + (i * pixelsPerStrip), pixels.Color(0, 0, 0));
          }
          // we shift the row bits with one to the left so we can check the next bit
          rowBits <<= 1;
        }
      }
	  
      // in the case of multidigit numbers we update the current position
      currentPosition = (currentPosition + 4) % pixelsPerStrip;
	}
  }

  void show() {
  	pixels.show();
  }
  
};

// define pin constants
const unsigned int ledPin = 7;
const unsigned int ledStripNum = 5;
const unsigned int pixelsPerStrip = 12;
LEDMatrix ledMatrix(pixelsPerStrip, ledStripNum, ledPin);

// define volatile variables for the dip input
volatile unsigned int numberToDisplay;
volatile unsigned int offsetToDisplay;

void configDip() {
  cli();
  
  DDRD &= ~((1 << DDD2) | (1 << DDD3) | (1 << DDD4) | (1 << DDD5));
  PORTD |= ((1 << PORTD3) | (1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6));
    
  DDRB &= ~((1 << DDB0) | (1 << DDB1) | (1 << DDB2) | (1 << DDB3));
  PORTB |= ((1 << PORTB0) | (1 << PORTB1) | (1 << PORTB2) | (1 << PORTB3));
  
  PCICR |= (1 << PCIE2);
  PCICR |= (1 << PCIE0);
  
  PCMSK2 |= ((1 << PCINT22) | (1 << PCINT21) | (1 << PCINT20) | (1 << PCINT19));
  PCMSK0 |= ((1 << PCINT3) | (1 << PCINT2) | (1 << PCINT1) | (1 << PCINT0));
  
  sei();
}

ISR(PCINT2_vect) {
  // we use pins 3,4,5,6 so we need to shift with 3 bits to the left to get the number from the bits
  // we AND this with 0b1111 to be sure we are getting only the desired bits
  numberToDisplay = (PIND >> 3) & 0b1111;
}

ISR(PCINT0_vect) {
  // the same as with the other interrupt handler
  offsetToDisplay = PINB & 0b1111;
}


void setup() {
  configDip();
}

void loop() {
  ledMatrix.displayNumbers(numberToDisplay, offsetToDisplay);
  ledMatrix.show();
  delay(1000);
}