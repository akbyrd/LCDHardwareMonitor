import board
import digitalio
cs_pin = digitalio.DigitalInOut(board.C0)
dc_pin = digitalio.DigitalInOut(board.C1)

import adafruit_rgb_display.ili9341 as ili9341
spi = board.SPI()
display = ili9341.ILI9341(spi, cs=cs_pin, dc=dc_pin, baudrate=64000000)

from adafruit_rgb_display import color565
#display.fill(color565(0xff, 0x11, 0x22))
#display.pixel(160, 120, color565(0x00, 0xff, 0xff))

command = 0x0A
data = display.read(command, 2)
print('cmd', '0x{:02X} '.format(command))
print('data', ''.join('0x{:02X} '.format(x) for x in data))
# cmd  0x0A
# data 0x9C 0x00 (1001'1100)
