CC = g++

OPT = -Wall -Wno-deprecated -g

OBJS = util.o BaseSocket.o EventDispatch.o netlib.o impdu.o

ROUTE_SERV_OBJS = $(OBJS) RouteConn.o route_server.o
IM_SERV_OBJS = $(OBJS) imconn.o im_server.o
IM_CLI_OBJS = $(OBJS) imconn.o im_client.o

ROUTE_SERVER = route_server
IM_SERVER = im_server
IM_CLIENT = im_client

all: $(ROUTE_SERVER) $(IM_SERVER) $(IM_CLIENT)

$(ROUTE_SERVER): $(ROUTE_SERV_OBJS) 
	$(CC) -o $@ $(ROUTE_SERV_OBJS) -lpthread

$(IM_SERVER): $(IM_SERV_OBJS) 
	$(CC) -o $@ $(IM_SERV_OBJS) -lpthread

$(IM_CLIENT): $(IM_CLI_OBJS)
	$(CC) -o $@ $(IM_CLI_OBJS) -lpthread

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

RouteConn.o: RouteConn.cpp
	$(CC) $(OPT) -c -o $@ $<

route_server.o: route_server.cpp
	$(CC) $(OPT) -c -o $@ $<

im_server.o: im_server.cpp
	$(CC) $(OPT) -c -o $@ $<

im_client.o: im_client.cpp
	$(CC) $(OPT) -c -o $@ $<

clean:
	rm -f $(ROUTE_SERVER) $(IM_SERVER) $(IM_CLIENT) *.o

