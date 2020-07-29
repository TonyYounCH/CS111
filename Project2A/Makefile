# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

build:
	gcc -g -Wall -Wextra -lpthread lab2_add.c -o lab2_add
	gcc -g -Wall -Wextra -lpthread lab2_list.c SortedList.c -o lab2_list

tests: build
	chmod +x tests.sh
	./tests.sh

graphs: tests
	./lab2_add.gp
	./lab2_list.gp

dist: graphs
	tar -czf lab2a-304207830.tar.gz lab2_add.c lab2_add.csv lab2_add-1.png \
	lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png \
	lab2_list.c SortedList.h SortedList.c lab2_list.csv lab2_list-1.png \
	lab2_list-2.png lab2_list-3.png lab2_list-4.png \
	lab2_add.gp lab2_list.gp Makefile README tests.sh

clean:
	rm -f *.o lab2_add lab2_list *.txt *.tar.gz
