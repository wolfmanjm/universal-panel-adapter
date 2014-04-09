// Viki Panel
#include "LiquidTWI2.h"
#include "utility/twi.h"
#include "Wire.h"
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
volatile long current_enc;

long last_enc;
long last_ms;

#define LE_ENCA 2
#define LE_ENCB 3
#define BUSY_PIN 4

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
bool selected;

void setup (void)
{
  	lcd.setMCPType(LTI_TYPE_MCP23017);
  	// set up the LCD's number of rows and columns:
  	lcd.begin(20, 4);

  	//Serial.begin (115200);   // debugging

	// have to send on master in, *slave out*
	//pinMode(MISO, OUTPUT);
 	DDRB &= ~((1<<2)|(1<<3)|(1<<5));   // SCK, MOSI and SS as inputs
    DDRB |= (1<<4);                    // MISO as output

	pinMode(BUSY_PIN, OUTPUT);
	digitalWrite(BUSY_PIN, LOW);

	// turn on SPI in slave mode
	//SPCR |= _BV(SPE);
	SPCR &= ~(1<<MSTR);                // Set as slave
    SPCR |= (1<<SPR0)|(1<<SPR1);       // divide clock by 128
    SPCR |= (1<<SPE);                  // Enable SPI


	pos = 0;   // buffer empty
	queue.clear();

	last_enc= enc.read();
	last_ms= 0;
	selected=false;

	lcd.setCursor(0, 0);
	lcd.print("UPA V0.9");
	lcd.setCursor(0, 1);
	lcd.print("Starting up...");
}  // end of setup


// SPI interrupt routine
ISR (SPI_STC_vect)
{
	byte b = SPDR;  // grab byte from SPI Data Register

	if(b == 0xFF) { // polling for free queue
		if(queue.size() < queue.capacity()-2) {
			// if queue is partially empty deassert busy
			digitalWrite(BUSY_PIN, LOW);
		}
		return;
	}

	if(pos == 0){
		switch(b >> 5) { // command
			case READ: return; // ignore 0 as that is a read
			case GET_BUTTONS:
				// return current button state on next read
				SPDR = buttons;
				return;
			case GET_ENCODER:
				// return current encoder delta since last read
				SPDR = last_enc - current_enc;
				last_enc= current_enc;
				return;

			default:
				toread= b&0x1F; // number of bytes in this command frame
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
			if(queue.size() >= queue.capacity()-2) {
				digitalWrite(BUSY_PIN, HIGH); // indicate busy here to avoid race condition
			}
			pos= 0;
		}
	}

}  // end of interrupt routine SPI_STC_vect


void handle_command(Cmd_t& c)
{
	int n= c.len;
	int x, y;

	switch(c.cmd) {
		case LCD_WRITE:
			if(n >= 2) {
				// position to write to
				x= c.data[0]&0x1F;
				y= c.data[0]>>5;
				c.data[n]= 0;

				lcd.setCursor(x, y);
				lcd.print((char *)&c.data[1]);
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
	}
}

// main loop - wait for flag set in interrupt routine
void loop (void)
{
	while(digitalRead(SS) == HIGH) {
		// deselected ignore everything
		if(selected) {
			selected= false;
  			SPCR &= ~_BV(SPIE); // turn off interrupts
  		}
	}

	// host will toggle SS to reset us
	if(!selected) {
		pos= 0;
		buttons= 0;
		enc.write(0);
		last_enc= 0;
		lcd.clear();
		lcd.setBacklight(0);
		queue.clear();
  		digitalWrite(BUSY_PIN, LOW);
  		selected= true;
		// now turn on interrupts
  		SPCR |= _BV(SPIE);
	}

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

	// this is safe to read every loop as it just copies a variable
	current_enc= enc.read();
}

