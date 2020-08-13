# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

UNAME := $(shell uname -a)
ifeq ($(findstring beaglebone,$(UNAME)),beaglebone)
	# Found
	COMP := gcc -g -lm -lmraa -Wall -Wextra lab4b.c -o lab4b
else
	# Not found
	COMP := gcc -g -lm -Wall -Wextra -DDUMMY lab4b.c -o lab4b
endif

default:
	$(COMP)

check: default
	chmod +x tests.sh
	./tests.sh

dist: check
	tar -czf lab4b-304207830.tar.gz lab4b.c Makefile README tests.sh

clean:
	rm -f *.o lab4b *.txt *.tar.gz
