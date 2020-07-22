#include <stdio.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <mcp3004.h>
#include <mcp23017.h>

#include <alsa/asoundlib.h>
#include <stdbool.h>

#include <sndfile.h>
#include <pthread.h>
#include <errno.h>

#include <math.h>

#define DRUM_CHANNELS 6
#define SYNTHS 15

#define EQUALIZER_SIZE 10
#define ALL_PATTERN_SIZE 19

#define FILENAME_PREFIX "/media/ramdisk"

// for drummachine
#define MCP3008_0 100
#define MCP3008_1 200
#define MCP23017_0 300
#define MCP23017_1 400

// for synthesizer
#define MCP23017_2 500
#define MCP23017_3 600

#define ALSA_INDEX 0

char *equalizer_names[EQUALIZER_SIZE] = {
    "00. 31 Hz",
    "01. 63 Hz",
    "02. 125 Hz",
    "03. 250 Hz",
    "04. 500 Hz",
    "05. 1 kHz",
    "06. 2 kHz",
    "07. 4 kHz",
    "08. 8 kHz",
    "09. 16 kHz"
};

char *all_pattern[ALL_PATTERN_SIZE] = {
    // house
    "1000100010001000",
    "1000000010000000",
    "0010001000100010",
    "0000100000001000",
    "0010010000100100",
    "0010000000100000",
    "0000100000001000",
    "1111111111111111",
    // dirty house
    "0010000000100010",
    "0000000000100001",
    "0010100010101000",
    "1010100010101000",
    // chigago house
    "0000100000001001",
    "0100100100101100",
    "1000100010001010",
    // deep house
    "0010001000110010",
    "0001000000010000",
    "0010000100100000",
    "0100000001000000"
};

char *pattern[DRUM_CHANNELS];

int power_values[DRUM_CHANNELS] = {0, 0, 0, 0, 0, 0};
int power_led_values[DRUM_CHANNELS] = {0, 0, 0, 0, 0, 0};

int pattern_led_pins[DRUM_CHANNELS] = {MCP23017_1 + 6, MCP23017_1 + 7, MCP23017_1 + 14, MCP23017_1 + 15, MCP23017_0 + 6, MCP23017_0 + 7};
int pattern_led_values[DRUM_CHANNELS] = {0, 0, 0, 0, 0, 0};

int bpm = 120;

char *drum_pcm_devices[DRUM_CHANNELS] = {"ch0", "ch1", "ch2", "ch3", "ch4", "ch5"};
char *drum_folders[DRUM_CHANNELS] = {"kick", "clap", "snare", "tom", "hat", "hihat"};
char *drum_equalizer_cards[DRUM_CHANNELS] = {"ch0_equal", "ch1_equal", "ch2_equal", "ch3_equal", "ch4_equal", "ch5_equal"};

int next_drums[DRUM_CHANNELS];

pthread_cond_t cond_drums[DRUM_CHANNELS];
pthread_mutex_t mutex_drums[DRUM_CHANNELS];

char *synth_pcm_device = "synth";
char *synth_folder = "synth";
char *synth_equalizer_card = "synth_equal";

int next_synths[SYNTHS];

pthread_cond_t cond_synth[SYNTHS];
pthread_mutex_t mutex_synth[SYNTHS];

char *get_filepath(char *folder, int sample_id) {
    int path_size = strlen(FILENAME_PREFIX) + 1 + strlen(folder) + 1 + 15;
    char *path = malloc(path_size);
    snprintf(path, path_size, "%s/%s/sample_%03d.wav", FILENAME_PREFIX, folder, sample_id);
    printf("DEBUG: file path %s\n", path);

    return path;
}

