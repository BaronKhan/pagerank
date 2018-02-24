#!/bin/bash
# INPUT: num_iters + list of strings which match make [type of test]
#   e.g: ./run_stats.sh 4 "small-test" "medium-test"
# test if first param is interger
if [ "$1" -gt 1 ]
then
    num_iters=$1
else
    echo "ERROR: first paramter(iterations) must be a integer > 1."
    exit 1
fi

echo "" > data.tempdata  # wipes file if exist
for instr in $@ #"small-test" "large-test" etc
  do
    # skips first integer arg
    if [ "${instr}" == "$1" ]
    then
      echo "Starting..."
    else
    for ((i=0; i< ${num_iters}; i++)) do
      echo  ${instr} ": Run ${i}"
      $(make ${instr} 2>&1 | grep ":.*" >> data.tempdata ) # appends result to file and skips screen output
    done
  fi
done
# runs actual data analyzing
python analyze_data.py ${num_iters} $#
