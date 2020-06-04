#include <alsa/asoundlib.h>
#include <stdio.h>
#include <sndfile.h>
#include <pthread.h>
#include <errno.h>

#define BPM 140
#define SAMPLES 6
#define SAMPLES_SIZE 25

#define FILENAME_PREFIX "/media/ramdisk/sample_"
#define FILENAME_SIZE strlen(FILENAME_PREFIX) + 7

char *pcm_devices[6] = {"default", "default", "default", "default", "default", "default"};

char *pattern[6] = {
    "1000100010001000",
    "0000100000001000",
    "0010001000100010",
    "0000100000001000",
    "1000100010001000",
    "0100010001000100"
};

int play_samples[SAMPLES] = {1, 1, 1, 1, 1, 1};

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

void hardware() {
    srand(time(NULL));
    
    sleep(2);
    printf("READER: load new file.\n");
    play_samples[0] = 0;
}

int main(int argc, char **argv) {
    pthread_t thread_ids[SAMPLES];    
    pthread_t hardware_id;
    
    pthread_create(&hardware_id, NULL, hardware, NULL);
    
    for (int sample_id = 0; sample_id < SAMPLES; sample_id++) {
        printf("INFO: create player for sample %d\n", sample_id);
        pthread_create(&thread_ids[sample_id], NULL, player, (void*)(int) sample_id);         
    }
    
    sleep(1);
    
    int sleep_in_ms = 60000000 / BPM / 4;
    int i = 0;
    
    while (i < 2) {
        for (int beat = 0; beat < 16; beat++) {
            for (int sample_id = 0; sample_id < SAMPLES; sample_id++) {
                if (pattern[sample_id][beat] == '1') {
                    printf("DEBUG: send information to play sample %d ->\n", sample_id);
                    pthread_cond_signal(&cond_play[sample_id]);
                }                    
            }
            
            usleep(sleep_in_ms);
        }
        
        i++;
    }
    
	return 0;
}
