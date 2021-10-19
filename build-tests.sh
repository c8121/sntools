#!/bin/bash

BASE=$(realpath $(dirname $0))

sourceDir=$BASE/src/test
binDir=$BASE/bin/test

if [[ ! -d "$sourceDir" ]] ; then
	echo "Source directory not found: $sourceDir"
	exit
fi

if [[ ! -d "$binDir" ]] ; then
	echo "Create $binDir"
	mkdir -p "$binDir"
fi

cd $BASE
gcc -g -Wall -o "$binDir/linked_items_test" "$sourceDir/linked_items_test.c"
gcc -g -Wall -o "$binDir/html_convert_test" "$sourceDir/html_convert_test.c"


