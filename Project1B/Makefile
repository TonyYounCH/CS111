# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

default:
	gcc -Wall -Wextra -lz -o lab1b-client lab1b-client.c
	gcc -Wall -Wextra -lz -o lab1b-server lab1b-server.c

client:
	gcc -Wall -Wextra -lz -o lab1b-client lab1b-client.c

server: 
	gcc -Wall -Wextra -lz -o lab1b-server lab1b-server.c

clean:
	rm -f *.o lab1b-client lab1b-server *.txt *.tar.gz

dist: default
	tar -czf lab1b-304207830.tar.gz lab1b-client.c lab1b-server.c Makefile README
