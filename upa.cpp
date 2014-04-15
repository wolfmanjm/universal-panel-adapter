// Viki Panel
#include "Arduino.h"
#include "LiquidTWI2.h"
#include "utility/twi.h"
#include "Wire.h"
#include "digitalWriteFast.h"

#include <Encoder.h>
#include "RingBuffer.h"

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidTWI2 lcd(0);

typedef struct {
	byte cmd;
	byte len;
	byte data[32];
} Cmd_t;

// commands get entered here in the ISR, and pulled off in main loop
RingBuffer<Cmd_t, 32> queue;
Cmd_t buf;

volatile int pos;
volatile int toread;
volatile byte buttons;

long last_ms;

#define LE_ENCA 2
#define LE_ENCB 3
#define BUSY_PIN 4

// Command byte is cccnnnnn, where ccc is the command and nnnnn is the length of data in the command
// 0xFF is a poll for queue empty
// 0xFE is Initialize
// 0x00 is a read results of previous command
enum Commands {
	READ,        // 0x00 read a result
	GET_STATUS,  // 0x20 read buttons or encoder size is 0 for buttons, 1 for encoder
	SET_CURSOR,  // 0x40 Set the cursor, 1 parameter yyyxxxxx, where yyy is the row and xxxxx is the column
	LCD_WRITE,   // 0x60 write to LCD, n chars
	LCD_CLEAR,   // 0x80 clear the LCD
	SET_LEDS,    // 0xA0 set the leds, 1 parameter, the led mask of leds to set
	BEEP, 		 // 0xC0 beep buzzer, optionally pass duration, frequency
	OTHER,		 // 0xE0 some other no parameter command specified by entire command byte
	// Other commands... 0xE0 thru 0xFF
	INIT= 0xFE,  // Initialize
	POLL= 0xFF   // Poll for queue empty
};

#define READ_BUTTONS 0
#define READ_ENCODER 1
#define READ_QUEUE   2

Encoder enc(LE_ENCA, LE_ENCB);

// SPI interrupt routine
ISR (SPI_STC_vect)
{
	byte b = SPDR;  // grab byte from SPI Data Register

	if(pos == 0){
		// first byte in frame is always a command
		toread= b&0x1F; // number of bytes in this command frame
		switch(b >> 5) { // command
			case READ: return; // ignore 0 as that is a read

			case GET_STATUS:
				if(toread == READ_BUTTONS) {
					// return current button state on next read
					SPDR = buttons;

				}else if(toread == READ_ENCODER) {
					// return current encoder delta since last read
					SPDR = enc.read();
					enc.write(0);

				}else if(toread == READ_QUEUE) {
					SPDR= queue.size();
				}
				return;

			case OTHER:
				if(b == INIT) {
					// INIT
					buf.cmd= INIT;
					buf.len= 0;
					toread= 0;
					queue.clear(); // make sure command gets queued

				}else if(b == POLL){
					if(queue.isEmpty()) {
						// if queue is empty deassert busy
						digitalWriteFast(BUSY_PIN, LOW);
					}
					return;
				}
				break;

			default:
				buf.cmd= b >> 5; // command
				buf.len= toread; // number of bytes in command frame
				pos= 1;
				if(toread > 0) return; // get the next byte of this command
		}
	}

	if (toread > 0) {
		if(pos < sizeof(buf.data)) // avoid overflow, truncate message if necessary
			buf.data[pos-1] = b;
		pos++;
		toread--;
	}

	if (toread == 0) {
		// full command frame read transfer it to command buffer and get ready for next frame
		if(!queue.isFull()) { // this should never fail as busy should have been asserted
			queue.pushBack(buf);
			if(queue.size() >= queue.capacity()-10) {
				digitalWriteFast(BUSY_PIN, HIGH); // indicate busy here to avoid race condition
				//Serial.print("Busy\r\n");
			}

		}else{
			//Serial.print("WARNING: Queue overflow\r\n");
		}
		pos= 0;
	}

}  // end of interrupt routine SPI_STC_vect

void clear()
{
	pos= 0;
	last_ms= 0;
	buttons= 0;
	enc.write(0);
	lcd.clear();
	lcd.setBacklight(0);
	queue.clear();
	digitalWrite(BUSY_PIN, LOW);
}

void setup (void)
{
  	lcd.setMCPType(LTI_TYPE_MCP23017);
  	// set up the LCD's number of rows and columns:
  	lcd.begin(20, 4);

  	//Serial.begin (115200);   // debugging
  	//Serial.print("Starting...\r\n");

	// have to send on master in, *slave out*
	//pinMode(MISO, OUTPUT);
 	DDRB &= ~((1<<2)|(1<<3)|(1<<5));   // SCK, MOSI and SS as inputs
    DDRB |= (1<<4);                    // MISO as output

	pinMode(BUSY_PIN, OUTPUT);
	digitalWrite(BUSY_PIN, LOW);

	// turn on SPI in slave mode
	//SPCR |= _BV(SPE);
	SPCR &= ~(1<<MSTR);                // Set as slave
    SPCR &= ~((1<<SPR0)|(1<<SPR1));    // fastest
    SPCR |= (1<<SPE);                  // Enable SPI
  	SPCR |= _BV(SPIE);				   // turn on interrupts

    clear();

	lcd.setCursor(0, 0);
	lcd.print("UPA V0.98");
	lcd.setCursor(0, 1);
	lcd.print("Starting up...");
}  // end of setup

void handle_command(Cmd_t& c)
{
	int n= c.len;
	int x, y;

	switch(c.cmd) {
		case LCD_WRITE:
			if(n > 0 && n <= sizeof(c.data)) {
				for (int i = 0; i < n; ++i) {
					lcd.write(c.data[i]);
				}
			}
			break;

		case SET_CURSOR:
			if(n == 1) {
				// position to write to
				x= c.data[0]&0x1F;
				y= c.data[0]>>5;
				lcd.setCursor(x, y);
			}
			break;

		case LCD_CLEAR:
			lcd.clear();
			break;

		case SET_LEDS:
			if(n == 1) {
				lcd.setBacklight(c.data[0]);
			}
			break;

		case BEEP:
			if(n == 3) {
				lcd.buzz(c.data[0], (int)(c.data[1])<<8 | (c.data[2]&0xFF));
			}else{
				lcd.buzz(100, 1000);
			}
			break;

		case INIT: // special case command Initialize
			clear();
			break;
	}
}

// main loop - wait for commands
void loop (void)
{
	if (!queue.isEmpty()) {
		Cmd_t cmd;
		queue.popFront(cmd);
		handle_command(cmd);
	}

	long now= millis();
	long delta= now-last_ms;

	if(delta > 50) { // read buttons every 50 ms, 20 times/sec
		buttons= lcd.readButtons();
		last_ms= now;
	}
}

