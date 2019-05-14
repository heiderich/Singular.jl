#!/bin/bash

## create json for all files

/home/sebastian/Software/Singular-git/Singular/libparse -f $1 | awk -F " " '{print "[\"" $1 "\",\"" $3 "\"]\,"}' | tail -n+4