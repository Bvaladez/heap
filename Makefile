CC = gcc
FLAGS = -Wall -Werror
TARGET = heap
OBJECTS = main.o

make: $(OBJECTS)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJECTS)

clean:
	rm -f ./*.o
	rm -f $(TARGET)
