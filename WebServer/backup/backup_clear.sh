#!/bin/bash
mkdir -p ./$1/
mkdir -p ./$1/$(date +"%Y_%m_%d_%H_%M_%S")+$2/
cp ../db/VizLensDynamic.db ./$1/$(date +"%Y_%m_%d_%H_%M_%S")+$2/
cp -vr ../images_original ./$1/$(date +"%Y_%m_%d_%H_%M_%S")+$2/
cp -vr ../images_reference ./$1/$(date +"%Y_%m_%d_%H_%M_%S")+$2/
cp -vr ../images_video ./$1/$(date +"%Y_%m_%d_%H_%M_%S")+$2/
python ./clear_video.py
