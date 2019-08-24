#!/bin/bash  
  
for((i=1;i<=${1};i++));  
do   
    ./test_log_producer $i $i & 
done  
