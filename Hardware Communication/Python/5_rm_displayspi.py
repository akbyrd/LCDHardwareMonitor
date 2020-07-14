class Pin:
    """A basic Pin class for use with FT232H."""

    IN = 0
    OUT = 1
    LOW = 0
    HIGH = 1

    ft232h_gpio = None

    def __init__(self, pin_id=None):
        # setup GPIO controller if not done yet
        # use one provided by I2C as default
        if not Pin.ft232h_gpio:
            # pylint: disable=import-outside-toplevel
            from pyftdi.i2c import I2cController

            # pylint: enable=import-outside-toplevel

            i2c = I2cController()
            i2c.configure("ftdi://ftdi:ft232h/1")
            Pin.ft232h_gpio = i2c.get_gpio()
        # check if pin is valid
        if pin_id:
            if Pin.ft232h_gpio.all_pins & 1 << pin_id == 0:
                raise ValueError("Can not use pin {} as GPIO.".format(pin_id))
        # ID is just bit position
        self.id = pin_id

    def init(self, mode=IN, pull=None):
        """Initialize the Pin"""
        if not self.id:
            raise RuntimeError("Can not init a None type pin.")
        # FT232H does't have configurable internal pulls?
        if pull:
            raise ValueError("Internal pull up/down not currently supported.")
        pin_mask = Pin.ft232h_gpio.pins | 1 << self.id
        current = Pin.ft232h_gpio.direction
        if mode == self.OUT:
            current |= 1 << self.id
        else:
            current &= ~(1 << self.id)
        Pin.ft232h_gpio.set_direction(pin_mask, current)

    def value(self, val=None):
        """Set or return the Pin Value"""
        if not self.id:
            raise RuntimeError("Can not access a None type pin.")
        current = Pin.ft232h_gpio.read(with_output=True)
        # read
        if val is None:
            return 1 if current & 1 << self.id != 0 else 0
        # write
        if val in (self.LOW, self.HIGH):
            if val == self.HIGH:
                current |= 1 << self.id
            else:
                current &= ~(1 << self.id)
            # must mask out any input pins
            Pin.ft232h_gpio.write(current & Pin.ft232h_gpio.direction)
            return None
        # release the kraken
        raise RuntimeError("Invalid value for pin")

# create pin instances for each pin
# D0 to D3 are used by I2C/SPI
D4 = Pin(4)
D5 = Pin(5)
D6 = Pin(6)
D7 = Pin(7)
C0 = Pin(8)
C1 = Pin(9)
C2 = Pin(10)
C3 = Pin(11)
C4 = Pin(12)
C5 = Pin(13)
C6 = Pin(14)
C7 = Pin(15)
# C8 and C9 are not GPIO

# create None type pins for I2C and SPI since they are expected to be defined
SCL = Pin()
SDA = Pin()
SCK = SCLK = Pin()
MOSI = Pin()
MISO = Pin()

class Enum:
    """
        Object supporting CircuitPython-style of static symbols
        as seen with Direction.OUTPUT, Pull.UP
    """

    def __repr__(self):
        """
        Assumes instance will be found as attribute of own class.
        Returns dot-subscripted path to instance
        (assuming absolute import of containing package)
        """
        cls = type(self)
        for key in dir(cls):
            if getattr(cls, key) is self:
                return "{}.{}.{}".format(cls.__module__, cls.__qualname__, key)
        return repr(self)

    @classmethod
    def iteritems(cls):
        """
            Inspects attributes of the class for instances of the class
            and returns as key,value pairs mirroring dict#iteritems
        """
        for key in dir(cls):
            val = getattr(cls, key)
            if isinstance(cls, val):
                yield (key, val)

class ContextManaged:
    """An object that automatically deinitializes hardware with a context manager."""

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.deinit()

    def deinit(self):
        """Free any hardware used by the object."""
        return

class DriveMode(Enum):
    PUSH_PULL = None
    OPEN_DRAIN = None
DriveMode.PUSH_PULL = DriveMode()
DriveMode.OPEN_DRAIN = DriveMode()


class Direction(Enum):
    INPUT = None
    OUTPUT = None
Direction.INPUT = Direction()
Direction.OUTPUT = Direction()


