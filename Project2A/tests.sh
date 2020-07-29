#!/bin/bash

# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

rm -f lab2_add.csv
rm -f lab2_list.csv
touch lab2_add.csv
touch lab2_list.csv

#add-none test
for i in 1, 2, 4, 8, 12
do
    for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
    do
        ./lab2_add --iterations=$j --threads=$i >> lab2_add.csv
    done
done

#add-yield-none test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --yield >> lab2_add.csv
        done
done

#add-m test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --sync=m >> lab2_add.csv
        done
done

#add-s test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --sync=s >> lab2_add.csv
        done
done

#add-c test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --sync=c >> lab2_add.csv
        done
done

#add-yield-m test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --yield --sync=m >> lab2_add.csv
        done
done

#add-yield-s test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --yield --sync=s >> lab2_add.csv
        done
done

#add-yield-c test
for i in 1, 2, 4, 8, 12
do
        for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
        do
                ./lab2_add --iterations=$j --threads=$i --yield --sync=c >> lab2_add.csv
        done
done



# list-none with single thread
for j in 10, 100, 1000, 10000, 20000
do
	./lab2_list --threads=1 --iterations=$j >> lab2_list.csv
done

# list-none with more then 1 thread
for i in 2, 4, 8, 12
do
	for j in 1, 10, 100, 1000
	do
		./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
	done
done

# list-i-none
for i in 2, 4, 8, 12
do
	for j in 1, 2, 4, 8, 16, 32
	do
		./lab2_list --threads=$i --iterations=$j --yield=i >> lab2_list.csv
	done
done

# list-d-none
for i in 2, 4, 8, 12
do
	for j in 1, 2, 4, 8, 16, 32
	do
		./lab2_list --threads=$i --iterations=$j --yield=d >> lab2_list.csv
	done
done

# list-il-none
for i in 2, 4, 8, 12
do
	for j in 1, 2, 4, 8, 16, 32
	do
		./lab2_list --threads=$i --iterations=$j --yield=il >> lab2_list.csv
	done
done

# list-dl-none
for i in 2, 4, 8, 12
do
	for j in 1, 2, 4, 8, 16, 32
	do
		./lab2_list --threads=$i --iterations=$j --yield=dl >> lab2_list.csv
	done
done

# list-[idl]-[ms]
./lab2_list --threads=12 --iterations=32 --yield=i --sync=m >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=d --sync=m >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=il --sync=m >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=dl --sync=m >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=i --sync=s >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=d --sync=s >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=il --sync=s >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=dl --sync=s >> lab2_list.csv

for i in 1, 2, 4, 8, 12, 16, 24
do
	./lab2_list --threads=$i --iterations=1000 >> lab2_list.csv
done

for i in 1, 2, 4, 8, 12, 16, 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2_list.csv
done

for i in 1, 2, 4, 8, 12, 16, 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2_list.csv
done

