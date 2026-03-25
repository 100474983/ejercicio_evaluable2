CC = gcc
CFLAGS = -Wall -Wextra -pthread -fPIC
LDFLAGS = -pthread

LIB_LOCAL = libclaves.so
LIB_LOCAL_SRC = claves.c
LIB_LOCAL_OBJ = claves.o

LIB_PROXY = libproxyclaves.so
LIB_PROXY_SRC = proxy-sock.c
LIB_PROXY_OBJ = proxy-sock.o

LINES_SRC = lines.c
LINES_OBJ = lines.o

CLIENT_LOCAL = app-cliente-local
CLIENT_SRC = app-cliente.c
CLIENT_OBJ = app-cliente.o

CLIENT_DIST = app-cliente-dist

SERVER = servidor-sock
SERVER_SRC = servidor-sock.c
SERVER_OBJ = servidor-sock.o

all: $(LIB_LOCAL) $(LIB_PROXY) $(CLIENT_LOCAL) $(CLIENT_DIST) $(SERVER)

$(LIB_LOCAL): $(LIB_LOCAL_OBJ)
	$(CC) -shared -o $(LIB_LOCAL) $(LIB_LOCAL_OBJ)

$(LIB_LOCAL_OBJ): $(LIB_LOCAL_SRC)
	$(CC) $(CFLAGS) -c $(LIB_LOCAL_SRC)

$(LIB_PROXY): $(LIB_PROXY_OBJ) $(LINES_OBJ)
	$(CC) -shared -o $(LIB_PROXY) $(LIB_PROXY_OBJ) $(LINES_OBJ) $(LDFLAGS)

$(LIB_PROXY_OBJ): $(LIB_PROXY_SRC)
	$(CC) $(CFLAGS) -c $(LIB_PROXY_SRC)

$(LINES_OBJ): $(LINES_SRC)
	$(CC) $(CFLAGS) -c $(LINES_SRC)

$(CLIENT_LOCAL): $(CLIENT_OBJ)
	$(CC) -o $(CLIENT_LOCAL) $(CLIENT_OBJ) -L. -lclaves $(LDFLAGS)

$(CLIENT_DIST): $(CLIENT_OBJ)
	$(CC) -o $(CLIENT_DIST) $(CLIENT_OBJ) -L. -lproxyclaves $(LDFLAGS)

$(CLIENT_OBJ): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC)

$(SERVER): $(SERVER_OBJ) $(LINES_OBJ)
	$(CC) -o $(SERVER) $(SERVER_OBJ) $(LINES_OBJ) -L. -lclaves $(LDFLAGS)

$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $(SERVER_SRC)

clean:
	rm -f *.o *.so $(CLIENT_LOCAL) $(CLIENT_DIST) $(SERVER)
