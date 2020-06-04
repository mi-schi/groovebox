from modules.helper.tone import Tone
from modules.synthesizer_sampler import SynthesizerSampler
from matrix_keypad import MCP230xx


kp = MCP230xx.keypad(address = 0x21, num_gpios = 8, columnCount = 4)
kp.getKey()

tone = Tone("minimal")
synthesizer = SynthesizerSampler("synth")
synthesizer.load(tone)
