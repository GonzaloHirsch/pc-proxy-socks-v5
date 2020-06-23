#!/bin/bash

## { time ./tests/test_time.sh -xs -name /home/gonzalo/Downloads/ >/dev/null; } |& grep real

if [[ $# -ne 4 ]]; then
	echo 'Usage: test_time.sh [-xs,-s,-m,-l] [-4, -6, -name] [amount] [outputFilePath]';
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

    OUTPUT_FILE_PATH=$(echo $4 | sed 's:/*$::')

    echo "########### START TEST ###########" >> ./test_results/results_proxy.txt

    echo "Starting test..."

    echo "#### START PROXY ####" >> ./test_results/results_proxy.txt

    echo "Starting proxy test..."

    for i in $(seq 1 $3); do
	    if [[ $2 == "-4" ]]; then
            { time curl -4 -x socks5://gonza:test@localhost:1080 $url 2>/dev/null > "$OUTPUT_FILE_PATH/download$i.pdf" >/dev/null; } |& grep real | awk '{ print $2 }' >> ./test_results/results_proxy.txt
        elif [[ $2 == "-6" ]]; then
            { time curl -6 -x socks5://gonza:test@localhost:1080 $url 2>/dev/null > "$OUTPUT_FILE_PATH/download$i.pdf" >/dev/null; } |& grep real | awk '{ print $2 }' >> ./test_results/results_proxy.txt
        elif [[ $2 == "-name" ]]; then
            { time curl --socks5-hostname gonza:test@localhost:1080 $url 2>/dev/null > "$OUTPUT_FILE_PATH/download$i.pdf" >/dev/null; } |& grep real | awk '{ print $2 }' >> ./test_results/results_proxy.txt
        else
            { time curl --socks5-hostname gonza:test@localhost:1080 $url 2>/dev/null > "$OUTPUT_FILE_PATH/download$i.pdf" >/dev/null; } |& grep real | awk '{ print $2 }' >> ./test_results/results_proxy.txt
        fi
    done

    echo "Finished proxy test..."

    echo "#### END PROXY ####" >> ./test_results/results_proxy.txt

    echo "#### START NORMAL ####" >> ./test_results/results_proxy.txt

    echo "Starting normal test..."

    for i in $(seq 1 $3); do
	    if [[ $2 == "-4" ]]; then
            { time curl -4 $url 2>/dev/null > "$OUTPUT_FILE_PATH/download$i.pdf" >/dev/null; } |& grep real | awk '{ print $2 }' >> ./test_results/results_proxy.txt
        elif [[ $2 == "-6" ]]; then
            { time curl -6 $url 2>/dev/null > "$OUTPUT_FILE_PATH/download$i.pdf" >/dev/null; } |& grep real | awk '{ print $2 }' >> ./test_results/results_proxy.txt
        elif [[ $2 == "-name" ]]; then
            { time curl $url 2>/dev/null > "$OUTPUT_FILE_PATH/download$i.pdf" >/dev/null; } |& grep real | awk '{ print $2 }' >> ./test_results/results_proxy.txt
        else
            { time curl $url 2>/dev/null > "$OUTPUT_FILE_PATH/download$i.pdf" >/dev/null; } |& grep real | awk '{ print $2 }' >> ./test_results/results_proxy.txt
        fi
    done

    echo "Finished normal test..."

    echo "#### END NORMAL ####" >> ./test_results/results_proxy.txt

    echo "Finished test..."

    echo "########### END TEST ###########" >> ./test_results/results_proxy.txt
fi
