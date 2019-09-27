#!/usr/bin/env python

from multiprocessing import Process, Manager
from hardware.mcp23017 import MCP23017
from hardware.mcp3008 import MCP3008
from modules.drum_machine import DrumMachine

from modules.helper import modifier
from modules.helper.recorders import Recorders
from modules.helper.tone import Tone
from modules.synthesizer_sampler import SynthesizerSampler


bpm = 140
tact_number = 4
tact_value = 4
genre_name = "minimal"

tone = Tone(genre_name)
drum_machine = DrumMachine()
synthesizer = SynthesizerSampler("synth", tone)
sampler = SynthesizerSampler("sam", tone)
synthesizer_recorders = Recorders(3)
sampler_recorders = Recorders(3)

mcp23017 = MCP23017()
mcp3008 = MCP3008()

manager = Manager()
drums = manager.list(drum_machine.get_drums(tone))
filtered_drums = manager.list(drums[:])
patterns = manager.list(drum_machine.get_patterns())
drum_machine_powers = manager.list(drum_machine.get_powers())
drum_machine_next_queue = manager.Queue()


synthesizer_tone_queue = manager.Queue()
synthesizer_tones = manager.list(synthesizer.get_tones())
synthesizer_next_tones = manager.Event()

synthesizer_recordings = manager.list(synthesizer_recorders.get_recordings())
synthesizer_filtered_recordings = manager.list(synthesizer_recordings[:])
synthesizer_powers = manager.list(synthesizer_recorders.get_powers())
synthesizer_records = manager.list(synthesizer_recorders.get_records())
synthesizer_record_queue = manager.Queue()


sampler_tone_queue = manager.Queue()
sampler_tones = manager.list(sampler.get_tones())
sampler_next_tones = manager.Event()

sampler_recordings = manager.list(sampler_recorders.get_recordings())
sampler_filtered_recordings = manager.list(sampler_recordings[:])
sampler_powers = manager.list(sampler_recorders.get_powers())
sampler_records = manager.list(sampler_recorders.get_records())
sampler_record_queue = manager.Queue()

drum_machine_volumes_raw = manager.list([mcp3008.raw_default] * drum_machine.drum_types_length)
drum_machine_filter_raw = manager.list([mcp3008.raw_default] * drum_machine.drum_types_length)
synthesizer_volumes_raw = manager.list([mcp3008.raw_default] * synthesizer_recorders.number)
synthesizer_filter_raw = manager.list([mcp3008.raw_default] * synthesizer_recorders.number)
sampler_volumes_raw = manager.list([mcp3008.raw_default] * sampler_recorders.number)
sampler_filter_raw = manager.list([mcp3008.raw_default] * sampler_recorders.number)

process_drum_machine_play = Process(target=drum_machine.play, args=(bpm, tact_number, tact_value, filtered_drums, drum_machine_powers, patterns))
process_drum_machine_next = Process(target=drum_machine.next, args=(drum_machine_next_queue, filtered_drums, patterns, tone))
process_drum_machine_play.start()
process_drum_machine_next.start()

process_synthesizer = Process(target=synthesizer.run, args=(synthesizer_tone_queue, synthesizer_tones, synthesizer_next_tones))
process_synthesizer_recorders_play = Process(target=synthesizer_recorders.play, args=(synthesizer_filtered_recordings, synthesizer_powers, synthesizer_records))
process_synthesizer_recorders_record = Process(target=synthesizer_recorders.record, args=(synthesizer_recordings, synthesizer_filtered_recordings, synthesizer_records, synthesizer_record_queue))
process_synthesizer.start()
process_synthesizer_recorders_play.start()
process_synthesizer_recorders_record.start()

process_sampler = Process(target=sampler.run, args=(sampler_tone_queue, sampler_tones, sampler_next_tones))
process_sampler_recorders_play = Process(target=sampler_recorders.play, args=(sampler_filtered_recordings, sampler_powers, sampler_records))
process_sampler_recorders_record = Process(target=sampler_recorders.record, args=(sampler_recordings, sampler_filtered_recordings, sampler_records, sampler_record_queue))
process_sampler.start()
process_sampler_recorders_play.start()
process_sampler_recorders_record.start()

process_mcp23017 = Process(target=mcp23017.read, args=(drum_machine_powers, drum_machine_next_queue,
                                                       synthesizer_tones, synthesizer_tone_queue, synthesizer_records, synthesizer_record_queue,
                                                       synthesizer_next_tones, synthesizer_powers,
                                                       sampler_tones, sampler_tone_queue, sampler_records, sampler_record_queue,
                                                       sampler_next_tones, sampler_powers))
process_mcp3008 = Process(target=mcp3008.read, args=(drum_machine_volumes_raw, drum_machine_filter_raw,
                                                     synthesizer_volumes_raw, synthesizer_filter_raw,
                                                     sampler_volumes_raw, sampler_filter_raw))
process_mcp23017.start()
process_mcp3008.start()

process_drum_machine_volumes = Process(target=modifier.add_volumes, args=(drum_machine_volumes_raw, filtered_drums, drums))
process_drum_machine_filter = Process(target=modifier.add_filter, args=(drum_machine_filter_raw, filtered_drums))
process_drum_machine_volumes.start()
process_drum_machine_filter.start()

process_synthesizer_volumes = Process(target=modifier.add_volumes, args=(synthesizer_volumes_raw, synthesizer_filtered_recordings, synthesizer_recordings))
process_synthesizer_filter = Process(target=modifier.add_filter, args=(synthesizer_filter_raw, synthesizer_filtered_recordings))
process_synthesizer_volumes.start()
process_synthesizer_filter.start()

process_sampler_volumes = Process(target=modifier.add_volumes, args=(sampler_volumes_raw, sampler_filtered_recordings, sampler_recordings))
process_sampler_filter = Process(target=modifier.add_filter, args=(sampler_filter_raw, sampler_filtered_recordings))
process_sampler_volumes.start()
process_sampler_filter.start()

process_drum_machine_play.join()
process_drum_machine_next.join()

process_synthesizer.join()
process_synthesizer_recorders_play.join()
process_synthesizer_recorders_record.join()

process_sampler.join()
process_sampler_recorders_play.join()
process_sampler_recorders_record.join()

process_mcp23017.join()
process_mcp3008.join()

process_drum_machine_volumes.join()
process_drum_machine_filter.join()

process_synthesizer_volumes.join()
process_synthesizer_filter.join()

process_sampler_volumes.join()
process_sampler_filter.join()