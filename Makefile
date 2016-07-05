CC=gcc
MAKE=make

#编译选项
LIB=-lpthread

SOURCE=filter.c logs.c main.c monitor.c send.c utils.c
DEPEND=filter.o logs.o main.o monitor.o send.o utils.o
TARGET=bpi.out

all:clean out mv
	@ctags -R *
	@echo "-- ok!-- "

gdb:clean gdb_out mv
	@ctags -R *
	@echo "-- ok!-- "

out:
	@$(CC) -c $(SOURCE)
	@$(CC) $(DEPEND) -o $(TARGET) $(LIB)

gdb_out:
	@$(CC) -g -O0 -c $(SOURCE)
	@$(CC) -g -O0 $(DEPEND) -o $(TARGET) $(LIB)

mv:
	-@rm -f *.o

clean:
	-@rm -f *.out *.o
