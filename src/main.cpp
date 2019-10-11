#include <iostream>
#include <fstream>
#include <ctime>


int main() {


  std::cout << "Do you want to redirect std::cout to a text file? \n"
       << "Please input [y/n]: " ;
  char redirect;
  redirect = 'n'; //cin >> redirect;

  std::string log_file = "log_bayesian_network_program.txt";
  std::streambuf *coutbuf = std::cout.rdbuf();  //save old buf
  std::ofstream out(log_file);

  if (redirect=='y' || redirect=='Y') {
    // Redirect std::cout.
    std::cout << '\n' << "The std::cout will be redirect to " << log_file << std::endl;
    std::cout.rdbuf(out.rdbuf()); //redirect std::cout to log_bayesian_network.txt!
  }

  time_t now = time(0);
  char* dt = ctime(&now);
  std::cout << "Log file last modified at " << dt << std::endl;

  now = time(0);
  dt = ctime(&now);
  std::cout << '\n' << "Log file last modified at " << dt << std::endl;

  if (redirect=='y' || redirect=='Y') {
    std::cout.rdbuf(coutbuf); //reset to standard output again
    std::cout << "The std::cout has been reset to standard output again." << std::endl;
  }
  std::cout << "Hello, World!" << std::endl;

  return 0;


}