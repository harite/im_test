CC = g++

OPT = -Wall -Wno-deprecated -g

OBJS = util.o BaseSocket.o EventDispatch.o netlib.o imconn.o impdu.o

SERV_OBJS = $(OBJS) im_server.o
CLI_OBJS = $(OBJS) im_client.o
	
SERVER = im_server
CLIENT = im_client

all: $(CLIENT) $(SERVER)

$(SERVER): $(SERV_OBJS) 
	$(CC) -lpthread -o $@ $(SERV_OBJS)

$(CLIENT): $(CLI_OBJS)
	$(CC) -lpthread -o $@ $(CLI_OBJS)

util.o: util.cpp
	$(CC) $(OPT) -c -o $@ $<

BaseSocket.o: BaseSocket.cpp
	$(CC) $(OPT) -c -o $@ $<

EventDispatch.o: EventDispatch.cpp
	$(CC) $(OPT) -c -o $@ $<

netlib.o: netlib.cpp 
	$(CC) $(OPT) -c -o $@ $<

imconn.o: imconn.cpp
	$(CC) $(OPT) -c -o $@ $<

impdu.o: impdu.cpp
	$(CC) $(OPT) -c -o $@ $<

im_server.o: im_server.cpp
	$(CC) $(OPT) -c -o $@ $<

im_client.o: im_client.cpp
	$(CC) $(OPT) -c -o $@ $<

clean:
	rm -f $(SERV_OBJS) $(SERVER) $(CLI_OBJS) $(CLIENT)

