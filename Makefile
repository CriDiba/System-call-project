CFLAGS   = -Wall -std=gnu99
INCLUDES = -I .
LIBS     = -lm
OBJDIR   = obj

# keep track of these files
KEEP_TRACK = Makefile

SERVER_SRCS = defines.c err_exit.c shared_memory.c semaphore.c fifo.c server.c
SERVER_OBJS = $(addprefix $(OBJDIR)/, $(SERVER_SRCS:.c=.o))

CLIENT_SRCS = defines.c err_exit.c shared_memory.c fifo.c client.c
CLIENT_OBJS = $(addprefix $(OBJDIR)/, $(CLIENT_SRCS:.c=.o))

# must be first rule
default: all

$(OBJDIR):
	@mkdir -p $(OBJDIR)

%.h:
	@echo "(missing header $@)"

$(OBJDIR)/%.o: %.c %.h $(KEEP_TRACK)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -c -o $@ $<

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -o $@ $^
	@echo "Server compiled."
	@echo

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -o $@ $^
	@echo "Client compiled."
	@echo

run: server
	@./server 100 input/file_posizioni.txt

clean:
	@rm -vf $(SERVER_OBJS) $(CLIENT_OBJS)
	@rm -vf server
	@rm -vf client
	@rm -vf /tmp/dev_fifo.*
	@ipcrm -a
	@echo "Removed object files and executables..."

all: $(OBJDIR) server client

.PHONY: run clean
