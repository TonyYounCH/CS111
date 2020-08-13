#!/bin/bash

# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

{ sleep 1; echo "SCALE=C"; sleep 2; echo "STOP"; sleep 2;echo "START"; sleep 2; echo "SCALE=F"; sleep 2; echo "PERIOD=3"; echo "LOG123"; sleep 3;  echo "OFF"; } | ./lab4b --log=log.txt

if [ $? -ne 0 ]
then
	echo "Program exited with error"
else
	echo "Program ran successfully and exited with exit code 0"
fi

for i in SCALE START STOP PERIOD LOG OFF SHUTDOWN
	do
		grep "$i" log.txt > /dev/null
		if [ $? -ne 0 ]
		then
			echo "$i command did not work or not logged"
		else
			echo "$i command worked and was logged successfully!"
		fi
	done

rm -f log.txt
