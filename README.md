# groovebox

With this project you can use a raspberry to create your own groovebox with a drummachine and a synthesizer. You will need
- a raspberry with sd card and raspian os
- an external sound card (with the onboard soundcard it is not possible to play two files simultaneously)
- buttons, switches, potentiometers with knobs, perfboards, leds, MCP3008s, MCP23017s, resitors and a lot of wires (see microcontroller.png for the circuit diagram.)
- soldering iron and accessories
- a lot of time, also for finding errors

## installation

The alsa configuration for raspi can be found in `asoundrc`. Rename it to `.asoundrc` and copy it to the home directory of your raspberry or if you want it globally to `/etc/asound.conf`. There is a concatination between volume and equalizer using alsa softvol and alsa equal for each of the channels.

Create a ram disk to speed up the reading of the wav files and protect the sd card:

    sudo mount -t tmpfs -o size=50M none /media/ramdisk/
    
Upload or checkout the project (sample folder and the c-files) to the home directory on the raspberry.
You have to compile the executable code on the raspberry, not on your pc.

If you want to start the grovvebox after booting the raspberry, add the groovestart.sh to you crontab:

    @reboot /home/pi/groovestart.sh > /home/pi/logs/groovebox.log 2>&1

There are a lot of other configuration for raspi to run the project.
Unfortunately I don't remember :( Contact me, if you have problems or questions.

## start with testing the components

First, test the alsa configuration on your raspberry.
You don't need to build up the hole project with the hardware for it.

### set_alsa.c

To test, if the settings for alsa softvol and equal works with c.

    gcc ./set_alsa.c -o ./set_alsa -lasound -lm

### drummachine.c

To test, if the code works on a normal computer.

    gcc ./drummachine.c ./drummachine -lsndfile -lpthread -lasound

## start the main script

If everything works without the external circuits you can start the main script.

### groovbox.c

The code for the whole project on raspi with MCP23017 and MCP3008 connection. See microcontroller.png for the circuit diagram.
You can compile it with this command:

    gcc ./groovebox.c ./groovebox -lsndfile -lasound -lpthread -lwiringPi -lm

## use other sample files

The sample files should be in the format "Signed 16 bit Little Endian". For better performance use 44100Hz and mono.
Use sox to convert the wav files:

    sudo apt install sox
    sox input.wav --bits 16 --channels 1 --endian little --encoding signed-integer sample_000.wav
    
Use the given folder structure and the file name pattern "sample_XXX.wav".
Look into `multiple_converter.sh`. You can create a folder `samples_collected` with the given subfolder,
copy all you samples in the folders and convert and rename it to the `samples` folder.