void player(char *folder, int sample_id, char *pcm_device, pthread_cond_t *cond, pthread_mutex_t *mutex, int *next_sample, int next_adder) {
    int first_sample_id = sample_id;
    while (1) {
        printf("INFO: open sample %s/%d with pcm device %s\n", folder, sample_id, pcm_device);
        char *path = get_filepath(folder, sample_id);

        SF_INFO sfinfo;
        SNDFILE *sndfile;
        sndfile = sf_open(path, SFM_READ, &sfinfo);

        snd_pcm_t *pcm;
        snd_pcm_open(&pcm, pcm_device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);

        snd_pcm_hw_params_t *params;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(pcm, params);
        snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_channels(pcm, params, sfinfo.channels);
        snd_pcm_hw_params_set_rate(pcm, params, sfinfo.samplerate, 0);
        snd_pcm_hw_params(pcm, params);

        snd_pcm_uframes_t frames;
        int dir;
        short* buffer;
        snd_pcm_hw_params_get_period_size(params, &frames, &dir);
        buffer = malloc(frames * sfinfo.channels * sizeof(short));

        int readcount;
        *next_sample = 0;

        while (1) {
            pthread_cond_wait(cond, mutex);

            if (*next_sample == 1) {
                printf("INFO: load new sample, old one is %s-%d\n", folder, sample_id);

                break;
            }

            printf("DEBUG: -> play sample %s-%d\n", folder, sample_id);
            snd_pcm_prepare(pcm);

            while ((readcount = sf_readf_short(sndfile, buffer, frames)) > 0) {
                snd_pcm_writei(pcm, buffer, readcount);
            }

            sf_seek(sndfile, 0, SEEK_SET);
        }

        printf("INFO: close sndfile + pcm and free buffer + path for old sample %s-%d\n", folder, sample_id);
        sf_close(sndfile);
        snd_pcm_drain(pcm);
        snd_pcm_close(pcm);
        free(buffer);
        free(path);

        sample_id = sample_id + next_adder;
        printf("INFO: try to set new sample %s-%d\n", folder, sample_id);

        if (access(get_filepath(folder, sample_id), F_OK) == -1) {
            printf("INFO: new sample %s-%d is not available, set it to first sample %s-%d\n", folder, sample_id, folder, first_sample_id);
            sample_id = first_sample_id;
        }
    }
}

void synthesizer(void* id) {
    int synth_id = (int) id;
    printf("INFO: start thread for synth %d\n", synth_id);

    player(synth_folder, synth_id, synth_pcm_device, &cond_synth[synth_id], &mutex_synth[synth_id], &next_synths[synth_id], SYNTHS);
}

void drummer(void* id) {
    int channel_id = (int) id;
    printf("INFO: start thread for drum channel %d\n", channel_id);

    player(drum_folders[channel_id], 0, drum_pcm_devices[channel_id], &cond_drums[channel_id], &mutex_drums[channel_id], &next_drums[channel_id], 1);
}

void next_synth() {
    for (int synth_id = 0; synth_id < SYNTHS; synth_id++) {
        next_synths[synth_id] = 1;
        pthread_cond_signal(&cond_synth[synth_id]);
    }
}

void next_drum(int channel_id) {
    next_drums[channel_id] = 1;
    pthread_cond_signal(&cond_drums[channel_id]);
}

void next_pattern(int i) {
    pattern[i] = all_pattern[rand() % ALL_PATTERN_SIZE];
}

