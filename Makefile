build:
	g++ server.cpp -Wall -std=c++11 -o server
	g++ subscriber.cpp -Wall -std=c++11 -o subscriber
clean:
	rm -f server
	rm -f subscriber