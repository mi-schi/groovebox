from pydub.effects import *
from interfaces.potentiometer import *
from typing import List
from pydub import AudioSegment
import time


def add_volumes(bpm, tact_number, volumes_raw: List[int], filtered_tones: List[AudioSegment], tones: List[AudioSegment]):
    old_volumes_raw = volumes_raw[:]
    while True:
        sleep_time = 60 / bpm / tact_number
        time.sleep(sleep_time)
        for i, volume_raw in enumerate(volumes_raw):
            if volume_raw != old_volumes_raw[i]:
                filtered_tones[i] = tones[i] + calculate_volume(volume_raw)
                old_volumes_raw[i] = volume_raw


def add_filter(bpm, tact_number, filters_raw: List[int], filtered_tones: List[AudioSegment]):
    old_filters_raw = filters_raw[:]
    while True:
        sleep_time = 60 / bpm / tact_number
        time.sleep(sleep_time)
        for i, filter_raw in enumerate(filters_raw):
            if filter_raw != old_filters_raw[i]:
                if is_high_pass_filter(filter_raw):
                    filtered_tones[i] = high_pass_filter(filtered_tones[i], calculate_high_pass_filter(filter_raw))
                else:
                    filtered_tones[i] = low_pass_filter(filtered_tones[i], calculate_low_pass_filter(filter_raw))
                old_filters_raw[i] = filter_raw
