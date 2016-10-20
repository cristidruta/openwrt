#WEB_PRO_DIR=webproto

all:fxc

OBJS=main.o iotc_log.o listener.o parser.o cJSON.o proto.o socket.o session.o glue.o

# TODO: support full http protocol
#OBJS+=$(WEB_PRO_DIR)/protocol.o $(WEB_PRO_DIR)/wget.o $(WEB_PRO_DIR)/www.o


CFLAGS+=-lubox -lm

fxc:$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf fxc

.PHONY: all clean
