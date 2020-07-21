# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

default:
	/usr/local/cs/bin/gcc -g -Wall -Wextra lab1b-client.c -lz -o lab1b-client
	/usr/local/cs/bin/gcc -g -Wall -Wextra lab1b-server.c -lz -o lab1b-server

client:
	/usr/local/cs/bin/gcc -g -Wall -Wextra lab1b-client.c -lz -o lab1b-client

server: 
	/usr/local/cs/bin/gcc -g -Wall -Wextra lab1b-server.c -lz -o lab1b-server

clean:
	rm -f *.o lab1b-client lab1b-server *.txt *.tar.gz

dist: default
	tar -czf lab1b-304207830.tar.gz lab1b-client.c lab1b-server.c Makefile README
