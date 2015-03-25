universal-panel-adapter
=======================

Using a cheap Arduino mini pro, uno or nano this sketch allows you to
interface a panel like Viki or a Parallel LCD with encoder via SPI.

Specifically used for running I2C and parallel panels on a Smoothie compatible board.

Prebuild bianries ready for upload to arduino are:-

* viki_panel_adapter.hex for viki/panelolu2
* parallel_panel_adapter.hex for parallel LCD

Default Wiring is as follows:-

Viki to Nano
---------------

	SDA  -> A4
	SCL  -> A5
	ENCA -> D2
	ENCB -> D3
	Gnd  -> Gnd
	+5v  -> +5v

Parallel LCD pin to Nano
--------------------

	 4 RS -> D9
	 5 RW -> A0
	 6 EN -> A1
	11 DB4 -> D5
	12 DB5 -> D6
	13 DB6 -> D7
	14 DB7 -> D8

	 1 gnd
	 2 +5v
	 3 contrast
	 15,16 backlight power (on some)

PanelOne (5V) LCD pin to Nano
--------------------

	AUX2  6 RS    -> D9
	AUX2  8 EN    -> A1
	AUX2 10 DB4   -> D5
	AUX2  9 DB5   -> D6
	AUX2  7 DB6   -> D7
	AUX2  5 DB7   -> D8
	AUX2  4 ENCA  -> D2
	AUX2  3 ENCB  -> D3
	AUX3  7 ENCSW -> A2
	
	AUX2 2 -> GND
	AUX2 1 -> +5V

PanelOne (5V) SD pin to Smoothie
--------------------

	AUX3 3  CS -> Unused GPIO : 1.30
	AUX3 4 CLK -> SCK  : 0.15
	AUX3 5  DO -> MISO : 0.17
	AUX3 6  DI -> MOSI : 0.18
	
	AUX3 2 -> GND
	AUX3 8 -> +5V

Smoothie to Nano
----------------
	MOSI -> D11 : 0.18
	MISO -> D12 : 0.17
	SCK  -> D13 : 0.15
	SS   -> D10 : 0.16
	BUSY -> D4  : 2.11 (or 1.30 on Azteeg X5) (And set panel.busy_pin in config to that pin)

Smoothie config
---------------

add this to your config file on smoothie

	panel.enable                                true              # enable panel
	panel.lcd                                   universal_adapter #
	panel.spi_channel                           0                 # spi channel to use (0- MISO 0.17, MOSI 0.18, SCK 0.15, SS 0.16)
	panel.spi_cs_pin                            0.16              # spi chip select
	panel.busy_pin                              2.11              # busy pin NOTE 1.30 on Azteeg X5

second SD card on panelone

	panel.external_sd                     true              # set to true if there is an extrernal sdcard on the panel
	panel.external_sd.spi_channel         0                 # set spi channel the sdcard is on
	panel.external_sd.spi_cs_pin          1.30              # set spi chip select for the sdcard (or any spare pin)
	panel.external_sd.sdcd_pin            nc                # sd detect signal (set to nc if no sdcard detect) (or any spare pin)

Requirements
------------
For compiling the following Arduino libraries are needed

* Encoder library from http://www.pjrc.com/teensy/td_libs_Encoder.html

For Viki and I2C based panels
* LiquidTWI2 from https://github.com/lincomatic/LiquidTWI2

For Parallel panels
* LiquidCrystalFast from https://www.pjrc.com/teensy/td_libs_LiquidCrystal.html



