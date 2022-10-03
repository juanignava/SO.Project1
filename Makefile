
run-writer:
	gcc -std=c17 -Wall writer.c -o writer -lm -lpthread
	./writer -i $(image) -n $(chunk) -m $(mode) -k $(key)

run-reader :
	gcc -std=c17 -Wall reader.c -o reader -lm -lpthread
	./reader -m $(mode) -k $(key)

run-test :
	gcc -std=c17 -Wall test.c -o test -lm -lpthread
	./test 
