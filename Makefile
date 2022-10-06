
run-writer:
	gcc -std=c17 -Wall writer.c -o writer -lm -lpthread
	./writer -i $(image) -n $(chunk) -m $(mode) -k $(key)

run-reader :
	gcc -std=c17 -Wall reader.c -o reader -lm -lpthread
	./reader -m $(mode) -k $(key)

end:
	gcc end_files.c -o end_files
	./end_files