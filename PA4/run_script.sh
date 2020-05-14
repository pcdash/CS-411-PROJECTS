#!/bin/bash

for ((n = 1024; n <= 3000000000; n *= 2))
do
  for (( proc=1; proc<=32; proc *= 2 ))
  do
     ~/pi_est $((n*proc)) $proc &
     BACK_PID=$!
     wait $BACK_PID
  done
  printf '\n' >> ~/pi_results.txt
  printf '\n' >> ~/time_results.txt
done

