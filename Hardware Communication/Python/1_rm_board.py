from adafruit_blinka.microcontroller.ft232h import pin

D4 = pin.D4
D5 = pin.D5
D6 = pin.D6
D7 = pin.D7
C0 = pin.C0
C1 = pin.C1
C2 = pin.C2
C3 = pin.C3
C4 = pin.C4
C5 = pin.C5
C6 = pin.C6
C7 = pin.C7

SDA = pin.SDA
SCL = pin.SCL

SCK = pin.SCK
SCLK = pin.SCLK
MOSI = pin.MOSI
MISO = pin.MISO

import digitalio
cs_pin = digitalio.DigitalInOut(C0)
dc_pin = digitalio.DigitalInOut(C1)

import adafruit_rgb_display.ili9341 as ili9341
import busio
spi = busio.SPI(SCLK, MOSI, MISO)
display = ili9341.ILI9341(spi, cs=cs_pin, dc=dc_pin, baudrate=64000000)

from adafruit_rgb_display import color565
display.fill(color565(0xff, 0xff, 0x22))
