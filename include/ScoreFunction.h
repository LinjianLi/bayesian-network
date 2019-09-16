//
// Created by llj on 3/12/19.
//

#ifndef BAYESIANNETWORK_SCOREFUNCTION_H
#define BAYESIANNETWORK_SCOREFUNCTION_H

#include <iostream>
#include <set>
#include <queue>
#include <vector>
#include <cmath>

#include "Network.h"
#include "Dataset.h"
#include "Node.h"
#include "Factor.h"
#include "gadget.h"

/**
 * IMPORTANT!
 * In this project, higher score means better structure.
 * That is, the aim is to MAXIMIZE the score.
 */
class ScoreFunction {
 public:

  ScoreFunction(Network *net, Dataset *trn);
  double LogLikelihoodForNode(Node *node_ptr);
  double LogLikelihood();
  double K2();
  double BDe();
  double BDeu(int equi_sample_size = 10);
  double AIC();
  double BIC();
  double MDL();

 private:

  ScoreFunction() = default;

  int num_network_params = 0;
  Network *net;
  Dataset *trn;

  double LogLikelihoodForNode(Node*, Network*, Dataset*);
  double LogLikelihood(Network*, Dataset*);
  double K2(Network*, Dataset*);
  double LogK2(Network*, Dataset*);
  double BDe(Network*, Dataset*, int);
  double BDeu(Network*, Dataset*, int);
  double LogBDeu(Network*, Dataset*, int);
  double AIC(Network*, Dataset*);
  double BIC(Network*, Dataset*);
  double MDL(Network*, Dataset*);

};


#endif //BAYESIANNETWORK_SCOREFUNCTION_H
