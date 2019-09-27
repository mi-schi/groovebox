from pydub import AudioSegment
import os


class Tone:
    genre_name = ""
    __path = os.getcwd() + "/resources"

    def __init__(self, genre_name: str):
        self.genre_name = genre_name

    def load(self, type_name: str, number: int):
        path_to_file = self.__path + "/music/" + self.genre_name + "/" + type_name + "_" + str(number) + ".wav"
        if not os.path.isfile(path_to_file):
            raise FileNotFoundError
        return AudioSegment.from_wav(path_to_file)