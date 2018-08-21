#!/bin/bash
filename="iperf.out"
rm $filename

iperf -c 10.0.1.15 -p 123 -u >> $filename 2>&1
iperf -c 10.0.1.15 -p 5060 -u >> $filename 2>&1
iperf -c 10.0.1.15 -p 16384 -u >> $filename 2>&1

status=""
while read -r line
do
    if [[ $line = *"connecting"* ]]; then
        port=$(echo "$line" | cut -d' ' -f 7)
    elif [[ $line = *"Server Report:"* ]]; then
        status="ok"
    elif [[ $line = *"failed"* ]]; then
        status="fail"
    elif [[ $line = *"WARNING: did not"* ]]; then
        status="fail"
    fi

    if [[ $status != "" ]]; then
        echo "status_report[port_blocking][port_$port]=$status"
        status=""
    fi
done < "$filename"

