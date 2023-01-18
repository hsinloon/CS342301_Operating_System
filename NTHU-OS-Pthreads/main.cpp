#include <assert.h>
#include <stdlib.h>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000

int main(int argc, char** argv) {
	assert(argc == 4);

	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

	// TODO: implements main function

	TSQueue<Item*>* input_q = new TSQueue<Item*>(READER_QUEUE_SIZE);
	TSQueue<Item*>* worker_q = new TSQueue<Item*>(WORKER_QUEUE_SIZE);
	TSQueue<Item*>* output_q = new TSQueue<Item*>(WRITER_QUEUE_SIZE);
	/*TSQueue<Item*>* input_q = new TSQueue<Item*>(n);
	TSQueue<Item*>* worker_q = new TSQueue<Item*>(n);
	TSQueue<Item*>* output_q = new TSQueue<Item*>(n);*/
	Transformer* transformer = new Transformer;

	Reader* reader = new Reader(n, input_file_name, input_q);
	Writer* writer = new Writer(n, output_file_name, output_q);

	Producer* p1 = new Producer(input_q, worker_q, transformer);
	Producer* p2 = new Producer(input_q, worker_q, transformer);
	Producer* p3 = new Producer(input_q, worker_q, transformer);
	Producer* p4 = new Producer(input_q, worker_q, transformer);
	int check_period = CONSUMER_CONTROLLER_CHECK_PERIOD;
	int low_threshold = (WORKER_QUEUE_SIZE * CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE) / 100;
	int high_threshold = (WORKER_QUEUE_SIZE * CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE) / 100;

	ConsumerController* consumercontroller = new ConsumerController(worker_q, output_q, transformer , check_period, low_threshold, high_threshold);

	reader->start();
	writer->start();

	p1->start();
	p2->start();
	p3->start();
	p4->start();

	consumercontroller->start();
	
	reader->join();
	writer->join();

	delete p4;
	delete p3;
	delete p2;
	delete p1;
	delete writer;
	delete reader;
	delete consumercontroller;
	delete input_q;
	delete worker_q;
	delete output_q;

	return 0;
}
