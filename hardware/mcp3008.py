import spidev
import RPi.GPIO as GPIO
from typing import List


class MCP3008:
    raw_default = 512

    DRM_1 = 0
    DRM_2 = 1
    SYN = 3
    SAM = 4

    MPLX_S0 = 23
    MPLX_S1 = 24
    MPLX_S2 = 25

    __spi = spidev.SpiDev()

    def __init__(self):
        self.__spi.open(0, 0)
        self.__spi.max_speed_hz = 1000000

        GPIO.setmode(GPIO.BCM)
        GPIO.setup(self.MPLX_S0, GPIO.OUT)
        GPIO.setup(self.MPLX_S1, GPIO.OUT)
        GPIO.setup(self.MPLX_S2, GPIO.OUT)

    def read(self, drum_machine_volumes_raw: List[int], drum_machine_filter_raw: List[int],
             synthesizer_volumes_raw: List[int], synthesizer_filter_raw: List[int],
             sampler_volumes_raw: List[int], sampler_filter_raw: List[int]):
        while True:
            for i in range(7):
                drum_machine_volumes_raw[i] = self.__read_mcp3008(self.DRM_1, i)
                drum_machine_filter_raw[i] = self.__read_mcp3008(self.DRM_2, i)
            for i in range(3):
                synthesizer_volumes_raw[i] = self.__read_mcp3008(self.SYN, i)
                sampler_volumes_raw[i] = self.__read_mcp3008(self.SAM, i)
            for i in range(3, 6):
                synthesizer_filter_raw[i] = self.__read_mcp3008(self.SYN, i)
                sampler_filter_raw[i] = self.__read_mcp3008(self.SAM, i)

    def __read_mcp3008(self, device_id: int, channel: int):
        GPIO.output(self.MPLX_S2, (device_id >> 2) & 1)
        GPIO.output(self.MPLX_S1, (device_id >> 1) & 1)
        GPIO.output(self.MPLX_S0, device_id & 1)
        adc = self.__spi.xfer2([1, (8 + channel) << 4, 0])
        return ((adc[1] & 3) << 8) + adc[2]