CFALGS := -g

all:smart
smart:main.o logSmart.o listener.o parser.o 
	$(CC) $(LDFALGS)  $^ -o $@ -luci -lubus -lubox -lblobmsg_json -ljson-c 

%.o:%.c
	$(CC) $(CFALGS) -O2 -c $<

clean: 
	rm -rf smart

.PHONY: all clean