class Pull(Enum):
    UP = None
    DOWN = None
Pull.UP = Pull()
Pull.DOWN = Pull()

class DigitalInOut(ContextManaged):
    _pin = None

    def __init__(self, pin):
        self._pin = Pin(pin.id)
        self.direction = Direction.INPUT

    def switch_to_output(self, value=False, drive_mode=DriveMode.PUSH_PULL):
        self.direction = Direction.OUTPUT
        self.value = value
        self.drive_mode = drive_mode

    def switch_to_input(self, pull=None):
        self.direction = Direction.INPUT
        self.pull = pull

    def deinit(self):
        del self._pin

    @property
    def direction(self):
        return self.__direction

    @direction.setter
    def direction(self, dir):
        self.__direction = dir
        if dir is Direction.OUTPUT:
            self._pin.init(mode=Pin.OUT)
            self.value = False
            self.drive_mode = DriveMode.PUSH_PULL
        elif dir is Direction.INPUT:
            self._pin.init(mode=Pin.IN)
            self.pull = None
        else:
            raise AttributeError("Not a Direction")

    @property
    def value(self):
        return self._pin.value() == 1

    @value.setter
    def value(self, val):
        if self.direction is Direction.OUTPUT:
            self._pin.value(1 if val else 0)
        else:
            raise AttributeError("Not an output")

    @property
    def pull(self):
        if self.direction is Direction.INPUT:
            return self.__pull
        else:
            raise AttributeError("Not an input")

    @pull.setter
    def pull(self, pul):
        if self.direction is Direction.INPUT:
            self.__pull = pul
            if pul is Pull.UP:
                self._pin.init(mode=Pin.IN, pull=Pin.PULL_UP)
            elif pul is Pull.DOWN:
                if hasattr(Pin, "PULL_DOWN"):
                    self._pin.init(mode=Pin.IN, pull=Pin.PULL_DOWN)
                else:
                    raise NotImplementedError(
                        "{} unsupported on {}".format(Pull.DOWN, board_id)
                    )
            elif pul is None:
                self._pin.init(mode=Pin.IN, pull=None)
            else:
                raise AttributeError("Not a Pull")
        else:
            raise AttributeError("Not an input")

    @property
    def drive_mode(self):
        if self.direction is Direction.OUTPUT:
            return self.__drive_mode
        else:
            raise AttributeError("Not an output")

    @drive_mode.setter
    def drive_mode(self, mod):
        self.__drive_mode = mod
        if mod is DriveMode.OPEN_DRAIN:
            self._pin.init(mode=Pin.OPEN_DRAIN)
        elif mod is DriveMode.PUSH_PULL:
            self._pin.init(mode=Pin.OUT)

cs_pin = DigitalInOut(C0)
dc_pin = DigitalInOut(C1)

import busio
spi = busio.SPI(SCLK, MOSI, MISO)

import struct
# This is the size of the buffer to be used for fill operations, in 16-bit
# units.
_BUFFER_SIZE = 320 * 240  # blit the whole thing

