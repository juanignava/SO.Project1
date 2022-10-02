
run-writer:
	gcc -std=c17 -Wall writer.c -o writer -lm -lpthread
	./writer $(mode)

run-reader :
	gcc -std=c17 -Wall reader.c -o reader -lm -lpthread
	./reader $(mode)

run-test :
	gcc -std=c17 -Wall test.c -o test -lm -lpthread
	./test 
