OBJS = main.o service.o

all: tuyaled_a tuyaled_cmd

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

tuyaled_a: $(OBJS)
	$(CC) -o $@ $(OBJS)

tuyaled_cmd: tuyaled_cmd.o
	$(CC) -o $@ tuyaled_cmd.o

clean:
	rm -f tuyaled_a *.o
