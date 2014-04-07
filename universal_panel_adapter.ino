// Viki Panel
#include "LiquidTWI2.h"
#include "utility/twi.h"
#include "Wire.h"
#include <Encoder.h>

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidTWI2 lcd(0);

char inbuf [32];
byte cmd_buf[32];

volatile int pos;
volatile int toread;
volatile boolean process_it;
volatile int buttons;
volatile long current_enc;

long last_enc;
long last_ms;

#define LE_ENCA 2
#define LE_ENCB 3
#define LED 13

// Command byte is cccnnnnn, where ccc is the command and nnnnn is the length of data in the command
enum Commands {
	READ,        // 0x00 read a result
	GET_BUTTONS, // 0x20 read buttons
	GET_ENCODER, // 0x40 Encoder delta
	LCD_WRITE,   // 0x60 write to LCD, yyyxxxxx, chars...
	LCD_CLEAR,   // 0x80 clear the LCD
	SET_LEDS,    // 0xA0 set the leds, leds mask
	BEEP, 		 // 0xC0 beep buzzer
};

Encoder enc(LE_ENCA, LE_ENCB);

void setup (void)
{
	pinMode(LED, OUTPUT);
  	lcd.setMCPType(LTI_TYPE_MCP23017);
  	// set up the LCD's number of rows and columns:
  	lcd.begin(20, 4);

  	//Serial.begin (115200);   // debugging

	// have to send on master in, *slave out*
	pinMode(MISO, OUTPUT);

	// turn on SPI in slave mode
	SPCR |= _BV(SPE);

	pos = 0;   // buffer empty
	process_it = false;

	last_enc= enc.read();
	last_ms= 0;

	// now turn on interrupts
  	SPCR |= _BV(SPIE);

}  // end of setup


// SPI interrupt routine
ISR (SPI_STC_vect)
{
	byte c = SPDR;  // grab byte from SPI Data Register

	if(pos == 0 && c == READ) return; // ignore 0 as that is a read

	// add to buffer if room
	if (pos < sizeof inbuf) {
		if(pos == 0) {
			toread= c&0x1F; // number of bytes in this command frame
			inbuf[pos++]= c >> 5; // command
			inbuf[pos++]= toread; // number of bytes in command frame
		}else{
			inbuf[pos++] = c;
			toread--;
		}

		if (toread == 0) {
			if(inbuf[0] == GET_BUTTONS) {
				// return current button state on next read
				SPDR = buttons & 0xFF;

			}else if(inbuf[0] == GET_ENCODER) {
				// return current encoder delta since last read
				SPDR = last_enc - current_enc;
				last_enc= current_enc;

			}else{
				// full command frame read transfer it to command buffer and get ready for next frame
				process_it = true;
				memcpy(cmd_buf, inbuf, pos);
			}
			pos= 0;
		}

	}  // end of room available
}  // end of interrupt routine SPI_STC_vect


void handle_command()
{
	int n;
	int x, y;
	switch(cmd_buf[0]) {
		case LCD_WRITE:
			// number of bytes to write
			n= cmd_buf[1];
			// position to write to
			x= cmd_buf[2]&0x1F;
			y= cmd_buf[2]>>5;
			cmd_buf[3+n]= 0;

			lcd.setCursor(x, y);
			lcd.print((char *)&cmd_buf[3]);
			break;

		case LCD_CLEAR:
			lcd.clear();
			break;

		case SET_LEDS:
			lcd.setBacklight(cmd_buf[2]);
			break;

		case BEEP:
			lcd.buzz(100, 1000);
			break;
	}
}

// main loop - wait for flag set in interrupt routine
void loop (void)
{
	if (process_it) {
		process_it= false;
		handle_command();
	}

	long now= millis();

	long delta= now-last_ms;

	if(delta > 10) { // read every 10 ms
		buttons= lcd.readButtons();
		current_enc= enc.read();
		last_ms= now;
	}

}  // end of loop

