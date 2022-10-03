
run-writer:
	gcc -std=c17 -Wall writer.c -o writer -lm -lpthread
	./writer $(mode) $(path) $(chunk)

run-reader :
	gcc -std=c17 -Wall reader.c -o reader -lm -lpthread
	./reader $(mode)

end:
	gcc end_files.c -o end_files
	./end_files