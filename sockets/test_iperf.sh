#!/bin/bash
filename="iperf.out"
rm $filename

## ---- execute actual test - collect results in the out file
echo "iperf -c 10.0.1.15 -p 123 -u "
iperf -c 10.0.1.15 -p 123 -u >> $filename 2>&1
echo "iperf -c 10.0.1.15 -p 5060 -u"
iperf -c 10.0.1.15 -p 5060 -u >> $filename 2>&1
echo "iperf -c 10.0.1.15 -p 16384 -u"
iperf -c 10.0.1.15 -p 16384 -u >> $filename 2>&1

status=""
skip="yes"
while read -r line
do
    if [[ $line = "--------------"* ]]; then
        skip="no"
    fi

    if [[ $skip = "no" ]]; then
        if [[ $line = *"connecting"* ]]; then
            port=$(echo "$line" | cut -d' ' -f 7)
        elif [[ $line = *"Server Report:"* ]]; then
            status="ok"
        elif [[ $line = *"failed"* ]]; then
            status="fail"
        elif [[ $line = *"WARNING: did not"* ]]; then
            status="fail"
        fi
    fi

    if [[ $status != "" ]]; then
        echo "status_report[port_blocking][port_$port]=$status"
        status=""
        skip="yes"
    fi
done < "$filename"

