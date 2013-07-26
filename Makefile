# John Wood                                                                   
# minitorrent Makefile

CC=gcc
CFLAGS= -Wall -pedantic -std=c99 
all: minitorrent minitracker 

minitracker: minitracker.o tracker.o trackerhash.o torrent.o connections.o
	${CC} ${CFLAGS} -o minitracker minitracker.o tracker.o torrent.o trackerhash.o connections.o  -lm `libgcrypt-config --libs`

minitracker.o: minitracker.c tracker.c tracker.h connections.h client.h
	${CC} ${CFLAGS} -c minitracker.c 
	
tracker.o: tracker.c tracker.h trackerhash.h connections.h
	${CC} ${CFLAGS} -c tracker.c 

minitorrent: minitorrent.o torrent.o tracker.o trackerhash.o uploadhash.o connections.o client.o
	${CC} ${CFLAGS} -o minitorrent minitorrent.o torrent.o tracker.o trackerhash.o uploadhash.o connections.o client.o -pthread -lm `libgcrypt-config --libs`

minitorrent.o: minitorrent.c torrent.h tracker.h uploadhash.h connections.h
	${CC} ${CFLAGS} -c minitorrent.c 

torrent.o: torrent.c 
	${CC} ${CFLAGS} -c torrent.c `libgcrypt-config --cflags`

trackerhash.o: trackerhash.c torrent.h trackerhash.h
	${CC} ${CFLAGS} -c trackerhash.c 

uploadhash.o: uploadhash.c uploadhash.h
	${CC} ${CFLAGS} -c uploadhash.c 

connections.o: connections.c connections.h
	${CC} ${CFLAGS} -c connections.c 

client.o: client.c connections.h client.h uploadhash.h
	${CC} ${CFLAGS} -c client.c 

clean:
	rm -f *.o minitorrent minitracker

cleant:
	rm -f *.torrent
