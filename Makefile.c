main : main.o dir_function.o lsh_function.o
	gcc main.o lsh_function.o -o main

main.o : main.c dir-function.c lsh_function.c dir_function.h lsh_function.h
	gcc -Wall -c main.c

dir_function.o : dir_function.c lsh_function.c dir_function.h lsh_function.h
	gcc -Wall -c dir_function.c

lsh-function.o : lsh_function.c lsh_function.h
	gcc -Wall -c lsh_function.c

clean :
	rm *.o main.exe
