#!/bin/bash

UBUNTU_URL=https://releases.ubuntu.com/20.04/ubuntu-20.04-desktop-amd64.iso?_ga=2.259928202.1892971045.1592146811-255999976.1581382552
MEDIUM_URL=https://www.diewerkstatt-wgt.ch/app/download/9504125398/Katalog_die_werkstatt_Freiraumm%C3%B6bel.pdf?t=1400088350
SMALL_URL=http://www.uoitc.edu.iq/images/documents/informatics-institute/exam_materials/Computer%20Networks%20-%20A%20Tanenbaum%20-%205th%20edition.pdf
VSMALL_URL=http://users.physik.fu-berlin.de/~kleinert/files/1905_17_891-921.pdf
url=$VSMALL_URL

echo $url
for i in $(seq 1 500); do
	curl -x socks5://localhost:1080 $url > /media/click/TOSHIBA\ EXT/Documents/Protos/TPE/download$i.pdf &
done