long map(long poti_value, long in_min, long in_max, long out_min, long out_max) {
    return (poti_value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

snd_mixer_t *open_mixer(char *card) {
    snd_mixer_t *handle;

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    return handle;
}

snd_mixer_elem_t *find_selem(snd_mixer_t *handle, int alsa_index, char *selem_name) {
    snd_mixer_selem_id_t *sid;

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, alsa_index);
    snd_mixer_selem_id_set_name(sid, selem_name);

    return snd_mixer_find_selem(handle, sid);
}

void close_mixer(snd_mixer_t *handle) {
    snd_mixer_close(handle);
}

void set_volume(char *card, int alsa_index, char *selem_name, long volume) {
    snd_mixer_t *handle;

    handle = open_mixer(card);
    snd_mixer_elem_t* elem = find_selem(handle, alsa_index, selem_name);

    long min, max;
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(handle);
}

void set_equalizer(long poti_value, char * card) {
    long factor = map(poti_value, 0, 1023, 50, -50);
    long adder = 66;

    if (poti_value < 512) {
        adder = map(poti_value, 0, 511, 50, 66);
    } else {
        adder = map(poti_value, 512, 1023, 66, 50);
    }

    for (int channel_id = 0; channel_id < EQUALIZER_SIZE; channel_id++) {
        long equalizer_value = factor * cos(channel_id / M_PI) + adder;
        printf("INFO: set volume to %ld for equalizer channel %s for %s\n", equalizer_value, equalizer_names[channel_id], card);
        set_volume(card, ALSA_INDEX, equalizer_names[channel_id], equalizer_value);
    }
}

void load_pattern() {
    srand(time(NULL));

    for (int i = 0; i < DRUM_CHANNELS; i++) {
        pattern[i] = all_pattern[i];
    }
}

bool is_poti_changed(long poti_value, long old_poti_value) {
    long poti_range = 10;

    if (poti_value + poti_range < old_poti_value || poti_value - poti_range > old_poti_value) {
        return true;
    }

    return false;
}

bool is_button_debounced(struct timeval last_push) {
    int debounce_delay = 250 * 1000;
    struct timeval read_time;
    gettimeofday(&read_time, NULL);

    if (last_push.tv_sec * 1000000 + last_push.tv_usec + debounce_delay < read_time.tv_sec * 1000000 + read_time.tv_usec) {
        return true;
    }

    return false;
}

void hardware() {
    long old_poti_bpm_value = 0;

    long old_poti_volume_values[DRUM_CHANNELS] = {0};
    long old_poti_equalizer_values[DRUM_CHANNELS] = {0};

    int old_power_led_values[DRUM_CHANNELS] = {-1};
    int old_pattern_led_values[DRUM_CHANNELS] = {-1};

    long old_poti_synth_volume_value = 0;
    long old_poti_synth_equalizer_value = 0;

    struct timeval last_next_pattern_push[DRUM_CHANNELS] = {0},
                   last_next_sample_push[DRUM_CHANNELS] = {0},
                   last_synth_push[SYNTHS] = {0},
                   last_next_synth_push = {0};

    while (1) {
        // set bpm
        long poti_bpm_value = analogRead(MCP3008_0 + 7);

        if (is_poti_changed(poti_bpm_value, old_poti_bpm_value)) {
            bpm = (int) map(poti_bpm_value, 0, 1023, 90, 160);
            printf("EVENT: set bpm to %ld (raw %ld, further %ld)\n", bpm, poti_bpm_value, old_poti_bpm_value);
            old_poti_bpm_value = poti_bpm_value;
        }

        for (int channel_id = 0; channel_id < DRUM_CHANNELS; channel_id++) {
            // set volumes
            long poti_volume_value = analogRead(MCP3008_0 + channel_id);

            if (is_poti_changed(poti_volume_value, old_poti_volume_values[channel_id])) {
                long volume = map(poti_volume_value, 0, 1023, 0, 100);
                printf("EVENT: set volume to %ld (raw %ld, further %ld) for %s\n", volume, poti_volume_value, old_poti_volume_values[channel_id], drum_pcm_devices[channel_id]);
                set_volume("default", ALSA_INDEX, drum_pcm_devices[channel_id], volume);
                old_poti_volume_values[channel_id] = poti_volume_value;
            }

            // set equalizers
            long poti_equalizer_value = analogRead(MCP3008_1 + channel_id);

            if (is_poti_changed(poti_equalizer_value, old_poti_equalizer_values[channel_id])) {
                printf("EVENT: set equalizer with poti value %ld, further %ld for %s\n", poti_equalizer_value, old_poti_equalizer_values[channel_id], drum_equalizer_cards[channel_id]);
                set_equalizer(poti_equalizer_value, drum_equalizer_cards[channel_id]);
                old_poti_equalizer_values[channel_id] = poti_equalizer_value;
            }

            // read pattern buttons on bank a
            if (digitalRead(MCP23017_1 + channel_id) == 0 && is_button_debounced(last_next_pattern_push[channel_id])) {
                printf("EVENT: set next pattern for channel %d\n", channel_id);
                gettimeofday(&last_next_pattern_push[channel_id], NULL);
                pthread_mutex_lock(&mutex_drums[channel_id]);
                next_pattern(channel_id);
                pthread_mutex_unlock(&mutex_drums[channel_id]);
            }

            // read sample buttons on bank b
            if (digitalRead(MCP23017_1 + channel_id + 8) == 0 && is_button_debounced(last_next_sample_push[channel_id])) {
                printf("EVENT: set next sample for channel %d\n", channel_id);
                gettimeofday(&last_next_sample_push[channel_id], NULL);
                next_drum(channel_id);
            }

            // set powers, 0 = on, 1 = off
            int power = digitalRead(MCP23017_0 + channel_id);

            if (power == 0 && power_values[channel_id] == 0) {
                power_values[channel_id] = 1;
            }

            if (power == 1 && power_values[channel_id] == 1) {
                power_values[channel_id] = 0;
            }

            // switch leds
            if (power_led_values[channel_id] != old_power_led_values[channel_id]) {
                digitalWrite(MCP23017_0 + 8 + channel_id, power_led_values[channel_id]);
                old_power_led_values[channel_id] = power_led_values[channel_id];
            }

            if (pattern_led_values[channel_id] != old_pattern_led_values[channel_id]) {
                digitalWrite(pattern_led_pins[channel_id], pattern_led_values[channel_id]);
                old_pattern_led_values[channel_id] = pattern_led_values[channel_id];
            }
        }

        // set synth volume
        long poti_synth_volume_value = analogRead(MCP3008_1 + 6);

        if (is_poti_changed(poti_synth_volume_value, old_poti_synth_volume_value)) {
            long volume = map(poti_synth_volume_value, 0, 1023, 0, 100);
            printf("EVENT: set volume to %ld (raw %ld, further %ld) for %s\n", volume, poti_synth_volume_value, old_poti_synth_volume_value, synth_pcm_device);
            set_volume("default", ALSA_INDEX, synth_pcm_device, volume);
            old_poti_synth_volume_value = poti_synth_volume_value;
        }

        // set synth equalizer
        long poti_synth_equalizer_value = analogRead(MCP3008_1 + 7);

        if (is_poti_changed(poti_synth_equalizer_value, old_poti_synth_equalizer_value)) {
            printf("EVENT: set equalizer with poti value %ld, further %ld for %s\n", poti_synth_equalizer_value, old_poti_synth_equalizer_value, synth_equalizer_card);
            set_equalizer(poti_synth_equalizer_value, synth_equalizer_card);
            old_poti_synth_equalizer_value = poti_synth_equalizer_value;
        }

        for (int synth_id = 0; synth_id < SYNTHS; synth_id++) {
            // play synthesizer
            if (digitalRead(MCP23017_2 + synth_id) == 0) {
                if (is_button_debounced(last_synth_push[synth_id])) {
                    printf("EVENT: send information to play synth %d ->\n", synth_id);
                    gettimeofday(&last_synth_push[synth_id], NULL);
                    pthread_cond_signal(&cond_synth[synth_id]);
                    digitalWrite(MCP23017_3 + synth_id, 1);
                }
            } else {
                digitalWrite(MCP23017_3 + synth_id, 0);
            }
        }

        // set next synths
        if (digitalRead(MCP23017_2 + 15) == 0 && is_button_debounced(last_next_synth_push)) {
            printf("EVENT: set next synths\n");
            gettimeofday(&last_next_synth_push, NULL);
            next_synth();
        }
    }
}

int main(int argc, char **argv) {
    printf("INFO: initialize hardware\n");
    wiringPiSetup();

    mcp23017Setup(MCP23017_0, 0x20);
    mcp23017Setup(MCP23017_1, 0x21);
    mcp23017Setup(MCP23017_2, 0x22);
    mcp23017Setup(MCP23017_3, 0x23);

    mcp3004Setup(MCP3008_0, 0);
    mcp3004Setup(MCP3008_1, 1);

    printf("INFO: set mcp23017 0/1 bank a for powers, next pattern and mixed pattern-leds\n");
    int i;
    for (i = 0; i < DRUM_CHANNELS; i++) {
        pinMode(MCP23017_0 + i, INPUT);
        pullUpDnControl(MCP23017_0 + i, PUD_UP);

        pinMode(MCP23017_1 + i, INPUT);
        pullUpDnControl(MCP23017_1 + i, PUD_UP);

        pinMode(pattern_led_pins[i], OUTPUT);
    }

    printf("INFO: set mcp23017 0/1 bank b for power-leds and next samples\n");
    for (i = 8; i < 8 + DRUM_CHANNELS; i++) {
        pinMode(MCP23017_0 + i, OUTPUT);

        pinMode(MCP23017_1 + i, INPUT);
        pullUpDnControl(MCP23017_1 + i, PUD_UP);
    }

    printf("INFO: set mcp23017 2/3 for synthesizer for buttons and leds\n");
    for (i = 0; i < 16; i++) {
        pinMode(MCP23017_2 + i, INPUT);
        pullUpDnControl(MCP23017_2 + i, PUD_UP);

        pinMode(MCP23017_3 + i, OUTPUT);
    }

    sleep(1);

    pthread_t drummer_ids[DRUM_CHANNELS];
    pthread_t synthesizer_ids[SYNTHS];
    pthread_t hardware_id;

    printf("INFO: start hardware loop\n");
    pthread_create(&hardware_id, NULL, hardware, NULL);

    int channel_id;
    for (channel_id = 0; channel_id < DRUM_CHANNELS; channel_id++) {
        printf("INFO: create player for drum channel %d\n", channel_id);
        pthread_create(&drummer_ids[channel_id], NULL, drummer, (void*)(int) channel_id);
    }

    for (int synth_id = 0; synth_id < SYNTHS; synth_id++) {
        printf("INFO: create player for synthesizer %d\n", synth_id);
        pthread_create(&synthesizer_ids[synth_id], NULL, synthesizer, (void*)(int) synth_id);
    }

    sleep(1);

    printf("INFO: load pattern\n");
    load_pattern();

    struct timeval start_time, end_time;

    while (1) {
        for (int beat = 0; beat < 16; beat++) {
            gettimeofday(&start_time, NULL);
            for (int channel_id = 0; channel_id < DRUM_CHANNELS; channel_id++) {
                if (pattern[channel_id][beat] == '1') {
                    pattern_led_values[channel_id] = 1;

                    if (power_values[channel_id] == 1) {
                        printf("DEBUG: send information to play drum channel %d ->\n", channel_id);
                        pthread_cond_signal(&cond_drums[channel_id]);
                        power_led_values[channel_id] = 1;
                    } else {
                        power_led_values[channel_id] = 0;
                    }
                } else {
                    pattern_led_values[channel_id] = 0;
                    power_led_values[channel_id] = 0;
                }
            }

            int default_sleep_in_microseconds = 60000000 / bpm / 4;
            gettimeofday(&end_time, NULL);
            int elapsed_microseconds = end_time.tv_usec - start_time.tv_usec;

            int sleep_in_ms = default_sleep_in_microseconds - elapsed_microseconds;
            printf("DEBUG: elapsed microseconds %d, default microseconds are %d, so sleep %d microseconds\n", elapsed_microseconds, default_sleep_in_microseconds, sleep_in_ms);

            if (sleep_in_ms > 0) {
                usleep(sleep_in_ms);
            }
        }
    }

	return 0;
}