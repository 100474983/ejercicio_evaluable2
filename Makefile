CC = gcc
CFLAGS = -Wall -Wextra -pthread -fPIC
LDFLAGS = -pthread

# Biblioteca del servidor 
LIB_LOCAL = libclaves.so
LIB_LOCAL_SRC = claves.c
LIB_LOCAL_OBJ = claves.o

# Biblioteca del lado cliente (proxy TCP)
LIB_PROXY = libproxyclaves.so
LIB_PROXY_SRC = proxy-sock.c
LIB_PROXY_OBJ = proxy-sock.o

# Utilidades de comunicación por socket
LINES_SRC = lines.c
LINES_OBJ = lines.o

# Cliente de prueba funcional (app-cliente.c)
CLIENT_DIST = app-cliente-dist
CLIENT_SRC = app-cliente.c
CLIENT_OBJ = app-cliente.o

# Cliente de prueba de concurrencia (app-cliente2.c)
CLIENT2 = app-cliente-dist2
CLIENT2_SRC = app-cliente2.c
CLIENT2_OBJ = app-cliente2.o

# Servidor TCP
SERVER = servidor-sock
SERVER_SRC = servidor-sock.c
SERVER_OBJ = servidor-sock.o

.PHONY: all clean

all: $(LIB_LOCAL) $(LIB_PROXY) $(CLIENT_DIST) $(CLIENT2) $(SERVER)

# Biblioteca libclaves.so
$(LIB_LOCAL): $(LIB_LOCAL_OBJ)
	$(CC) -shared -o $(LIB_LOCAL) $(LIB_LOCAL_OBJ) $(LDFLAGS)

$(LIB_LOCAL_OBJ): $(LIB_LOCAL_SRC) claves.h
	$(CC) $(CFLAGS) -c $(LIB_LOCAL_SRC)

# Biblioteca libproxyclaves.so
$(LIB_PROXY): $(LIB_PROXY_OBJ) $(LINES_OBJ)
	$(CC) -shared -o $(LIB_PROXY) $(LIB_PROXY_OBJ) $(LINES_OBJ) $(LDFLAGS)

$(LIB_PROXY_OBJ): $(LIB_PROXY_SRC) claves.h lines.h
	$(CC) $(CFLAGS) -c $(LIB_PROXY_SRC)

$(LINES_OBJ): $(LINES_SRC) lines.h
	$(CC) $(CFLAGS) -c $(LINES_SRC)

# Ejecutable cliente funcional
$(CLIENT_DIST): $(CLIENT_OBJ) $(LIB_PROXY)
	$(CC) -o $(CLIENT_DIST) $(CLIENT_OBJ) -L. -lproxyclaves $(LDFLAGS)

$(CLIENT_OBJ): $(CLIENT_SRC) claves.h
	$(CC) $(CFLAGS) -c $(CLIENT_SRC)

# Ejecutable cliente concurrente
$(CLIENT2): $(CLIENT2_OBJ) $(LIB_PROXY)
	$(CC) -o $(CLIENT2) $(CLIENT2_OBJ) -L. -lproxyclaves $(LDFLAGS)

$(CLIENT2_OBJ): $(CLIENT2_SRC) claves.h
	$(CC) $(CFLAGS) -c $(CLIENT2_SRC)

# Ejecutable servidor
$(SERVER): $(SERVER_OBJ) $(LINES_OBJ) $(LIB_LOCAL)
	$(CC) -o $(SERVER) $(SERVER_OBJ) $(LINES_OBJ) -L. -lclaves $(LDFLAGS)

$(SERVER_OBJ): $(SERVER_SRC) claves.h lines.h
	$(CC) $(CFLAGS) -c $(SERVER_SRC)

# Limpieza 
clean:
	rm -f *.o *.so $(CLIENT_DIST) $(CLIENT2) $(SERVER)