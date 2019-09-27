from pydub import AudioSegment
from multiprocessing import Queue
import time
from pydub.playback import _play_with_simpleaudio
from typing import List


class Recorders:
    number = 1

    def __init__(self, number):
        self.number = number

    def get_recordings(self):
        return [AudioSegment.silent()] * self.number

    def get_powers(self):
        return [0] * self.number

    def get_records(self):
        return [False] * self.number

    def play(self, filtered_recordings: List[AudioSegment], powers: List[bool], records: List[bool]):
        end_plays = [0] * len(filtered_recordings)
        while True:
            for i, recording in enumerate(filtered_recordings):
                if powers[i] and not records[i] and time.time() > end_plays[i]:
                    end_plays[i] = time.time() + recording.duration_seconds
                    _play_with_simpleaudio(recording)

    def record(self, recordings: List[AudioSegment], filtered_recordings: List[AudioSegment], records: List[bool], record_queue: Queue):
        start_times = [time.time()] * len(records)
        internal_record = [False] * len(records)
        while True:
            for i, record in enumerate(records):
                if record:
                    if not internal_record[i]:
                        internal_record[i] = True
                        start_times[i] = time.time()
                        recordings[i] = AudioSegment.silent()
                        filtered_recordings[i] = recordings[i]
                    if not record_queue.empty():
                        tone = record_queue.get()
                        duration = (time.time() - start_times[i]) * 1000
                        complete_duration = duration + len(tone)
                        pre_recording = recordings[i]
                        recordings[i] = AudioSegment.silent(duration=complete_duration)
                        recordings[i] = recordings[i].overlay(pre_recording)
                        silence = AudioSegment.silent(duration=duration)
                        recordings[i] = recordings[i].overlay(silence + tone)
                        filtered_recordings[i] = recordings[i]
                if not record and internal_record[i]:
                    recordings[i] = recordings[i][:(time.time() - start_times[i]) * 1000]
                    filtered_recordings[i] = recordings[i]
                    internal_record[i] = False
