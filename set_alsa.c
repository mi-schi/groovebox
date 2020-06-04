#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define ALSA_INDEX 0
#define EQUALIZERS 10
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

long map(long poti_value, long in_min, long in_max, long out_min, long out_max) {
    return (poti_value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void set_equalizer(long poti_value) {
    long factor = map(poti_value, 0, 1023, 50, -50);
    long adder = 66;
    if (poti_value < 512) {
        adder = map(poti_value, 0, 511, 50, 66);
    } else {
        adder = map(poti_value, 512, 1023, 66, 50);
    }
    for (int channel = 0; channel < EQUALIZERS; channel++) {
        long equalizer_value = factor * cos(channel / M_PI) + adder;
        printf("set volume to %d for channel %s\n", equalizer_value, equalizer_names[channel]);
    }
}

int main(int argc, char **argv) {
	printf("hello world\n");
    set_equalizer(0);
    //set_volume("default", ALSA_INDEX, "Master", 20);
    //set_switch("default", ALSA_INDEX, "Master", true);
	return 0;
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

void set_switch(char *card, int alsa_index, char *selem_name, bool on) {
    snd_mixer_t *handle;
    
    handle = open_mixer(card);
    snd_mixer_elem_t* elem = find_selem(handle, alsa_index, selem_name);
    
    snd_mixer_selem_set_playback_switch_all(elem, (int) on);
    
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
