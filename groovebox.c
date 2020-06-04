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

#define SAMPLES 6
#define SAMPLES_SIZE 25
#define ALL_PATTERN_SIZE 32
#define EQUALIZERS 10

#define FILENAME_PREFIX "/media/ramdisk/sample_"
#define FILENAME_SIZE strlen(FILENAME_PREFIX) + 7

#define MCP23017_0 100
#define MCP23017_1 200
#define MCP3008_0 300
#define MCP3008_1 400

#define ALSA_INDEX 0

char *pcm_devices[6] = {"ch0", "ch1", "ch2", "ch3", "ch4", "ch5"};

char *equalizer_names[EQUALIZERS] = {
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
    "0100000001000000",
    // french house
    "0101010101010101",
    "1110101111101011",
    // dubstep
    "0000001000000000",
    "1100011111011101",
    "1010100010101000",
    // hip hop
    "0000010000000100",
    "1101101111011011",
    "1001000010110000",
    // break beat
    "1100010011000111",
    "0000001000100000",
    "0000000000000100",
    "0000100001001000",
    "1001001010001000"
};

char *pattern[SAMPLES];
int bpm = 140;

int play_samples[SAMPLES] = {1, 1, 1, 1, 1, 1};
int led_values[SAMPLES] = {0, 0, 0, 0, 0, 0};
int power_values[SAMPLES] = {0, 0, 0, 0, 0, 0};

pthread_mutex_t mutex_play[SAMPLES];
pthread_cond_t cond_play[SAMPLES];

char *get_filepath(int sample_id) {
    char *path = malloc(FILENAME_SIZE);
    snprintf(path, FILENAME_SIZE, "%s%02d.wav", FILENAME_PREFIX, sample_id);
    printf("DEBUG: file path %s\n", path);

    return path;    
}

