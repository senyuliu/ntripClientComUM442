TARGET=ntripClientDemo
CC=gcc 

SOURCE += main.c\
          ntripclient.c   
		 
FLAGS= -g -Wall 
all:
	$(CC) $(FLAGS) -o  $(TARGET)  $(SOURCE) -lpthread

clean:
	rm -rf *.o ntripClientDemo
