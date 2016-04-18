#! /bin/bash

# find where this script is being run from (could be local, could be in the path)
fullpath=$(which $0)
script_path=${fullpath%/*}

$script_path/iohyp_manager.sh "$@" -z -f -n
exit $?
