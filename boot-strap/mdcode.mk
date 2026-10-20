#line 116 "../mdcode.mdc"
CFLAGS += -Wall -g
all::
mdcode.h libmdcode.c md2c.c mdcode.mk :  mdcode.mdc
	./md2c mdcode.mdc
#line 147 "../mdcode.mdc"
all :: libmdcode.o
libmdcode.o : libmdcode.c mdcode.h
	$(CC) $(CFLAGS) -c libmdcode.c
#line 167 "../mdcode.mdc"
all :: md2c
md2c : md2c.o libmdcode.o
	$(CC) $(CFLAGS) -o md2c md2c.o libmdcode.o
md2c.o : md2c.c mdcode.h
	$(CC) $(CFLAGS) -c md2c.c
