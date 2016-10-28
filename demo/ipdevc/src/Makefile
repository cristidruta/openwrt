EXEC=ipdevc
SRC=main.c msg.c msgHandle.c

all: $(EXEC)
$(EXEC): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS) -lcurl

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

