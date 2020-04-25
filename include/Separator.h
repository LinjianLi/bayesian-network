//
// Created by LinjianLi on 2019/2/20.
//

#ifndef BAYESIANNETWORK_SEPARATOR_H
#define BAYESIANNETWORK_SEPARATOR_H

#include "Clique.h"

/**
 * @brief: Separator is used like an edge, and the other none separator cliques are nodes in a network.
 */
class Separator : public Clique {
 protected:
  Separator(const Separator&) = default;

 public:
  int weight;//the number of nodes in this separator
  map<DiscreteConfig, double> map_old_potentials;//this member is used when constructing message in separator

  Separator();
  explicit Separator(set<Node*>);

  Separator* CopyWithoutPtr();

  void UpdateUseMessage(Factor) override;
  Factor ConstructMessage() override;


};


#endif //BAYESIANNETWORK_SEPARATOR_H
