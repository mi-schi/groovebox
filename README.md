# groovebox

The alsa configuration for raspi can be found in ´asoundrc´. Rename it to ´.asoundrc´ and copy it to the home directory of your raspberry. There is a concatination between volume and equalizer using alsa softvol and alsa equal for each of the channels.

## set_alsa.c

To test, if the settings for alsa softvol and equal works with c.

    gcc ./set_alsa.c -o ./set_alsa -lasound -lm
