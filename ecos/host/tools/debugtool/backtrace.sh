#!/bin/bash
# This Script needs ELF and log file
# Usage backtrace.sh <ELF> <log>

# log format should contain one addresses at a line with no 0x
	# ex: 80000123
	#     80013131

GDB=`which mipsisa32-elf-gdb`			# change here for gdb prefix

printIt()
{
	tmp=`$GDB --batch --se=${_ELF_} -ex 'info symbol '"$1"''`"";
	tmp=`echo $tmp | grep text | grep +`
	[[ $tmp ]] && echo $tmp
}

if [[ -z "$2" ]] 
then
	echo "Usage backtrace.sh <ELF> <log>"
	exit
else
	_ELF_="$1"
fi

if [[ -z "$GDB" ]]
then
        echo "Command not found: mipsisa32-elf-gdb"
        exit
fi

if [[ "-stack" == "$3" ]]
then
	arr=($(cat $2))
	count=($(cat $2 | wc -w))
else
	arr=($(cat $2 | cut -d' ' -f6))
	count=($(cat $2 | wc))
fi

for each in `seq 0 $count`
do
 	[[ "${arr[(each)]}" -ge "0x80000000" ]] && printIt ${arr[each]}
done

