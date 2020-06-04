from pydub import AudioSegment
from pydub.playback import _play_with_simpleaudio
from pydub.effects import *
from pydub.generators import *
from time import sleep
from pynput import keyboard
import os

bpm = 125
db = 0
lowPass = 2000
highPass = 1000
p = 5

kick = AudioSegment.from_wav("resources/music/minimal/Kick 3.wav")
clap = AudioSegment.from_wav("resources/music/minimal/Clap 5.wav")
hihat = AudioSegment.from_wav("resources/music/minimal/HiHat 5.wav")
openhat = AudioSegment.from_wav("resources/music/minimal/hat_1.wav")
perc = AudioSegment.from_wav("resources/music/minimal/Perc 5.wav")
vocal = AudioSegment.from_wav("resources/music/minimal/VoxCut 1.wav")

sounds = {"k": kick, "c": clap, "h": hihat, "o": openhat, "p": perc}
manipulatedSounds = sounds.copy()
currentSound = "k"

def on_press(key):
    global sounds, currentSound, bpm, db, lowPass, highPass, p, bass

    if key == keyboard.Key.left:
        bpm = bpm - 1
    if key == keyboard.Key.right:
        bpm = bpm + 1

    if hasattr(key, "char") and key.char in sounds.keys():
        currentSound = key.char

    if key == keyboard.Key.up:
        db = db + 1
    if key == keyboard.Key.down:
        db = db - 1

    if hasattr(key, "char"):
        if key.char == "v":
            _play_with_simpleaudio(vocal)

        if key.char == "y" and p > 1:
            p = p - 1
            bass = AudioSegment.from_wav("music/Bassline " + str(p) + ".wav")
        if key.char == "x":
            p = p + 1
            bass = AudioSegment.from_wav("music/Bassline " + str(p) + ".wav")
        if key.char == "1" and lowPass > 100:
            lowPass = lowPass - 100
        if key.char == "3":
            lowPass = lowPass + 100
        if key.char == "7" and highPass > 100:
            highPass = highPass - 100
        if key.char == "9":
            highPass = highPass + 100

    print(db)
    print(lowPass)
    print(highPass)
    print(p)
    manipulatedSounds[currentSound] = low_pass_filter(sounds[currentSound], lowPass) + db


listener = keyboard.Listener(on_release=on_press)
listener.start()

while True:
    i = 1
    while i <= 16:
        if i % 4 == 1:
            _play_with_simpleaudio(manipulatedSounds["k"])

        if i % 8 == 1:
            _play_with_simpleaudio(manipulatedSounds["c"])

        if i % 4 == 3:
            _play_with_simpleaudio(manipulatedSounds["h"])

        if i % 8 == 3:
            _play_with_simpleaudio(manipulatedSounds["o"])

        if i == 1 or i == 5 or i == 10 or i == 13:
            _play_with_simpleaudio(manipulatedSounds["p"])

        sleep(60/bpm/4)
        i += 1
