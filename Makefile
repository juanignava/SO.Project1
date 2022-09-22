run-writer:
	gcc writer.c -o writer -lpthread
	./writer

run-reader:
	gcc -g reader.c utils.c sem.c -o reader -lpthread
	./reader