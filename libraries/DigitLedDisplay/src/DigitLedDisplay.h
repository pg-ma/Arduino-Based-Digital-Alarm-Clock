#ifndef DigitLedDisplay_h
#define DigitLedDisplay_h

#if (defined(__AVR__))
#include <avr/pgmspace.h>
#else
#include <pgmspace.h>
#endif

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

const static byte charTable [] PROGMEM = {
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,B01111111,B01111011, B01110111, B01100111, B00111101, B00111110, B01000110, B00110111, B01001110, B01001111
};

class DigitLedDisplay
{
	private:
		int DIN_PIN;
		int CS_PIN;
		int CLK_PIN;
		int _digitLimit;
		void table(byte address, int val);	
	public:
		DigitLedDisplay(int dinPin, int csPin, int clkPin);
		void setBright(int brightness);
		void setDigitLimit(int limit);
		void printDigit(long number, byte startDigit = 0);
		void printCharA(byte digitPosition);
		void printCharP(byte digitPosition);
		void printCharD(byte digitPosition);
		void printCharU(byte digitPosition);
		void printCharT(byte digitPosition);
		void printCharH(byte digitPosition);
		void printCharC(byte digitPosition);
		void printCharE(byte digitPosition);
		void write(byte address, byte data);
		void clear();
		void on();
		void off();		
};

#endif	//DigitLedDisplay.h
