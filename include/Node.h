//
// Created by Linjian Li on 2018/11/29.
//

#ifndef BAYESIANNETWORK_NODE_H
#define BAYESIANNETWORK_NODE_H

#include <set>
#include <map>
#include <utility>
#include <string>
#include <random>
#include <chrono>
#include "gadget.h"

using namespace std;

typedef set< pair<int, int> > Combination;

class Node {

 protected:

  // Indexes for all nodes should start at 0.
  int node_index;

 public:

  string node_name;
  bool is_discrete;
  int num_potential_vals;

  // The domain of a variable has an order if it is specified by an XMLBIF file.
  vector<string> vec_str_potential_vals;
  vector<int> vec_potential_vals;

  int *potential_vals;

  set<Node*> set_parents_ptrs;
  set<Node*> set_children_ptrs;
  set<Combination> set_parents_combinations;
  map<int, map<Combination, double> >  map_cond_prob_table;
  map<int, double>  map_marg_prob_table;

  Node();
  int GetNodeIndex() const;
  void SetNodeIndex(int index);
  void AddChild(Node *node_ptr);
  void AddParent(Node *node_ptr);
  void RemoveChild(Node *node_ptr);
  void RemoveParent(Node *node_ptr);
  void GenParCombs();

  int SampleNodeGivenParents(Combination evidence);
};


#endif //BAYESIANNETWORK_NODE_H
