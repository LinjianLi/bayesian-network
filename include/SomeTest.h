//
// Created by Linjian Li on 2018/11/30.
//

#ifndef BAYESIANNETWORK_SOMETEST_H
#define BAYESIANNETWORK_SOMETEST_H

#include <iostream>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include "Trainer.h"
#include "Network.h"

class SomeTest {
public:
	void test1() {
		std::cout << "doing some tests..." << std::endl;

		// do some test

		std::cout << "test finished" << std::endl;
	}

	void test2(Network* network) {
		/*
		 * For a1a dataset, the node 0 is the root.
		 * The children of node "0" are {12,39,40,51,60,75,76,89,96,111,116,120,121,122,123}.
		 * There exists a path:		0(root) -> 75 -> 74(leaf)
		 */

/*
		Node* Y = network->givenIndexToFindNodePointer(0);
		int nZ=2, *Z=new int[nZ]{39,75};
		int eNum=2, *eIndex=new int[eNum]{39,75}, *eValue=new int[eNum]{1,1};
		Combination E = network->constructEvidence(eIndex,eValue,eNum);
		Factor F = network->variableEliminationInferenceReturningPossibilities(Z, nZ, E, Y);
		cout << "=======================================================================" << '\n'
			 << "Given: " ;
		for (int i = 0; i < eNum; ++i) {
			cout << "\"" << eIndex[i] << "\"=" << eValue[i] << " , " ;
		}
		cout << '\n' << "Exact inferences: \n";
		for (auto comb : F.combList) {
			cout << "P(\"" << (*comb.begin()).first << "\'=" << (*comb.begin()).second << ")=" << F.potentialsList[comb] << endl;
		}
*/


/*
		Node* Y = network->givenIndexToFindNodePointer(0);
		int nZ=4, *Z=new int[nZ]{12,39,74,75};
		int eNum=4, *eIndex=new int[eNum]{12,39,74,75}, *eValue=new int[eNum]{0,1,0,1};
		Combination E = network->constructEvidence(eIndex,eValue,eNum);
		Factor F = network->variableEliminationInferenceReturningPossibilities(Z, nZ, E, Y);
		cout << "=======================================================================" << '\n'
			 << "Given: " ;
		for (int i = 0; i < eNum; ++i) {
			cout << "\"" << eIndex[i] << "\"=" << eValue[i] << " , " ;
		}
		cout << '\n' << "Exact inferences: \n";
		for (auto comb : F.combList) {
			cout << "P(\"" << (*comb.begin()).first << "\'=" << (*comb.begin()).second << ")=" << F.potentialsList[comb] << endl;
		}
*/


/*
		Node* Y = network->givenIndexToFindNodePointer(0);
		int nZ=15, *Z=new int[nZ]{12,39,40,51,60,75,76,89,96,111,116,120,121,122,123};
		int eNum=4, *eIndex=new int[eNum]{12,39,74,75}, *eValue=new int[eNum]{0,1,0,1};
		Combination E = network->constructEvidence(eIndex,eValue,eNum);
		Factor F = network->variableEliminationInferenceReturningPossibilities(Z, nZ, E, Y);
		cout << "=======================================================================" << '\n'
			 << "Given: " ;
		for (int i = 0; i < eNum; ++i) {
			cout << "\"" << eIndex[i] << "\"=" << eValue[i] << " , " ;
		}
		cout << '\n' << "Exact inferences: \n";
		for (auto comb : F.combList) {
			cout << "P(\"" << (*comb.begin()).first << "\'=" << (*comb.begin()).second << ")=" << F.potentialsList[comb] << endl;
		}
*/



/**/
		Node* Y = network->givenIndexToFindNodePointer(0);
		int eNum=4, *eIndex=new int[eNum]{12,39,74,75}, *eValue=new int[eNum]{0,1,0,1};
		Combination E = network->constructEvidence(eIndex,eValue,eNum);
		Factor F = network->variableEliminationInferenceReturningPossibilities(E, Y);
		cout << "=======================================================================" << '\n'
			 << "Without specifying the elimination order explicitly. \n"
			 << "Given: " ;
		for (int i = 0; i < eNum; ++i) {
			cout << "\"" << eIndex[i] << "\"=" << eValue[i] << " , " ;
		}
		cout << '\n' << "Exact inferences: \n";
		for (auto comb : F.combList) {
			cout << "P(\"" << (*comb.begin()).first << "\'=" << (*comb.begin()).second << ")=" << F.potentialsList[comb] << endl;
		}
/**/



	}

};


#endif //BAYESIANNETWORK_SOMETEST_H