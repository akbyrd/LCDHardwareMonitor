class Pin:
    """A basic Pin class for use with FT232H."""

    IN = 0
    OUT = 1
    LOW = 0
    HIGH = 1

    ft232h_gpio = None

    def __init__(self, pin_id=None):
        # setup GPIO controller if not done yet use one provided by I2C as default
        if not Pin.ft232h_gpio:
            from pyftdi.i2c import I2cController

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

# create pin instances (D0 = 0, C0 = 8)
C0 = Pin(8)
C1 = Pin(9)
SCLK = Pin()
MOSI = Pin()
MISO = Pin()

class Enum:
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
# This is the size of the buffer to be used for fill operations, in 16-bit units.
_BUFFER_SIZE = 320 * 240

class SPIDevice:
    def __init__(
        self,
        spi,
        chip_select=None,
        *,
        baudrate=100000,
        polarity=0,
        phase=0,
        extra_clocks=0
    ):
        self.spi = spi
        self.baudrate = baudrate
        self.polarity = polarity
        self.phase = phase
        self.extra_clocks = extra_clocks
        self.chip_select = chip_select
        if self.chip_select:
            self.chip_select.switch_to_output(value=True)

    def __enter__(self):
        while not self.spi.try_lock():
            pass
        self.spi.configure(
            baudrate=self.baudrate, polarity=self.polarity, phase=self.phase
        )
        if self.chip_select:
            self.chip_select.value = False
        return self.spi

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.chip_select:
            self.chip_select.value = True
        if self.extra_clocks > 0:
            buf = bytearray(1)
            buf[0] = 0xFF
            clocks = self.extra_clocks // 8
            if self.extra_clocks % 8 != 0:
                clocks += 1
            for _ in range(clocks):
                self.spi.write(buf)
        self.spi.unlock()
        return False

class Display:
    _PAGE_SET = 0x2B
    _COLUMN_SET = 0x2A
    _RAM_WRITE = 0x2C
    _ENCODE_PIXEL = ">H"
    _ENCODE_POS = ">HH"
    _INIT = (
        (0xEF, b"\x03\x80\x02"),
        (0xCF, b"\x00\xc1\x30"),
        (0xED, b"\x64\x03\x12\x81"),
        (0xE8, b"\x85\x00\x78"),
        (0xCB, b"\x39\x2c\x00\x34\x02"),
        (0xF7, b"\x20"),
        (0xEA, b"\x00\x00"),
        (0xC0, b"\x23"),          # Power Control 1, VRH[5:0]
        (0xC1, b"\x10"),          # Power Control 2, SAP[2:0], BT[3:0]
        (0xC5, b"\x3e\x28"),      # VCM Control 1
        (0xC7, b"\x86"),          # VCM Control 2
        (0x36, b"\x48"),          # Memory Access Control
        (0x3A, b"\x55"),          # Pixel Format
        (0xB1, b"\x00\x18"),      # FRMCTR1
        (0xB6, b"\x08\x82\x27"),  # Display Function Control
        (0xF2, b"\x00"),          # 3Gamma Function Disable
        (0x26, b"\x01"),          # Gamma Curve Selected
        (0xE0, b"\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00"), # Set Gamma
        (0xE1, b"\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f"), # Set Gamma
        (0x11, None),
        (0x29, None),
    )

    def __init__( self, spi_device, dc, cs, width, height):
        self._scroll = 0
        self.spi_device = spi_device
        self.dc_pin = dc
        self.dc_pin.switch_to_output(value=0)
        self.width = width
        self.height = height
        for command, data in self._INIT:
            self.write(command, data)

    def _block(self, x0, y0, x1, y1, data):
        self.write(self._COLUMN_SET, self._encode_pos(x0, x1))
        self.write(self._PAGE_SET,   self._encode_pos(y0, y1))
        self.write(self._RAM_WRITE, data)
        return None

    def _encode_pos(self, x, y):
        """Encode a postion into bytes."""
        return struct.pack(self._ENCODE_POS, x, y)

    def _encode_pixel(self, color):
        """Encode a pixel color into bytes."""
        return struct.pack(self._ENCODE_PIXEL, color)

    def fill_rectangle(self, x, y, width, height, color):
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

spi_device = SPIDevice(spi, cs_pin, baudrate=64000000, polarity=0, phase=0)
display = Display(spi_device, dc_pin, cs_pin, 240, 320)

def color565(r, g, b):
    return (r & 0xF8) << 8 | (g & 0xFC) << 3 | b >> 3

display.fill_rectangle(0, 0, 240, 320, color565(0x00, 0xff, 0xff))
