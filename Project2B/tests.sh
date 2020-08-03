#!/bin/bash

# NAME: Changhui Youn
# EMAIL: tonyyoun2@gmail.com
# ID: 304207830

rm -f lab2b_list.csv
touch lab2b_list.csv

# list-none-m for 1000
for i in 1, 2, 4, 8, 12, 16, 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv
done

# list-none-s for 1000
for i in 1, 2, 4, 8, 12, 16, 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
done


# list-id-none
for i in 1, 4, 8, 12, 16
do
	for j in 1, 2, 4, 8, 16
	do
		./lab2_list --threads=$i --iterations=$j --yield=id --lists=4 >> lab2b_list.csv
	done
done


# list-id-m
for i in 1, 4, 8, 12, 16
do
	for j in 10, 20, 40, 80
	do
		./lab2_list --threads=$i --iterations=$j --yield=id --sync=m --lists=4 >> lab2b_list.csv
	done
done

# list-id-s
for i in 1, 4, 8, 12, 16
do
	for j in 10, 20, 40, 80
	do
		./lab2_list --threads=$i --iterations=$j --yield=id --sync=s --lists=4 >> lab2b_list.csv
	done
done


# list-none-m
for i in 1, 2, 4, 8, 12
do
	for j in 1, 4, 8, 16
	do
		./lab2_list --threads=$i --iterations=1000 --sync=m --lists=$j >> lab2b_list.csv
	done
done

# list-none-s
for i in 1, 2, 4, 8, 12
do
	for j in 1, 4, 8, 16
	do
		./lab2_list --threads=$i --iterations=1000 --sync=s --lists=$j >> lab2b_list.csv
	done
done
