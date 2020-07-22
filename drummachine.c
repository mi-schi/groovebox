#include <alsa/asoundlib.h>
#include <stdio.h>
#include <sndfile.h>
#include <pthread.h>
#include <errno.h>

#define DRUM_CHANNELS 6
#define SYNTHS 16

#define FILENAME_PREFIX "/media/ramdisk"

int bpm = 120;

char *drum_pcm_devices[DRUM_CHANNELS] = {"default", "default", "default", "default", "default", "default"};
char *drum_folders[DRUM_CHANNELS] = {"kick", "clap", "snare", "tom", "hat", "hihat"};

char *pattern[6] = {
    "1000100010001000",
    "0000100000001000",
    "0010001000100010",
    "0000100000001000",
    "1000100010001000",
    "0100010001000100",
    /*
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
    "1001001010001000" */
};

int next_drums[DRUM_CHANNELS];

pthread_cond_t cond_drums[DRUM_CHANNELS];
pthread_mutex_t mutex_drums[DRUM_CHANNELS];

char *synth_pcm_device = "default";
char *synth_folder = "synth";

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

void hardware() {
    sleep(1);
    printf("READER: load new file.\n");
    next_drum(0);
    pthread_cond_signal(&cond_synth[0]);
    pthread_cond_signal(&cond_synth[2]);
    usleep(500000);
    pthread_cond_signal(&cond_synth[2]);
    usleep(500000);
    pthread_cond_signal(&cond_synth[2]);
    pthread_cond_signal(&cond_synth[15]);

    sleep(100);
    next_synth();

    sleep(1);
    pthread_cond_signal(&cond_synth[0]);
    pthread_cond_signal(&cond_synth[15]);
}

int main(int argc, char **argv) {
    pthread_t drummer_ids[DRUM_CHANNELS];
    pthread_t synthesizer_ids[SYNTHS];
    pthread_t hardware_id;

    pthread_create(&hardware_id, NULL, hardware, NULL);

    int channel_id;
    for (channel_id = 0; channel_id < DRUM_CHANNELS; channel_id++) {
        printf("INFO: create player for channel %d\n", channel_id);
        pthread_create(&drummer_ids[channel_id], NULL, drummer, (void*)(int) channel_id);
    }

    for (int synth_id = 0; synth_id < SYNTHS; synth_id++) {
        printf("INFO: create player for synthesizer %d\n", synth_id);
        pthread_create(&synthesizer_ids[synth_id], NULL, synthesizer, (void*)(int) synth_id);
    }

    int sleep_in_ms = 60000000 / bpm / 4;
    int i = 0;

    while (i < 3) {
        for (int beat = 0; beat < 16; beat++) {
            for (channel_id = 0; channel_id < DRUM_CHANNELS; channel_id++) {
                if (pattern[channel_id][beat] == '1') {
                    printf("DEBUG: send information to play channel %d ->\n", channel_id);
                    pthread_cond_signal(&cond_drums[channel_id]);
                }
            }

            usleep(sleep_in_ms);
        }

        i++;
    }

	return 0;
}