from multiprocessing import Queue, Event
import smbus
from typing import List
from pydub import AudioSegment


class MCP23017:
    __bus = smbus.SMBus(1)

    DRM_1 = 0x20
    DRM_2 = 0x21

    SYN_PAD = 0x22
    SYN_REC = 0x23

    SAM_PAD = 0x24
    SAM_REC = 0x25

    IODIRA = 0x00
    IODIRB = 0x01
    OLATB = 0x15
    GPIOA = 0x12
    GPIOB = 0x13
    GPPUB = 0x0D
    GPPUA = 0x0C
    OLATA = 0x14

    def __init__(self):
        self.__load_drum_machine(self.DRM_1, self.DRM_2)

        self.__load_matrix(self.SYN_PAD)
        self.__load_rec(self.SYN_REC)

        self.__load_matrix(self.SAM_PAD)
        self.__load_rec(self.SAM_REC)
    
    def read(self, drum_machine_powers: List[bool], drum_machine_next_queue: Queue,
             synthesizer_tones: List[AudioSegment], synthesizer_tone_queue: Queue, synthesizer_records: List[bool], synthesizer_record_queue: Queue,
             synthesizer_next_tones: Event, synthesizer_powers: List[bool],
             sampler_tones: List[AudioSegment], sampler_tone_queue: Queue, sampler_records: List[bool], sampler_record_queue: Queue,
             sampler_next_tones: Event, sampler_powers: List[bool]):
        # only for test @TODO
        # time.sleep(3)
        # synthesizer_tone_queue.put_nowait(synthesizer_tones[0])
        # time.sleep(2)
        # synthesizer_records[0] = True
        #
        # synthesizer_tone_queue.put_nowait(synthesizer_tones[0])
        # time.sleep(0.3)
        # synthesizer_record_queue.put_nowait(synthesizer_tones[0])
        #
        # synthesizer_tone_queue.put_nowait(synthesizer_tones[1])
        # synthesizer_record_queue.put_nowait(synthesizer_tones[1])
        #
        # time.sleep(1)
        # synthesizer_tone_queue.put_nowait(synthesizer_tones[2])
        # synthesizer_record_queue.put_nowait(synthesizer_tones[2])
        # time.sleep(1)
        # synthesizer_records[0] = False
        # synthesizer_powers[0] = True
        # time.sleep(10)
        # synthesizer_powers[0] = False
        # time.sleep(2)
        # drum_machine_next_queue.put_nowait(["drum", 0])
        # time.sleep(2)
        # drum_machine_next_queue.put_nowait(["pattern", 4])

        while True:
            self.__read_drum_machine(self.DRM_1, self.DRM_2, drum_machine_powers, drum_machine_next_queue)
            self.__read_matrix(self.SYN_PAD, synthesizer_tones, synthesizer_tone_queue, synthesizer_records, synthesizer_record_queue)
            self.__read_rec(self.SYN_REC, synthesizer_next_tones, synthesizer_records, synthesizer_powers)

            self.__read_matrix(self.SAM_PAD, sampler_tones, sampler_tone_queue, sampler_records, sampler_record_queue)
            self.__read_rec(self.SAM_REC, sampler_next_tones, sampler_records, sampler_powers)
    
    def __load_drum_machine(self, device_id_next: hex, device_id_power: hex):
        # next bank a is drum, next bank b is pattern
        self.__bus.write_byte_data(device_id_next, self.IODIRA, 0b00111111)
        self.__bus.write_byte_data(device_id_next, self.GPPUA, 0b00111111)
        self.__bus.write_byte_data(device_id_next, self.IODIRB, 0b00111111)
        self.__bus.write_byte_data(device_id_next, self.GPPUB, 0b00111111)
    
        # power bank b is input, bank a is output for leds
        self.__bus.write_byte_data(device_id_power, self.IODIRB, 0b00111111)
        self.__bus.write_byte_data(device_id_power, self.GPPUB, 0b00111111)
        self.__bus.write_byte_data(device_id_power, self.IODIRA, 0b0)

    def __read_drum_machine(self, device_id_next: hex, device_id_power: hex, powers: List[bool], next_queue: Queue):
        power_inputs_raw = self.__bus.read_byte_data(device_id_power, self.GPIOB)
        self.__bus.write_byte_data(device_id_power, self.OLATA, power_inputs_raw)
        power_inputs = str(bin(power_inputs_raw))
        next_drum_inputs = str(bin(self.__bus.read_byte_data(device_id_next, self.GPIOA)))
        next_pattern_inputs = str(bin(self.__bus.read_byte_data(device_id_next, self.GPIOB)))
        i = 4
        while i < 10:
            powers[i - 4] = bool(power_inputs[i])
            i += 1
            if next_drum_inputs[i] == "1":
                next_queue.put_nowait(["drum", i-4])
            if next_pattern_inputs[i] == "1":
                next_queue.put_nowait(["pattern", i-4])

    def __load_rec(self, device_id: hex):
        # bank b is input for next and rec
        self.__bus.write_byte_data(device_id, self.IODIRB, 0b00001111)
        self.__bus.write_byte_data(device_id, self.GPPUB, 0b00001111)
        # bank a is on/off and led, pin 7, 6, 5 is for button
        self.__bus.write_byte_data(device_id, self.IODIRA, 0b00000111)
        self.__bus.write_byte_data(device_id, self.GPPUA, 0b00000111)
    
    def __read_rec(self, device_id: hex, next_tones: Event, records: List[bool], powers: List[bool]):
        # set all LEDs to off
        self.__bus.write_byte_data(device_id, self.OLATA, 0b0)
        next_and_rec_inputs = str(bin(self.__bus.read_byte_data(device_id, self.GPIOB)))
        if next_and_rec_inputs[9] == "1":
            next_tones.set()
        i = 6
        while i < 9:
            records[i-6] = bool(next_and_rec_inputs[i])
            i += 1
        on_off_inputs_raw = self.__bus.read_byte_data(device_id, self.GPIOA)
        on_off_inputs = str(bin(on_off_inputs_raw))
        i = 9
        while i > 6:
            powers[i-9] = bool(on_off_inputs[i])
            self.__bus.write_byte_data(device_id, self.OLATA, on_off_inputs_raw << 3)
            i -= 1

    def __load_matrix(self, device_id: hex):
        # define bank B as button interface with 4 inputs and 4 outputs, [0, 1, 2, 3] on the left side is input with active pull up
        self.__bus.write_byte_data(device_id, self.IODIRB, 0b00001111)
        self.__bus.write_byte_data(device_id, self.GPPUB, 0b00001111)
        # define bank A as LED interface with 8 outputs
        self.__bus.write_byte_data(device_id, self.IODIRA, 0b00000000)
        # set all button outputs pins to high
        self.__bus.write_byte_data(device_id, self.OLATB, 0b111100000)
    
    def __read_matrix(self, device_id: hex, tones: List[AudioSegment], tone_queue: Queue, records: List[bool], record_queue: Queue):
        # set all LEDs off
        self.__bus.write_byte_data(device_id, self.OLATA, 0b11111111)
        low_outputs = [0b11100000, 0b11010000, 0b10110000, 0b01110000]
        for output_number, low_output in enumerate(low_outputs):
            self.__bus.write_byte_data(device_id, self.OLATB, low_output)
            inputs = str(bin(self.__bus.read_byte_data(device_id, self.GPIOB)))
            input_read = 9
            while input_read > 5:
                if inputs[input_read] == "1":
                    # 9 = 0, 8 = 1, 7 = 2, 6 = 3
                    internal_operator = (input_read - 9) * -1
                    # set LED on same position
                    self.__bus.write_byte_data(device_id, self.OLATA, low_output + (1 << internal_operator))
                    tone = tones[output_number + (internal_operator * 4)]
                    tone_queue.put_nowait(tone)
                    if True in records:
                        record_queue.put_nowait(tone)
                input_read -= 1