void player(void* id) {
    int sample_id = (int) id;
    printf("INFO: start thread for sample %d\n", sample_id);
    char *path = get_filepath(sample_id);
    
    while (1) {
        printf("INFO: open wav file for sample %d with pcm device %s\n", sample_id, pcm_devices[sample_id]);
        SF_INFO sfinfo;
        SNDFILE *sndfile;
        sndfile = sf_open(path, SFM_READ, &sfinfo);
        
        snd_pcm_t *pcm;
        snd_pcm_open(&pcm, pcm_devices[sample_id], SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);

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
        
        int readcount, pcmrc;
        play_samples[sample_id] = 1;
        
        while (play_samples[sample_id] == 1) {
            pthread_cond_wait(&cond_play[sample_id], &mutex_play[sample_id]);
            pthread_mutex_unlock(&mutex_play[sample_id]);

            printf("DEBUG: -> play sample %d\n", sample_id); 
            snd_pcm_prepare(pcm);
            
            while ((readcount = sf_readf_short(sndfile, buffer, frames)) > 0) {
                pcmrc = snd_pcm_writei(pcm, buffer, readcount);
            }
            
            sf_seek(sndfile, 0, SEEK_SET);
        }
        
        printf("INFO: close pcm and free buffer for sample %d\n", sample_id);
        snd_pcm_drain(pcm);
        snd_pcm_close(pcm);
        free(buffer);
        
        printf("INFO: copy sample %d\n", sample_id);
        FILE *new_sample, *sample;
        new_sample = fopen(get_filepath((rand() % (SAMPLES_SIZE - SAMPLES)) + SAMPLES), "r");
        sample = fopen(get_filepath(sample_id), "w");
        
        short b; 
        while ((b = fgetc(new_sample)) != EOF) { 
            fputc(b, sample); 
        } 
        
        fclose(new_sample);
        fclose(sample);
    }
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

void set_equalizer(long poti_value, int sample_id) {
    char *card = malloc(10);
    snprintf(card, 10, "ch%d_equal", sample_id);
    printf("DEBUG: equalizer card %s\n", card);
    
    long factor = map(poti_value, 0, 1023, 50, -50);
    long adder = 66;
    
    if (poti_value < 512) {
        adder = map(poti_value, 0, 511, 50, 66);
    } else {
        adder = map(poti_value, 512, 1023, 66, 50);
    }
    
    for (int channel = 0; channel < EQUALIZERS; channel++) {
        long equalizer_value = factor * cos(channel / M_PI) + adder;
        printf("INFO: set volume to %d for equalizer channel %s for sample %d\n", equalizer_value, equalizer_names[channel], sample_id);
        set_volume(card, ALSA_INDEX, equalizer_names[channel], equalizer_value);
    }
}

void load_pattern() {
    for (int i = 0; i < SAMPLES; i++) {
        pattern[i] = all_pattern[i];
    }
}

void next_pattern(int i) {
    pattern[i] = all_pattern[rand() % ALL_PATTERN_SIZE];
}

void hardware() {
    srand(time(NULL));
    
    long poti_volume_values[SAMPLES];
    long poti_equalizer_values[SAMPLES];
    long poti_range = 10;
    int old_led_values[SAMPLES];
        
    while (1) {
        for (int sample_id = 0; sample_id < SAMPLES; sample_id++) {
            // set volumes
            long poti_volume_value = analogRead(MCP3008_0 + sample_id);
            
            if (poti_volume_value + poti_range < poti_volume_values[sample_id] || poti_volume_value - poti_range > poti_volume_values[sample_id]) {
                long volume = map(poti_volume_value, 0, 1023, 0, 100);
                printf("INFO: set volume to %d (raw %d, further %d) for %s\n", volume, poti_volume_value, poti_volume_values[sample_id], pcm_devices[sample_id]);
                set_volume("default", ALSA_INDEX, pcm_devices[sample_id], volume);
                poti_volume_values[sample_id] = poti_volume_value;
            }
            
            // set equalizers
            long poti_equalizer_value = analogRead(MCP3008_1 + sample_id);
            
            if (poti_equalizer_value + poti_range < poti_equalizer_values[sample_id] || poti_equalizer_value - poti_range > poti_equalizer_values[sample_id]) {
                printf("INFO: set equalizer with poti value %d, further %d for sample %d\n", poti_equalizer_value, poti_equalizer_values[sample_id], sample_id);
                set_equalizer(poti_equalizer_value, sample_id);  
                poti_equalizer_values[sample_id] = poti_equalizer_value;
            }
            
            // read pattern buttons on bank a
            if (digitalRead(MCP23017_1 + sample_id) == 0) {
                printf("EVENT: set next pattern for sample %d\n", sample_id);
                pthread_mutex_lock(&mutex_play[sample_id]);
                next_pattern(sample_id);
                pthread_mutex_unlock(&mutex_play[sample_id]);
            }
            
            // read sample buttons on bank b
            if (digitalRead(MCP23017_1 + sample_id + 8) == 0) {
                printf("EVENT: set next sample for sample %d\n", sample_id);
                play_samples[sample_id] = 0;
            }
            
            // set powers, 0 = on, 1 = off
            int power = digitalRead(MCP23017_0 + sample_id);
            
            if (power == 0 && power_values[sample_id] == 0) {
                power_values[sample_id] = 1;
            } 
            
            if (power == 1 && power_values[sample_id] == 1) {
                power_values[sample_id] = 0;
            }
            
            // switch leds
            if (led_values[sample_id] != old_led_values[sample_id]) {
                digitalWrite(MCP23017_0 + 8 + sample_id, led_values[sample_id]);
                old_led_values[sample_id] = led_values[sample_id];
            }
        }
        
        usleep(50000*2);
    }
}

int main(int argc, char **argv) {
    printf("INFO: initialize hardware\n");
    wiringPiSetup();
  
    mcp23017Setup(MCP23017_0, 0x20);
    mcp23017Setup(MCP23017_1, 0x21);
  
    mcp3004Setup(MCP3008_0, 0);
    mcp3004Setup(MCP3008_1, 1);

    printf("INFO: set mcp23017 bank a\n");
    int i;
    for (i = 0; i < 8; i++) {
        pinMode(MCP23017_0 + i, INPUT);
        pullUpDnControl(MCP23017_0 + i, PUD_UP);
        
        pinMode(MCP23017_1 + i, INPUT);
        pullUpDnControl(MCP23017_1 + i, PUD_UP);
    }
    
    printf("INFO: set mcp23017 bank b\n");
    for (i = 8; i < 16; i++) {
        pinMode(MCP23017_0 + i, OUTPUT);
        
        pinMode(MCP23017_1 + i, INPUT);
        pullUpDnControl(MCP23017_1 + i, PUD_UP);
    }
    
    sleep(1);
    
    pthread_t thread_ids[SAMPLES];
    pthread_t hardware_id;
    
    printf("INFO: start hardware loop\n");
    pthread_create(&hardware_id, NULL, hardware, NULL);
    
    for (int sample_id = 0; sample_id < SAMPLES; sample_id++) {
        printf("INFO: create player for sample %d\n", sample_id);
        pthread_create(&thread_ids[sample_id], NULL, player, (void*)(int) sample_id);         
    }
    
    sleep(1);
    
    printf("INFO: load pattern\n");
    load_pattern();
    
    struct timeval start_time, end_time;
    int default_sleep_in_ms = 60000000 / bpm / 4;
    
    while (1) {
        for (int beat = 0; beat < 16; beat++) {
            gettimeofday(&start_time, NULL);
            for (int sample_id = 0; sample_id < SAMPLES; sample_id++) {
                if (pattern[sample_id][beat] == '1' && power_values[sample_id] == 1) {
                    printf("DEBUG: send information to play sample %d ->\n", sample_id);
                    led_values[sample_id] = 1;
                    pthread_cond_signal(&cond_play[sample_id]);
                } else {
                    led_values[sample_id] = 0;
                }                 
            }
            
            gettimeofday(&end_time, NULL);
            int elapsed_microseconds = end_time.tv_usec - start_time.tv_usec;
            int sleep_in_ms = default_sleep_in_ms - elapsed_microseconds;
            printf("DEBUG: elapsed microseconds %d, default microseconds are %d, so sleep %d microseconds\n", elapsed_microseconds, default_sleep_in_ms, sleep_in_ms);
            
            if (sleep_in_ms > 0) {
                usleep(sleep_in_ms);
            }
        }
    }
    
	return 0;
}
