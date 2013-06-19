.SUFFIXES : .c .o

CC = gcc
INC = 
LIBS = -lmsgpack
CFLAGS = -g $(INC)

OBJS = lib_cloudletdiscover.o
SRCS = lib_cloudletdiscover.c

TARGET = "cloudlet_discover"

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

clean : 
	rm -rf $(OBJS) $(TARGET)

new :
	$(MAKE) clean
	$(MAKE)
