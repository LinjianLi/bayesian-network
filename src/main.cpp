#include <iostream>
#include <fstream>
#include <ctime>
#include "Dataset.h"
#include "Network.h"
#include "ChowLiuTree.h"



int main() {

  cout << "Do you want to redirect std::cout to a text file? \n"
       << "Please input [y/n]: " ;
  char redirect;
  redirect = 'n'; //cin >> redirect;

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