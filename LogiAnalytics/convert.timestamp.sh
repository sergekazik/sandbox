#!/bin/bash
if [[ "$1" == "?" ]] || [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
    echo "format: convert.timestamp.sh filename [-v]"
    echo "-v verbose"
    exit -1
elif [[ "$1" != "" ]]; then
    input="$1"
else
    echo "missing argument: input file. Abort"
    echo "format: convert.timestamp.sh filename [-v]"
    echo "-v verbose"
    exit -1
fi
output="$input.timestamp.processed.json"
rm $output -f

if [[ "$2" == "-v" ]]; then
    verbose="true"
else
    verbose="false"
fi
linux='false'
if [ `uname | grep -i linux` -gt 0 ]; then
	linux='true'
fi

count=0
maxline=$(wc -l < $input)
echo -e "processing:\ninput:  $input, $maxline lines\noutput: $output"

while IFS= read -r line
do
  count=$((count+1))
  if [ `echo $line | grep -c "timestamp" ` -gt 0 ]
  then
    ts=${line:17:10}
    
    if [ "$linux" = true ] ; then
    	dt=$(date -d "@$ts")
    else
        dt=$(date -r $ts)
    fi
    
    echo "$line" >> $output
    if [ "$verbose" = true ] ; then
    	echo "$line"
    fi
    
    echo "    \"datetime\": \"$dt\","  >> $output
    if [ "$verbose" = true ] ; then
    	echo "    \"datetime\": \"$dt\"," 
    fi
  else
    echo "$line" >> $output
    if [ "$verbose" = true ] ; then
       echo "$line"
    fi
  fi
  
  # progress if -q
  if [ "$verbose" = false ] ; then
       prs=$((count*100/maxline))
       printf "\r$prs%%"
  fi
done < "$input"
echo ""

