from pydub.playback import _play_with_simpleaudio
import os
from multiprocessing import Queue, Event
from modules.helper.tone import Tone
from pydub import AudioSegment
from typing import List


class SynthesizerSampler:
    path = os.getcwd() + "/resources"
    __tones_number_start = 0
    __tone = Tone
    type = str

    def __init__(self, type: str, tone: Tone):
        self.type = type
        self.__tone = tone

    def get_tones(self):
        number = 0
        tones = []
        while number < 16:
            try:
                tones.append(self.__tone.load(self.type, number + self.__tones_number_start))
            except FileNotFoundError:
                tones.append(AudioSegment.silent())
            number += 1
        return tones

    def run(self, tone_queue: Queue, tones: List[AudioSegment], next_tones: Event):
        while True:
            if next_tones.is_set():
                tones = self.__get_next_tones()
                next_tones.clear()
            tone = tone_queue.get()
            _play_with_simpleaudio(tone)

    def __get_next_tones(self):
        self.__tones_number_start = self.__tones_number_start + 16
        try:
            self.__tone.load(self.type, self.__tones_number_start)
        except FileNotFoundError:
            self.__tones_number_start = 0

        return self.get_tones()


