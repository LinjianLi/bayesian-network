//
// Created by Linjian Li on 2018/11/29.
//

#include "Node.h"
#include "DiscreteNode.h"

Node::Node(int index) {
  SetNodeIndex(index);
}

Node::Node(int index, string name): Node(index) {
  node_name = name;
}

int Node::GetNodeIndex() const {
  return node_index;
}


int Node::GetNumParents() const {
  return set_parent_indexes.size();
}

int Node::GetNumChildren() const {
  return set_children_indexes.size();
}

int Node::GetNumParentsConfig() {
  if (num_parents_config < 0) {
    num_parents_config = 1;
    for (auto const &par : vec_disc_parent_indexes) {
      // If the node has no parent, this for loop will not be executed.
      // And the number of parents config will be 1.
      num_parents_config *= map_disc_parents_domain_size[par];
    }
  }
  return num_parents_config;
}

int Node::GetParConfigGivenParValueIndexes(vector<int> &par_val_indexes) {
  int config = 0;
  for (int i = 0; i < GetNumParents(); ++i) {
    config *= map_disc_parents_domain_size[vec_disc_parent_indexes.at(i)];
    config += par_val_indexes.at(i);
  }
  return config;
}

int Node::GetParConfigGivenAllVarValueIndexes(vector<int> &all_var_val_indexes) {
  vector<int> par_val_indexes;
  par_val_indexes.reserve(vec_disc_parent_indexes.size());
  for (auto const par : vec_disc_parent_indexes) {
    par_val_indexes.push_back(all_var_val_indexes.at(par));
  }
  return GetParConfigGivenParValueIndexes(par_val_indexes);
}


void Node::SetNodeIndex(int i) {
  if (i<0) {
    fprintf(stderr, "Error in function %s! \nInvalid index!", __FUNCTION__);
    exit(1);
  }
  node_index = i;
}


void Node::AddChild(Node *c) {
  int c_idx = c->GetNodeIndex();
  if (set_children_indexes.find(c_idx) == set_children_indexes.end()) {
    set_children_indexes.insert(c_idx);
  }
}


void Node::AddParent(Node *p) {
  set_parents_ptrs.insert(p);
  int p_idx = p->GetNodeIndex();
  if (set_parent_indexes.find(p_idx) == set_parent_indexes.end()) {
    set_parent_indexes.insert(p_idx);
    if (p->is_discrete) {
      vec_disc_parent_indexes.push_back(p_idx);
      map_disc_parents_domain_size[p_idx] = ((DiscreteNode *) p)->GetDomainSize();
    }
  }
}


void Node::RemoveChild(Node *c) {
  int c_idx = c->GetNodeIndex();
  if (set_children_indexes.find(c_idx) == set_children_indexes.end()) {
    fprintf(stderr, "Node #%d does not have parent node #%d!", this->GetNodeIndex(), c_idx);
    return;
  }
  set_children_indexes.erase(c_idx);
}


void Node::RemoveParent(Node *p) {
  int p_idx = p->GetNodeIndex();
  if (set_parent_indexes.find(p_idx)==set_parent_indexes.end()) {
    fprintf(stderr, "Node #%d does not have parent node #%d!", this->GetNodeIndex(), p_idx);
    return;
  }
  set_parents_ptrs.erase(p);

  set_parent_indexes.erase(p_idx);
  vec_disc_parent_indexes.erase(std::find(vec_disc_parent_indexes.begin(), vec_disc_parent_indexes.end(), p_idx));
  map_disc_parents_domain_size.erase(p_idx);
}


void Node::GenDiscParCombs() {
  set_discrete_parents_combinations.clear();

  // Preprocess. Construct set of sets.
  set<DiscreteConfig> set_of_sets;
  if (set_parent_indexes.empty()) {return;}
  for (auto par_ptr : set_parents_ptrs) {
    if (!par_ptr->is_discrete) { continue; }
    DiscreteConfig cb;
    pair<int, int> ele;
    DiscreteNode *d_par_ptr = (DiscreteNode*)(par_ptr);

    for (int i=0; i<d_par_ptr->GetDomainSize(); ++i) {
      ele.first = par_ptr->node_index;
      ele.second = ((DiscreteNode*)par_ptr)->vec_potential_vals.at(i);
      cb.insert(ele);
    }
    set_of_sets.insert(cb);
  }

  // Generate
  set_discrete_parents_combinations = GenAllCombinationsFromSets(&set_of_sets);

}

void Node::ClearParents() {
  set_parents_ptrs.clear();
  set_discrete_parents_combinations.clear();

  vec_disc_parent_indexes.clear();
  set_parent_indexes.clear();
  map_disc_parents_domain_size.clear();
}

void Node::ClearChildren() {
  set_children_indexes.clear();
}