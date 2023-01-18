#include <unistd.h>
#include <iostream>
#include "ts_queue.hpp"
#include "reader.hpp"

int main() {
	TSQueue<Item*>* q = new TSQueue<Item*>;

	Reader* reader = new Reader(80, "./tests/01.in", q);

	reader->start();
	reader->join();

	sleep(1);

	for (int i = 0; i < 20; i++)
		std::cout << q->dequeue() << std::endl;

	sleep(1);

	for (int i = 0; i < 40; i++)
		std::cout << q->dequeue() << std::endl;

	sleep(1);
	for (int i = 0; i < 20; i++)
		std::cout << q->dequeue() << std::endl;

	delete reader;
	delete q;

	return 0;;
}
