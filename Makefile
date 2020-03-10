CC = gcc# compiler
CFLAGS = -pthread -Wall -g# compile flags
LIBS = -lpthread -lrt# libs

all: mapper reducer

# all: mapper

reducer: reducer.o
	$(CC) -o reducer reducer.o $(LIBS)

# mapper: mapper.o
# 	$(CC) -o mapper mapper.o $(LIBS)

	
clean:
	rm -f reducer mapper *.o *~
	
# clean:
# 	rm -f mapper *.o *~

%.o:%.c
	$(CC) $(CFLAGS) -c $*.c
