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
#include<bits/stdc++.h>
#include "gadget.h"

using namespace std;


class Node {

 protected:

  // Indexes for all nodes in a network should start at 0.
  int node_index;

 public:

  string node_name;
  bool is_discrete;

  set<Node*> set_parents_ptrs;
  set<Node*> set_children_ptrs;
  set<DiscreteConfig> set_discrete_parents_combinations;

  // =============== refactor like Weka ===============
  vector<int> vec_disc_parent_indexes;  // The order matters.
  set<int> set_parent_indexes;
  map<int, int> map_disc_parents_domain_size;  // Key: parent index. Value: parent's domain size.

  set<int> set_children_indexes;
  // ==================================================

  Node() = default;
  explicit Node(int index);
  Node(int index, string name);
  int GetNodeIndex() const;
  void SetNodeIndex(int index);
  int GetNumParents() const;
  int GetNumChildren() const;
  virtual void AddChild(Node *node_ptr);
  virtual void AddParent(Node *node_ptr);
  void RemoveChild(Node *node_ptr);
  virtual void RemoveParent(Node *node_ptr);
  void GenDiscParCombs();
  void ClearParents();
  void ClearChildren();
  virtual int GetNumParams() const = 0;
  virtual void ClearParams() = 0;
};


#endif //BAYESIANNETWORK_NODE_H
