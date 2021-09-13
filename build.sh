#!/bin/bash

BASE=$(dirname $0)

sourceDir=$BASE/src
binDir=$BASE/bin


if [[ ! -d "$sourceDir" ]] ; then
	echo "Source directory not found: $sourceDir"
	exit
fi

if [[ ! -d "$binDir" ]] ; then
	echo "Create $binDir"
	mkdir -p "$binDir"
fi



echo "Build arp-scan-util"
gcc -Wall -I./lib/liblmdb/ -o "$binDir/arp-scan-util" "$sourceDir/arp-scan-util.c" 
