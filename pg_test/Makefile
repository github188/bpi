CC=gcc
MAKE=make

#编译选项
INCLUDE=-I/usr/include/postgresql
LIB= -lpthread -lm -lpq

SOURCE=pool.c filter.c logs.c main.c monitor.c utils.c
DEPEND=pool.o filter.o logs.o main.o monitor.o utils.o
TARGET=bpi.out

all:clean out mv
	@ctags -R *
	@echo "-- ok!-- "

gdb:clean gdb_out mv
	@ctags -R *
	@echo "-- ok!-- "

out:
	@$(CC) -c $(SOURCE) $(INCLUDE)
	@$(CC) $(DEPEND) -o $(TARGET) $(LIB)

gdb_out:
	@$(CC) -g -O0 -c $(SOURCE) $(INCLUDE)
	@$(CC) -g -O0 $(DEPEND) -o $(TARGET) $(LIB)

mv:
	-@rm -f *.o

clean:
	-@rm -f *.out *.o
