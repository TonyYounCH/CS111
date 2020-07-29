#!/bin/bash

# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

rm -f lab2_add.csv
rm -f lab2_list.csv
touch lab2_add.csv
touch lab2_list.csv

# add-none
for i in 2, 4, 8, 12
do
	for j in 100, 1000, 10000, 100000
	do
		./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
	done
done

# add-yield-none
for i in 2, 4, 8, 12
do
	for j in 10, 20, 40, 80, 100, 1000, 10000, 100000
	do
		./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
	done
done

# compare the average execution time
for i in 2, 8
do
	for j in 100, 1000, 10000, 100000
	do
		./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
	done
done
for i in 2, 8
do
	for j in 100, 1000, 10000, 100000
	do
		./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
	done
done

# add-yield-m
for i in 2, 4, 8, 12
do
	./lab2_add --threads=$i --iterations=10000 --yield --sync=m >> lab2_add.csv
done

# add-yield-c
for i in 2, 4, 8, 12
do
	./lab2_add --threads=$i --iterations=10000 --yield --sync=c >> lab2_add.csv
done

# add-yield-s
for i in 2, 4, 8
do
	./lab2_add --threads=$i --iterations=1000 --yield --sync=s >> lab2_add.csv
done

# add-m
for i in 1, 2, 4, 8, 12
do
	./lab2_add --threads=$i --iterations=10000 --sync=m >> lab2_add.csv
done

# add-c
for i in 1, 2, 4, 8, 12
do
	./lab2_add --threads=$i --iterations=10000 --sync=c >> lab2_add.csv
done

# add-s
for i in 1, 2, 4, 8, 12
do
	./lab2_add --threads=$i --iterations=10000 --sync=s >> lab2_add.csv
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

