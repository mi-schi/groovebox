#! /bin/bash

array=( kick clap snare tom hat hihat synth )

cd samples_collected

for folder in "${array[@]}"; do
    echo $folder
    cd $folder
    i=1000
    for file in *.wav; do
        if [ ${#file} -gt 5 ]
        then 
            echo "file found: $file"
            sox $file --bits 16 --channels 1 --endian little --encoding signed-integer ../../samples/$folder/sample_${i:1:3}.wav
            ((i=i+1))
        fi
    done
    cd ../
done

