.SUFFIXES : .c .o

CC = gcc
REDIS_LIB = libhiredis.a
INC = `pkg-config glib-2.0 fuse libevent --cflags`
LIBS = -lpthread `pkg-config glib-2.0 fuse libevent --libs` $(REDIS_LIB)
CFLAGS = -g -std=gnu99 $(INC)

OBJS = cloudlet_cachefs.o fuse.o redis.o io.o cond.o async.o net.o sds.o dict.o
REDIS_OBJS = example.o

TARGET = "cachefs"

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

example : $(REDIS_OBJS)
	$(CC) -o $@ $(REDIS_OBJS) $(LIBS)

clean : 
	rm -rf $(OBJS) $(TARGET)

new :
	$(MAKE) clean
	$(MAKE)
