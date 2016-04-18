#!/bin/bash

LINES=20
FILTER=""

INVERSE=0
TRIM_DELIM="]"

function usage {
    echo "$0"	
    echo "   -c                   Clears the console log"
    echo "   -v                   Filter out any non-vRIO traces"
    echo "   -e                   Show errors only"
    echo "   -a                   Show asserts only"
    echo "   -m                   Show messages only"
    echo "   -n                   Show notice messages only"
    echo "   -i                   Filter out vRIO traces"
    echo "   -t                   Trim traces prefix"
    echo "   -s                   Show all"
    exit
}

while getopts cveamntsi option
do
        case "${option}"
        in
                c) sudo dmesg -C
                   exit 0;;
                v) FILTER+="|vrio";;
                e) FILTER+="|ERROR";;
                a) FILTER+="|ASSERT";;
                m) FILTER+="|MESSAGE";;
                n) FILTER+="|NOTICE";;
                t) TRIM_DELIM=">";;
                i) INVERSE=1;;
                h) usage
                    exit 1;;
                s) LINES=16777216;;
                \?) usage
                    exit 1;;
       esac
done

# Remove the surplus '|'
FILTER=${FILTER#"|"}

POS_ARG=${@:$OPTIND:1}
if [[ -n "$POS_ARG" ]]
then
	LINES=$POS_ARG
fi

if [ $INVERSE -eq 1 ]; then
   dmesg -s 16777216 | grep -v vrio | tail -$LINES
else
   dmesg -s 16777216 | egrep "$FILTER" | tail -$LINES | cut -d $TRIM_DELIM -f 2- | cut -d ' ' -f 2-
fi

