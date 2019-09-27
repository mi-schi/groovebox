from pydub.playback import _play_with_simpleaudio
from pydub import AudioSegment
import time
import os
from modules.helper.tone import Tone
from typing import List
from multiprocessing import Queue


class DrumMachine:
    drum_types = ["kick", "snare", "clap", "tom", "hihat", "hat"]
    drum_types_length = int

    __beat_numbers = []
    __beat_number_default = 1
    __pattern_numbers = []
    __pattern_number_default = 1
    __path = os.getcwd() + "/resources"

    def __init__(self):
        self.drum_types_length = len(self.drum_types)
        self.__beat_numbers = [self.__beat_number_default] * self.drum_types_length
        self.__pattern_numbers = [self.__pattern_number_default] * self.drum_types_length

    def get_drums(self, tone: Tone):
        drums = []
        for i, drum_type in enumerate(self.drum_types):
            drums.append(tone.load(drum_type, self.__beat_numbers[i]))
        return drums

    def get_patterns(self):
        patterns = []
        for i, drum_type in enumerate(self.drum_types):
            patterns.append(self.__load_pattern(drum_type, self.__pattern_numbers[i]))
        return patterns

    def get_powers(self):
        return [1] * 6

    def play(self, bpm, tact_number, tact_value, filtered_drums: List[AudioSegment], powers: List[bool], patterns: List[str]):
        sleep_time = 60 / bpm / tact_number
        beats = tact_number * tact_value
        while True:
            beat = 0
            while beat < beats:
                for i, filtered_drum in enumerate(filtered_drums):
                    if not powers[i]:
                        continue
                    if patterns[i][beat] == "0":
                        continue
                    _play_with_simpleaudio(filtered_drum)
                time.sleep(sleep_time)
                beat += 1

    def next(self, next_queue: Queue, filtered_drums: List[AudioSegment], patterns: List[str], tone: Tone):
        while True:
            next_one = next_queue.get()
            i = next_one[1]
            if next_one[0] == "pattern":
                self.__pattern_numbers[i] += 1
                try:
                    patterns[i] = self.__load_pattern(self.drum_types[i], self.__pattern_numbers[i])
                except FileNotFoundError:
                    self.__pattern_numbers[i] = self.__pattern_number_default
                    patterns[i] = self.__load_pattern(self.drum_types[i], self.__pattern_numbers[i])
            if next_one[0] == "drum":
                self.__beat_numbers[i] += 1
                try:
                    filtered_drums[i] = tone.load(self.drum_types[i], self.__beat_numbers[i])
                except FileNotFoundError:
                    self.__beat_numbers[i] = self.__beat_number_default
                    filtered_drums[i] = tone.load(self.drum_types[i], self.__beat_numbers[i])

    def __load_pattern(self, drum_type: str, number: int):
        path_to_file = self.__path + "/pattern/" + drum_type + "_" + str(number) + ".txt"
        if not os.path.isfile(path_to_file):
            raise FileNotFoundError
        return open(path_to_file, "r").read()
