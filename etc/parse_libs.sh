#!/bin/bash

## create json for all files

$1 -f $2 | awk -F " " '{print "[\"" $1 "\",\"" $3 "\"]\,"}' | tail -n+4