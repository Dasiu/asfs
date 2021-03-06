APP=asfs

# compilation flags
CC=g++
C_FLAGS=-Wall -g
L_FLAGS=-lm -lrt -lpthread
HEADER=${APP}.h


# Rules

all:	${APP}

server.o: src/server.cpp src/${HEADER}
	${CC} -c ${C_FLAGS} src/server.cpp -o obj/server.o

commands.o: src/commands.cpp src/${HEADER}
	${CC} -c ${C_FLAGS} src/commands.cpp -o obj/commands.o



${APP}:	server.o commands.o
	${CC} obj/server.o obj/commands.o -o bin/${APP} ${L_FLAGS}


.PHONY:	test
test:
	./bin/${APP}


.PHONY:	clean
clean:
	rm -rf ./bin/${APP}
	rm -rf ./obj/*.o
