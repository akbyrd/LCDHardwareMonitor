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

import adafruit_rgb_display.ili9341 as ili9341
display = ILI9341(spi, cs=cs_pin, dc=dc_pin, baudrate=64000000)

from adafruit_rgb_display import color565
display.fill(color565(0xff, 0xff, 0x22))
