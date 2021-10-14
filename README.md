# sntools

Some network tools for monitoring networks and providing machine information via shell, e-mail or http.


## Tools

### arp-scan-util

Uses `arp-scan` to scan network. Stores IPs, MACs and Hostnames into a local LMDB database

    arp-scan-util -i <interface> [-d <lmdb-directory>] [-v]
    
Parameter     | Description
--------------| -----------
i             | Interface name
d             | Directory path for LMDB database
v             | Enable verbose mode

### arp-history

Show stored data generated by `arp-scan-util`

    arp-history [-t] [-d <lmdb-directory>] [-q <ip or mac>]
    
Parameter     | Description
--------------| -----------
q             | Query (IP or MAC)
d             | Directory path for LMDB database
t             | Table format output
    
### hostwatch

Uses `tcpdump` to monitor traffic. Counts bytes transferred between two hosts and shows a sorted table

    hostwatch [-x] [-v] [-m <port>]  [-n <hosts-to-print>] [-s <ip>] [-p <port>] [-h] -i <interface>
    
Parameter     | Description
--------------| -----------
i             | Interface name
v             | Enable verbose mode (repeat v for more output)
m             | Port number: Strip ports above given number
x             | Ignore direction of communication between two hosts
n             | Number of hosts to be shown
h             | Show number in human readable format
s             | Run in server mode and bind to ip
p             | Port to bind server to (default: 8002)

### exec-and-mail

Execute a command and send ouput via smtp after given number of lines or after timeout

    exec-and-mail [-c <buffer-line-count>] [-t <wait-timeout-seconds>] [-v] [-s <subject>] <host> <port> <from> <to> "<command>"
    
Parameter     | Description
--------------| -----------
c             | How may lines from command to read before sending an e-mail
t             | How long to wait (in seconds) before sending an e-mail it there is at least one line
v             | Enable verbose mode (repeat v for more output)
s             | E-mail subject
    
    
### httpd-exec

Execute a command every time a client connects and send output to client (dangerous, use with care as any command can be used).

    httpd-exec [-c <content-type] [-p <port>] "<command>"
    
Parameter     | Description
--------------| -----------
c             | Response content-type (default is "text/plain")
p             | Port to bind to (default is 8001)
v             | Enable verbose mode

### snort-scan-util

Uses `snort` to monitor networks. 

    snort-scan-util [-s] [-v] -h <home network> -i <interface>
    
Parameter     | Description
--------------| -----------
i             | Interface name
h             | Home network
s             | Strip port
v             | Enable verbose mode


## Bash scripts

### arp-scan-and-mail-changes.sh

Scan network with `arp-scan-util` and send an e-mail only if changes where detected (based upan data from local LMDB database)

### arp-history-mail.sh

Send all known MACs with IPs via e-mail. Uses `arp-history` to read data from local LMDB database.


## Requirements

`arp-scan` (https://github.com/royhills/arp-scan),
available on Debian/Ubuntu via `apt install arp-scan`

Bash scripts are using `swaks` (https://github.com/jetmore/swaks) for sending e-mails,
available on Debian/Ubuntu via `apt install swaks`

`tcpdump` should be available wothput installation on Debian/Ubuntu
