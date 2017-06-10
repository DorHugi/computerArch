CC = gcc
CFLAGS = -std=c99 -g -Wall
CCLINK = $(CC)
OBJS = main.o cache.o 
RM = rm -f
# Creating the  executable
cacheSim: $(OBJS)
	$(CCLINK) $(CFLAGS) -o cacheSim $(OBJS) -lm
# Creating the object files
main.o: main.c cache.h
	$(CC) $(CFLAGS) -c main.c
cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c
# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ "#"* cacheSim*
