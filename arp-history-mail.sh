#!/bin/bash
#
# Run bin/arp-scan-util and send e-mail if changes were detected
# Requires swaks to send e-mail (apt install swaks)
#

SENDER=""
SUBJECT="$(hostname) arp-history"
SMTP_SERVER=root

BASE=$(realpath $(dirname $0))


usage="Usage: $0 <smtp-sever> <recipient>"

smtpServer=$1
if [[ "$smtpServer" == "" ]] ; then
	echo "$usage"
	exit
fi

recipient=$2
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

$BASE/bin/arp-history -t > "$logfile"


messagefile=$(mktemp)

echo "arp-history" > "$messagefile"
echo "Host: $(hostname)" >> "$messagefile"	
echo "Date: $(date)" >> "$messagefile"
cat "$logfile" >> "$messagefile"

swaks --to "$recipient" --from "$SENDER" --server "$smtpServer" -h-Subject "$SUBJECT" --body "$messagefile"

rm "$messagefile"



rm "$logfile"
