#!/bin/bash

if [[ $# -ne 3 ]]; then
	echo 'Usage: test.sh [-xs,-s,-m,-l] [number_of_connecions] [outputFilePath]';
else

LARGE_URL=https://releases.ubuntu.com/20.04/ubuntu-20.04-desktop-amd64.iso?_ga=2.259928202.1892971045.1592146811-255999976.1581382552
MEDIUM_URL=https://www.diewerkstatt-wgt.ch/app/download/9504125398/Katalog_die_werkstatt_Freiraumm%C3%B6bel.pdf?t=1400088350
SMALL_URL=http://www.uoitc.edu.iq/images/documents/informatics-institute/exam_materials/Computer%20Networks%20-%20A%20Tanenbaum%20-%205th%20edition.pdf
VSMALL_URL=http://users.physik.fu-berlin.de/~kleinert/files/1905_17_891-921.pdf

if [[ $* == *-xs* ]]; then
	url=$VSMALL_URL
elif [[ $* == *-s* ]]; then
	url=$SMALL_URL
elif [[ $* == *-m* ]]; then
	url=$MEDIUM_URL
elif [[ $* == *-l* ]]; then
	url=$LARGE_URL
else
	url=$VSMALL_URL
fi

OUTPUT_FILE_PATH=$(echo $3 | sed 's:/*$::')
for i in $(seq 1 $2); do
	curl -4 -x socks5://protos:protos@localhost:1080 $url > "$OUTPUT_FILE_PATH/download$i.pdf" &
done
fi
