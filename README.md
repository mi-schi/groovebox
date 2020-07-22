# groovebox

The alsa configuration for raspi can be found in `asoundrc`. Rename it to `.asoundrc` and copy it to the home directory of your raspberry or if you want it globally to `/etc/asound.conf`. There is a concatination between volume and equalizer using alsa softvol and alsa equal for each of the channels.

There are a lot of configuration for raspi to run the project. Contact me, if you have problems or questions.

## set_alsa.c

To test, if the settings for alsa softvol and equal works with c.

    gcc ./set_alsa.c -o ./set_alsa -lasound -lm

## drummachine.c

To test, if the code works on a normal computer.

    gcc ./drummachine.c ./drummachine -lsndfile -lpthread -lasound

## groovbox.c

The code for the whole project on raspi with mcp23017 and mcp3008 connection. See microcontroller.png for the circuit diagram.

    gcc ./groovebox.c ./groovebox -lsndfile -lasound -lpthread -lwiringPi -lm

