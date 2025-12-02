all:
	g++ -std=c++20 -Wall -Wextra device.cpp -o a.out -lgtest -pthread

clean:
	rm -f a.out

