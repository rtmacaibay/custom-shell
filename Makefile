bin=crash

# Set the following to '0' to disable log messages:
debug=1

CFLAGS += -Wall -g
LDFLAGS +=

src=history.c shell.c timer.c
obj=$(src:.c=.o)

$(bin): $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -DDEBUG=$(debug) $(obj) -o $@

shell.o: shell.c history.h timer.h
history.o: history.c history.h
timer.o: timer.c timer.h

clean:
	rm -f $(bin) $(obj)


# Tests --

test: $(bin) ./tests/run_tests
	./tests/run_tests $(run)

testupdate: testclean test

./tests/run_tests:
	git submodule update --init --remote

testclean:
	rm -rf tests
