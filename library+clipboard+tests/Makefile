CC=gcc 
FLAGS=-g -Wall 

all: clipboard app_1thread app_2processes app_3massive

clipboard: clipboard.o library.o
	$(CC) -pthread -o $@ $^ 

clipboard.o: clipboard.c clipboard.h topsecret.h
	$(CC) -c $^ $(FLAGS) 

app_1thread: app_1thread.o library.o
	$(CC) -o $@ $^ 

app_1thread.o: app_1thread.c clipboard.h
	$(CC) -c $^ $(FLAGS) 

app_2processes: app_2processes.o library.o
	$(CC) -o $@ $^ 

app_2processes.o: app_2processes.c clipboard.h
	$(CC) -c $^ $(FLAGS) 

app_3massive: app_3massive.o library.o
	$(CC) -o $@ $^ 

app_3massive.o: app_3massive.c clipboard.h
	$(CC) -c $^ $(FLAGS)

library.o: library.c clipboard.h topsecret.h
	$(CC) -c $^ $(FLAGS) 
	
clean:
	rm -f *.*o *.h.gch all
