#!/bin/bash  
  
for((i=1;i<=${1};i++));  
do   
    ./log_sender $i & 
done  
