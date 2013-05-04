.SUFFIXES : .c .o

CC = gcc
INC = `pkg-config glib-2.0 fuse --cflags`
LIBS = -lpthread `pkg-config glib-2.0 fuse --libs`
CFLAGS = -g $(INC)

OBJS = cloudlet_cachefs.o fuse.o
SRCS = cloudlet_cachefs.c fuse.c

TARGET = "cachefs"

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

clean : 
	rm -rf $(OBJS) $(TARGET)

new :
	$(MAKE) clean
	$(MAKE)