class Display:  # pylint: disable-msg=no-member
    """Base class for all RGB display devices
        :param width: number of pixels wide
        :param height: number of pixels high
    """

    _PAGE_SET = None
    _COLUMN_SET = None
    _RAM_WRITE = None
    _RAM_READ = None
    _X_START = 0  # pylint: disable=invalid-name
    _Y_START = 0  # pylint: disable=invalid-name
    _INIT = ()
    _ENCODE_PIXEL = ">H"
    _ENCODE_POS = ">HH"
    _DECODE_PIXEL = ">BBB"

    def __init__(self, width, height, rotation):
        self.width = width
        self.height = height
        if rotation not in (0, 90, 180, 270):
            raise ValueError("Rotation must be 0/90/180/270")
        self._rotation = rotation
        self.init()

    def init(self):
        """Run the initialization commands."""
        for command, data in self._INIT:
            self.write(command, data)

    # pylint: disable-msg=invalid-name,too-many-arguments
    def _block(self, x0, y0, x1, y1, data=None):
        """Read or write a block of data."""
        self.write(
            self._COLUMN_SET, self._encode_pos(x0 + self._X_START, x1 + self._X_START)
        )
        self.write(
            self._PAGE_SET, self._encode_pos(y0 + self._Y_START, y1 + self._Y_START)
        )
        if data is None:
            size = struct.calcsize(self._DECODE_PIXEL)
            return self.read(self._RAM_READ, (x1 - x0 + 1) * (y1 - y0 + 1) * size)
        self.write(self._RAM_WRITE, data)
        return None

    # pylint: enable-msg=invalid-name,too-many-arguments

    def _encode_pos(self, x, y):
        """Encode a postion into bytes."""
        return struct.pack(self._ENCODE_POS, x, y)

    def _encode_pixel(self, color):
        """Encode a pixel color into bytes."""
        return struct.pack(self._ENCODE_PIXEL, color)

    def _decode_pixel(self, data):
        """Decode bytes into a pixel color."""
        return color565(*struct.unpack(self._DECODE_PIXEL, data))

    def pixel(self, x, y, color=None):
        """Read or write a pixel at a given position."""
        if color is None:
            return self._decode_pixel(self._block(x, y, x, y))

        if 0 <= x < self.width and 0 <= y < self.height:
            self._block(x, y, x, y, self._encode_pixel(color))
        return None

    def image(self, img, rotation=None, x=0, y=0):
        """Set buffer to value of Python Imaging Library image. The image should
        be in 1 bit mode and a size not exceeding the display size when drawn at
        the supplied origin."""
        if rotation is None:
            rotation = self.rotation
        if not img.mode in ("RGB", "RGBA"):
            raise ValueError("Image must be in mode RGB or RGBA")
        if rotation not in (0, 90, 180, 270):
            raise ValueError("Rotation must be 0/90/180/270")
        if rotation != 0:
            img = img.rotate(rotation, expand=True)
        imwidth, imheight = img.size
        if x + imwidth > self.width or y + imheight > self.height:
            raise ValueError(
                "Image must not exceed dimensions of display ({0}x{1}).".format(
                    self.width, self.height
                )
            )
        if numpy:
            pixels = list(image_to_data(img))
        else:
            # Slower but doesn't require numpy
            pixels = bytearray(imwidth * imheight * 2)
            for i in range(imwidth):
                for j in range(imheight):
                    pix = color565(img.getpixel((i, j)))
                    pixels[2 * (j * imwidth + i)] = pix >> 8
                    pixels[2 * (j * imwidth + i) + 1] = pix & 0xFF
        self._block(x, y, x + imwidth - 1, y + imheight - 1, pixels)

    # pylint: disable-msg=too-many-arguments
    def fill_rectangle(self, x, y, width, height, color):
        """Draw a rectangle at specified position with specified width and
        height, and fill it with the specified color."""
        x = min(self.width - 1, max(0, x))
        y = min(self.height - 1, max(0, y))
        width = min(self.width - x, max(1, width))
        height = min(self.height - y, max(1, height))
        self._block(x, y, x + width - 1, y + height - 1, b"")
        chunks, rest = divmod(width * height, _BUFFER_SIZE)
        pixel = self._encode_pixel(color)
        if chunks:
            data = pixel * _BUFFER_SIZE
            for _ in range(chunks):
                self.write(None, data)
        self.write(None, pixel * rest)

    # pylint: enable-msg=too-many-arguments

    def fill(self, color=0):
        """Fill the whole display with the specified color."""
        self.fill_rectangle(0, 0, self.width, self.height, color)

    def hline(self, x, y, width, color):
        """Draw a horizontal line."""
        self.fill_rectangle(x, y, width, 1, color)

    def vline(self, x, y, height, color):
        """Draw a vertical line."""
        self.fill_rectangle(x, y, 1, height, color)

    @property
    def rotation(self):
        """Set the default rotation"""
        return self._rotation

    @rotation.setter
    def rotation(self, val):
        if val not in (0, 90, 180, 270):
            raise ValueError("Rotation must be 0/90/180/270")
        self._rotation = val

