STUDENT_ID=3114086

quash:
	gcc -g src/quash.c -o quash 

clean:
	rm -f quash

run:
	./quash

valgrind:
	rm -f quash
	gcc -Wall -g src/quash.c -o quash
	valgrind --leak-check=full ./quash

all:
	rm -f quash
	gcc -Wall -g src/quash.c -o quash
	./quash

tar:
	make clean
	mkdir $(STUDENT_ID)-quash
	cp -r Makefile quash.c $(STUDENT_ID)-quash
	tar cvzf $(STUDENT_ID)-ipc-lab.tar.gz $(STUDENT_ID)-quash
	rm -rf $(STUDENT_ID)-quash