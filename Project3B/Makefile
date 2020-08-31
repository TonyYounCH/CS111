#NAME: Changhui Youn
#EMAIL: tonyyoun2@gmail.com
#ID: 304207830

default: lab3b.py
	rm -f lab3b
	ln -s lab3b.py lab3b
	chmod +x lab3b
	echo "Compilation successful"

clean:
	rm -f lab3b *.tar.gz

dist: default
	tar -cvzf lab3b-304207380.tar.gz lab3b.py Makefile README
