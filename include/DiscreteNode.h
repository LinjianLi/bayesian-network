//
// Created by LinjianLi on 2019/2/9.
//

#ifndef BAYESIANNETWORK_DISCRETENODE_H
#define BAYESIANNETWORK_DISCRETENODE_H


#include <set>
#include <map>
#include <utility>
#include <string>
#include "Node.h"
#include "gadget.h"

using namespace std;

class DiscreteNode : public Node {
 public:
  int *potential_vals;
  // The domain of a variable has an order if it is specified by an XMLBIF file.
  vector<string> vec_str_potential_vals;
  vector<int> vec_potential_vals;

  map<int, map<DiscreteConfig, double> >  map_cond_prob_table;
  map<int, double>  map_marg_prob_table;

  // =============== refactor like Weka ===============
  // Keep the count instead of probability.
  map< int, map<int, int> > map_cond_prob_table_statistics;  // Keys: query variable, parents config. Value: count under condition.
                                                             // If the node has no parents, then the second dimension, parents_config, will only use the key -1.
  map<int, int> map_total_count_under_parents_config;   // Key: parents config. Value: total count.
  double laplace_smooth = 0;
  void AddInstances(int query_var, int parents_config, int count);
  double GetProbability(int query_var, int parents_config);
  // ==================================================

  DiscreteNode();
  explicit DiscreteNode(int index);
  DiscreteNode(int index, string name);
  void SetDomain(vector<string> str_domain);
  void SetDomain(vector<int> int_domain);
  int GetDomainSize() const;
  void SetDomainSize(int size);
  void AddParent(Node *node_ptr) override;
  int GetNumParams() const override;
  void ClearParams() override;
  void PrintProbabilityTable();
  int SampleNodeGivenParents(DiscreteConfig evidence);

 private:
  int num_potential_vals;
};


#endif //BAYESIANNETWORK_DISCRETENODE_H
