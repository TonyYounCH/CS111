# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

build:
	gcc -g -Wall -Wextra -lpthread lab2_list.c SortedList.c -o lab2_list

tests: build
	chmod +x tests.sh
	./tests.sh

profile:
	rm -f ./raw.gperf profile.out
	LD_PRELOAD=/usr/lib64/libprofiler.so \
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s 
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=thread_worker ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

graphs: tests
	chmod +x lab2_list.gp
	./lab2_list.gp

dist: graphs profile
	tar -czf lab2b-304207830.tar.gz lab2_list.c SortedList.h \
	SortedList.c lab2b_list.csv profile.out lab2b_1.png \
	lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png \
	lab2_list.gp Makefile README tests.sh

clean:
	rm -f *.o lab2_list *.txt *.tar.gz *.csv
