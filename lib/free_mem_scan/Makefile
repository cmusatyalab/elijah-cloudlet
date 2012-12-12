CC = gcc
LINK = gcc
CFLAGS = -Wall -m32 -g
LIBS = 

all: free_page_scan

.PHONY: all clean

free_page_scan: free_page_scan.c stub-linux-3.2.0-29.c
	$(CC) $(CFLAGS) -o free_page_scan free_page_scan.c stub-linux-3.2.0-29.c

# free_page_scan: free_page_scan.c stub.c
# 	$(CC) $(CFLAGS) -o free_page_scan free_page_scan.c stub.c

clean:
	rm -f free_page_scan
