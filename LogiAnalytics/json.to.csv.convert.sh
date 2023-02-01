#!/bin/bash
help()
{
    echo "format: json.to.csv.convert.sh filename.timestamp.processed.json [-h|-v]"
    echo "-h	help"
    echo "-v 	verbose"
}
if [[ "$1" == "?" ]] || [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
    help
    exit -1
elif [[ "$1" != "" ]]; then
    input="$1"
else
    echo "missing argument: input file. Abort"
    help
    exit -1
fi
output="$input.csv"
rm $output -f

if [[ "$2" == "-v" ]]; then
    verbose="true"
else
    verbose="false"
fi

count=0
maxline=$(wc -l < $input)
echo -e "processing:\ninput:  $input, $maxline lines\noutput: $output\n"

while IFS= read -r line
do
  count=$((count+1))
  if [ `echo $line | grep -c "timestamp" ` -gt 0 ]; then
    ts="$(cut -d':' -f2 <<<$line)"
    echo -n $ts >> $output
    if [ "$verbose" = true ] ; then
    	echo -n $ts
    fi
    
    #datetime
    read line
    count=$((count+1))
    if [ `echo $line | grep -c "datetime" ` -gt 0 ]; then
    	dt="$(cut -d'"' -f4 <<<$line)"
    	echo -n "\"$dt\"," >> $output
	if [ "$verbose" = true ] ; then
	   echo -n "\"$dt\","
	fi
    else
    	echo -e "\nwrong file format: expect datetime after timestamp. Abort"
    	exit -2
    fi
    
    #type
    read line
    count=$((count+1))
    if [ `echo $line | grep -c "type" ` -gt 0 ]; then
    	tp="$(cut -d'"' -f4 <<<$line)"
    	echo -n "\"$tp\"," >> $output
	if [ "$verbose" = true ] ; then
	   echo -n "\"$tp\","
	fi
    else
    	echo -e "\nwrong file format: expect type after datetime. Abort"
    	exit -3
    fi
    
    #all other info until "},"
    details=0
    while read line
    do	
    	count=$((count+1))
    	if [ "$line" = "}," ]; then
    	   if [ "$details" -eq "0" ]; then
    	   	echo "" >> $output
    	   else 
    	   	echo "\"" >> $output
    	   fi
    	   if [ "$verbose" = true ] ; then
    	      echo ""
    	   fi
    	   break
	fi
	
	if [ "$details" -eq "0" ]; then
	   echo -n "\"" >> $output
	fi
    	echo -n "$line" |  sed -r 's/,/; /g' |  sed -r 's/"//g'  >> $output
    	if [ "$verbose" = true ] ; then
    	   echo -n "$line" |  sed -r 's/,/ /g'
    	fi
    	
    	details=$((details+1))
     done
  fi
  
  # progress if -q
  if [ "$verbose" = false ] ; then
       prs=$((count*100/maxline))
       printf "\r$prs%%"
  fi
done < "$input"
echo ""

