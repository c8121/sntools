#!/bin/bash

BASE=$(realpath $(dirname $0))

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

# ---- make lmdb ---------
if [[ ! -f "/usr/local/lib/liblmdb.a" ]] ; then
	cd $BASE/lib/liblmdb
	make && sudo make install && sudo make clean
	sudo /sbin/ldconfig -v
fi


# ------------------------
echo "Build arp-scan-util"
cd $BASE
gcc -Wall -I./lib/liblmdb -o "$binDir/arp-scan-util" "$sourceDir/arp-scan-util.c" -llmdb
gcc -Wall -I./lib/liblmdb -o "$binDir/arp-history" "$sourceDir/arp-history.c" -llmdb
