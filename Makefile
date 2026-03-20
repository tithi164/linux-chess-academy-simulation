all:
	gcc main.c -o main -lrt -pthread
	gcc match.c -o match -pthread
	gcc logger.c -o logger -lrt

run:
	./main

clean:
	rm -f main match logger log.txt players.txt
