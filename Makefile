
run-writer:
	gcc writer.c -o writer -lpthread
	./writer $(mode)

run-reader :
	gcc -g reader.c -o reader -lpthread
	./reader $(mode)