import adafruit_bus_device.spi_device as spi_device
class DisplaySPI(Display):
    """Base class for SPI type devices"""

    # pylint: disable-msg=too-many-arguments
    def __init__(
        self,
        spi,
        dc,
        cs,
        rst=None,
        width=1,
        height=1,
        baudrate=12000000,
        polarity=0,
        phase=0,
        *,
        x_offset=0,
        y_offset=0,
        rotation=0
    ):
        self.spi_device = spi_device.SPIDevice(
            spi, cs, baudrate=baudrate, polarity=polarity, phase=phase
        )
        self.dc_pin = dc
        self.rst = rst
        self.dc_pin.switch_to_output(value=0)
        if self.rst:
            self.rst.switch_to_output(value=0)
            self.reset()
        self._X_START = x_offset  # pylint: disable=invalid-name
        self._Y_START = y_offset  # pylint: disable=invalid-name
        super().__init__(width, height, rotation)

    # pylint: enable-msg=too-many-arguments

    def reset(self):
        """Reset the device"""
        self.rst.value = 0
        time.sleep(0.050)  # 50 milliseconds
        self.rst.value = 1
        time.sleep(0.050)  # 50 milliseconds

    # pylint: disable=no-member
    def write(self, command=None, data=None):
        """SPI write to the device: commands and data"""
        if command is not None:
            self.dc_pin.value = 0
            with self.spi_device as spi:
                spi.write(bytearray([command]))
        if data is not None:
            self.dc_pin.value = 1
            with self.spi_device as spi:
                spi.write(data)

    def read(self, command=None, count=0):
        """SPI read from device with optional command"""
        data = bytearray(count)
        self.dc_pin.value = 0
        with self.spi_device as spi:
            if command is not None:
                spi.write(bytearray([command]))
            if count:
                spi.readinto(data)
        return data

class ILI9341(DisplaySPI):
    _COLUMN_SET = 0x2A
    _PAGE_SET = 0x2B
    _RAM_WRITE = 0x2C
    _RAM_READ = 0x2E
    _INIT = (
        (0xEF, b"\x03\x80\x02"),
        (0xCF, b"\x00\xc1\x30"),
        (0xED, b"\x64\x03\x12\x81"),
        (0xE8, b"\x85\x00\x78"),
        (0xCB, b"\x39\x2c\x00\x34\x02"),
        (0xF7, b"\x20"),
        (0xEA, b"\x00\x00"),
        (0xC0, b"\x23"),  # Power Control 1, VRH[5:0]
        (0xC1, b"\x10"),  # Power Control 2, SAP[2:0], BT[3:0]
        (0xC5, b"\x3e\x28"),  # VCM Control 1
        (0xC7, b"\x86"),  # VCM Control 2
        (0x36, b"\x48"),  # Memory Access Control
        (0x3A, b"\x55"),  # Pixel Format
        (0xB1, b"\x00\x18"),  # FRMCTR1
        (0xB6, b"\x08\x82\x27"),  # Display Function Control
        (0xF2, b"\x00"),  # 3Gamma Function Disable
        (0x26, b"\x01"),  # Gamma Curve Selected
        (
            0xE0,  # Set Gamma
            b"\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00",
        ),
        (
            0xE1,  # Set Gamma
            b"\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f",
        ),
        (0x11, None),
        (0x29, None),
    )
    _ENCODE_PIXEL = ">H"
    _ENCODE_POS = ">HH"
    _DECODE_PIXEL = ">BBB"

    def __init__(
        self,
        spi,
        dc,
        cs,
        rst=None,
        width=240,
        height=320,
        baudrate=16000000,
        polarity=0,
        phase=0,
        rotation=0,
    ):
        super().__init__(
            spi,
            dc,
            cs,
            rst=rst,
            width=width,
            height=height,
            baudrate=baudrate,
            polarity=polarity,
            phase=phase,
            rotation=rotation,
        )
        self._scroll = 0

    def scroll(self, dy=None):
        """Scroll the display by delta y"""
        if dy is None:
            return self._scroll
        self._scroll = (self._scroll + dy) % self.height
        self.write(0x37, struct.pack(">H", self._scroll))
        return None

display = ILI9341(spi, cs=cs_pin, dc=dc_pin, baudrate=64000000)

def color565(r, g, b):
    return (r & 0xF8) << 8 | (g & 0xFC) << 3 | b >> 3

display.fill(color565(0xff, 0x11, 0x22))
