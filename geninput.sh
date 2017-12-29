#!/bin/sh

LOGFILE="$1"

cat $LOGFILE | sed -e 's/^[a-zA-Z]\{3\} [0-9]* [0-9]*:[0-9]*:[0-9]*//g' | sed -e 's/^ *//g' > rawlog.txt

grep -v -e "^*" rawlog.txt | sed -e '/^\s*$/d' | grep "^<" |\
			grep -v -e "^<schizoHAL>" |\
			sed -e 's/<.*>\s*//g' | \
			grep -v "http://" | grep -v "https://" > say.txt

grep -e "^*\s" rawlog.txt | grep -v -e "^*\sNow talking on" | \
			grep -v -e "^*\sTopic for" | \
			grep -v -e "*\sChannel" | \
			grep -v -e "has joined$" | \
			grep -v "has quit" | grep -v "has left" | \
			grep -v -e "^*\sDisconnected" | \
			grep -v "has changed the topic to" | \
			grep -v "now known as" | \
			grep -v -i -e "^*\schanserv" | \
			grep -v -i -e "^*\sschizohal" | \
			grep -v "freenode-info" | \
			grep -v "Ping reply" | \
			grep -v -e "^*\s\[goliath\]" | \
			grep -v -e "^*\sOffering" | \
			grep -v -e "^*\sDCC" | \
			grep -v -e "^*\sReceived" |\
			grep -v -e "^*\sUsers" |\
			grep -v "has kicked" | grep -v "have been kicked" |\
			grep -v "http://" | grep -v "https://" | \
			grep -v "gives channel operator status" |
			grep -v "sets ban on" | \
			sed -e 's/^\*\s[a-zA-Z0-9_|-]*\s/\/me /g' > self.txt

