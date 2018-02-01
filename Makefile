CFLAGS = -Wall -g
CC     = gcc $(CFLAGS)

all : bl-server bl-client shell-tests

bl-server : server.c bl-server.c
	$(CC) -o $@ $^

bl-client : simpio.c bl-client.c
	$(CC) -o $@ $^
      
shell-tests : shell_tests.sh shell_tests_data.sh cat-sig.sh clean-tests
	chmod u+rx shell_tests.sh shell_tests_data.sh normalize.awk filter-semopen-bug.awk cat-sig.sh 
	./shell_tests.sh

clean-tests :
	rm -f test-*.log test-*.out test-*.expect test-*.diff test-*.valgrindout      

clean :
	rm -f bl-client bl-server *.fifo

