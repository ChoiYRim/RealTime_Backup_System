

LIB_DIR = /usr/local/lib
INCLUDE_DIR = /usr/local/include

CC = g++

lib : libbackup.so

libbackup.so : 
	$(CC) -shared

##########

install:
	cp libbackup.so $(LIB_DIR)
	###############

clean:
	rm -rf *.o *.so
