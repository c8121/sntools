# sntools

Some network tools (actually very few at the moment)



## Requirements

`arp-scan` (https://github.com/royhills/arp-scan),
available on Debian/Ubuntu via `apt install arp-scan`

Bash scripts are using `swaks` (https://github.com/jetmore/swaks) for sending e-mails,
available on Debian/Ubuntu via `apt install swaks`




## Tools

### arp-scan-util

Uses `arp-scan` to scan network. Stores IPs, MACs and Hostnames into a local LMDB database

    arp-scan-util -i <interface> [-d <lmdb-directory>] [-v]

### arp-history

Show stored data generated by `arp-scan-util`

    arp-history [-t] [-d <lmdb-directory>] [-q <ip or mac>]

### exec-and-mail

Execute a command and send ouput via smtp after given number of lines or after timeout

    exec-and-mail [-c buffer-line-count] [-t wait-timeout-seconds] [-v] [-s subject] <host> <port> <from> <to> "<command>"


## Bash scripts

### arp-scan-and-mail-changes.sh

Scan network with `arp-scan-util` and send an e-mail only if changes where detected (based upan data from local LMDB database)

### arp-history-mail.sh

Send all known MACs with IPs via e-mail. Uses `arp-history` to read data from local LMDB database.
