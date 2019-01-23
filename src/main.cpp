#include <iostream>
#include <fstream>
#include <ctime>
#include "SomeTest.h"
#include "Trainer.h"
#include "Network.h"

int main() {

	SomeTest st;

	st.test1();

	cout << "Do you want to redirect std::cout to a text file? \n"
	     << "Please input [y/n]: " ;
	char redirect;
	cin >> redirect;

	string log_file = "log_bayesian_network_program.txt";
	std::streambuf *coutbuf = std::cout.rdbuf();  //save old buf
	std::ofstream out(log_file);

	if (redirect=='y' || redirect=='Y') {
		// Redirect std::cout.
		std::cout << '\n' << "The std::cout will be redirect to " << log_file << endl;
		std::cout.rdbuf(out.rdbuf()); //redirect std::cout to log_bayesian_network.txt!
	}

	time_t now = time(0);
	char* dt = ctime(&now);
	cout << "Log file last modified at " << dt << endl;

	auto *trainer = new Trainer();
	auto *tester = new Trainer();
	auto *network = new Network();

	string train_set_file_path, test_set_file_path;

	train_set_file_path =
				//	"../data/a1a.txt"
				//	"../data/a2a.txt"
					"../data/a3a.txt"
				//	"../data/w1a.txt"
				//	"../data/w7a.txt"
					;

	test_set_file_path =
				//	"../data/a1a.test.txt"
				//	"../data/a2a.test.txt"
					"../data/a3a.test.txt"
				//	"../data/w1a.test.txt"
				//	"../data/w7a.test.txt"
					;

	trainer->LoadLIBSVMDataAutoDetectConfig(train_set_file_path);
	tester->LoadLIBSVMDataAutoDetectConfig(test_set_file_path);
	network->StructLearnChowLiuTreeCompData(trainer);
	network->LearnParmsKnowStructCompData(trainer);
	network->TestNetReturnAccuracy(tester);

	now = time(0);
	dt = ctime(&now);
	cout << '\n' << "Log file last modified at " << dt << endl;

	if (redirect=='y' || redirect=='Y') {
		std::cout.rdbuf(coutbuf); //reset to standard output again
		std::cout << "The std::cout has been reset to standard output again." << endl;
	}
	std::cout << "Hello, World!" << std::endl;

	return 0;


}