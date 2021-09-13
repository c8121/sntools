#!/bin/bash
#
# Run bin/arp-scan-util and send e-mail if changes were detected
# Requires swaks to send e-mail (apt install swaks)
#

SENDER=""
SUBJECT="arp-scan detected changes"
SMTP_SERVER=root

BASE=$(realpath $(dirname $0))


usage="Usage: $0 <interface> <smtp-sever> <recipient>"

interface=$1
if [[ "$interface" == "" ]] ; then
	echo "$usage"
	exit
fi

smtpServer=$2
if [[ "$smtpServer" == "" ]] ; then
	echo "$usage"
	exit
fi

recipient=$3
if [[ "$recipient" == "" ]] ; then
	echo "$usage"
	exit
fi

if [[ "$SENDER" == "" ]] ; then
	echo "Warn: No sender defined in $0, using $recipient as sender"
	SENDER="$recipient"
fi

logfile=$(mktemp)
#echo "Write output to $logfile"

$BASE/bin/arp-scan-util -i "$interface" > "$logfile"

if grep -q "Notice" "$logfile" ; then
	
	messagefile=$(mktemp)
	
	echo "arp-scan detected changes" > "$messagefile"
	echo "Date: $(date)" >> "$messagefile"
	echo "" >> "$messagefile"
	cat "$logfile" >> "$messagefile"
	
	swaks --to "$recipient" --from "$SENDER" --server "$smtpServer" -h-Subject "$SUBJECT" --body "$messagefile"
	
	rm "$messagefile"
	
fi

rm "$logfile"
