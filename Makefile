bin=crash

# Set the following to '0' to disable log messages:
debug=1

CFLAGS += -Wall -g
LDFLAGS +=

src=history.c shell.c str_func.c timer.c cmd_handling.c built_in.c
obj=$(src:.c=.o)

$(bin): $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -DDEBUG=$(debug) $(obj) -o $@

shell.o: shell.c history.h timer.h str_func.h built_in.h
history.o: history.c history.h cmd_handling.h built_in.h
timer.o: timer.c timer.h
str_func.o: str_func.c str_func.h
cmd_handling.o: cmd_handling.c cmd_handling.h str_func.h
built_in.o: built_in.c built_in.h cmd_handling.h history.h

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
