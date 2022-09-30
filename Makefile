
run-writer:
	gcc writer.c -o writer -lpthread
	./writer $(method)

run-reader :
	gcc -g reader.c -o reader -lpthread
	./reader $(method)