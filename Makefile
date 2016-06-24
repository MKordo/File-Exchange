OBJS1    = server.o s_functions.o queue.o
OBJS2    = client.o
SOURCE1  = server.c s_functions.c queue.c
SOURCE2  = client.c
OUT1     = dataServer
OUT2     = remoteClient
CC       = gcc
FLAGS    = -c

all: $(OUT1) $(OUT2)

$(OUT1): $(OBJS1)
	$(CC) $(OBJS1) -lpthread -o $(OUT1)

$(OUT2): $(OBJS2)
	$(CC) $(OBJS2) -o $(OUT2)

server.o: server.c
	$(CC) $(FLAGS) server.c

s_functions.o: s_functions.c
	$(CC) $(FLAGS) s_functions.c

queue.o: queue.c
	$(CC) $(FLAGS) queue.c

client.o: client.c
	$(CC) $(FLAGS) client.c

clean:
	rm -f $(OBJS1) $(OUT1) $(OBJS2) $(OUT2)
