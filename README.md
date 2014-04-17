universal-panel-adapter
=======================

Using a cheap Arduino mini pro, uno or nano this sketch allows you to
interface a panel like Viki or a Parallel LCD with encoder via SPI.

Specifically used for running I2C and parallel panels on a Smoothie compatible board.

Prebuild bianries ready for upload to arduino are:-

* viki_panel_adapter.hex for viki/panelolu2
* parallel_panel_adapter.hex for parallel LCD

Default Wiring is as fallows:-

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
    11 D4 -> D5
	12 D5 -> D6
	13 D6 -> D7
	14 D7 -> D8

	 1 gnd
	 2 +5v
	 3 contrast
	 15,16 backlight power (on some)

Smoothie to Nano
----------------
	MOSI -> D11
	MISO -> D12
	SCK -> D13
	SS -> D10
	BUSY -> D4 (And set panel.busy_pin in config to a spare Smoothie pin)

Smoothie config
---------------

add this to your config file on smoothie
    panel.enable                                true              # enable panel
	panel.lcd                                   universal_adapter #
	panel.spi_channel                           0                 # spi channel to use (0- MISO 0.17, MOSI 0.18, SCK 0.15, SS 0.16)
	panel.spi_cs_pin                            0.16              # spi chip select
	panel.busy_pin                              2.13              # busy pin (Azteeg X5 use 2.11)


Requirements
------------
For compiling the following Arduino libraries are needed

* Encoder library from http://www.pjrc.com/teensy/td_libs_Encoder.html

For Viki and I2C based panels
* LiquidTWI2 from https://github.com/lincomatic/LiquidTWI2

For Parallel panels
* LiquidCrystalFast from https://www.pjrc.com/teensy/td_libs_LiquidCrystal.html



