default :
	gcc -g -Wall -Wextra lab0.c -o lab0

clean :
	rm -f *.o lab0 *.txt *.tar.gz

dist : default
	tar -czf lab0-304207830.tar.gz lab0.c Makefile *.png README

check : default inputfileTest invalidArgs catchSegFault
	cat resultFile.txt;\
	rm -f *.txt;\

inputfileTest : createFile correctFilename wrongFilename permission noPermission

createFile : 
	echo "Test File" > input.txt; \
	echo "Test Result" > resultFile.txt; \


correctFilename :
	./lab0 --input=input.txt --output=output.txt &> /dev/null;\
	if [[ $$? -eq 0 ]];\
	then\
		echo "correctFilename : Program successfully exited with 0" >> resultFile.txt;\
	else\
		echo "correctFilename : Program exited with an error" >> resultFile.txt;\
	fi\

wrongFilename :
	./lab0 --input=inputt.txt --output=output.txt &> /dev/null;\
	if [[ $$? -eq 2 ]];\
	then\
		echo "wrongFilename : Program ran with wrong filename. Program exited with 2" >> resultFile.txt;\
	else\
		echo "wrongFilename : Program exited with an error" >> resultFile.txt;\
	fi\

permission : 
	chmod 111 input.txt;\

noPermission :
	./lab0 --input=input.txt --output=output.txt &> /dev/null;\
	if [[ $$? -eq 2 ]];\
	then\
		echo "noPermission : No permssion to the file. Program exited with 2" >> resultFile.txt;\
		chmod 666 input.txt;\
	else\
		echo "noPermission : Program exited with an error" >> resultFile.txt;\
		chmod 666 input.txt;\
	fi\

invalidArgs : 
	./lab0 --invalid &> /dev/null;\
	if [[ $$? -eq 1 ]]; then \
	echo "invalidArgs : Unrecognized arguement. Program exited with 1" >> resultFile.txt; \
	else \
	echo "invalidArgs : Program exited with an error" >> resultFile.txt; \
	fi\


catchSegFault:
	echo | ./lab0 --segfault --catch &> /dev/null; \
	if [[ $$? -eq 4 ]]; then \
	echo "catchSegFault : SEGFAULT caught! Program exited with 4" >> resultFile.txt; \
	else \
	echo "catchSegFault : Program exited with an error" >> resultFile.txt; \
	fi