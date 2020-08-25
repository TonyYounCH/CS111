# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

UNAME := $(shell uname -a)
ifeq ($(findstring beaglebone,$(UNAME)),beaglebone)
	# Found
	TCP := gcc -g -lm -lmraa -Wall -Wextra lab4c_tcp.c -o lab4c_tcp
	TLS := gcc -g -lm -lmraa -Wall -Wextra -lcrypto -lssl lab4c_tls.c -o lab4c_tls
else
	# Not found
	TCP := gcc -g -lm -Wall -Wextra -DDUMMY lab4c_tcp.c -o lab4c_tcp
	TLS := gcc -g -lm -Wall -Wextra -DDUMMY -lcrypto -lssl lab4c_tls.c -o lab4c_tls
endif


default:
	$(TCP)
	$(TLS)

dist: default
	tar -cvzf lab4c-304207830.tar.gz lab4c_tls.c lab4c_tcp.c README Makefile

clean:
	rm -rf *.o lab4c_tcp lab4c_tls *.txt *.tar.gz
