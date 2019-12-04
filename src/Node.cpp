//
// Created by Linjian Li on 2018/11/29.
//

#include "Node.h"
#include "DiscreteNode.h"

Node::Node(int index) : Node(index, to_string(index)){

}

Node::Node(int index, string name) {
  SetNodeIndex(index);
  node_name = name;
}

int Node::GetNodeIndex() const {
  return node_index;
}

bool Node::HasParents() const {
  return !set_parent_indexes.empty();
}


int Node::GetNumParents() const {
  return set_parent_indexes.size();
}

int Node::GetNumDiscParents() const {
  return vec_disc_parent_indexes.size();
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

DiscreteConfig Node::GetDiscParConfigGivenAllVarValue(DiscreteConfig &all_var_val) {
  DiscreteConfig par_var_val;

  if (!HasParents()) {
    return par_var_val;  // Return an empty config.
  }

  assert(all_var_val.size() >= GetNumDiscParents());

  map<int, int> map_all_var_val = DiscreteConfigToMap(all_var_val);

  for (const auto & par : this->vec_disc_parent_indexes) {
    int given_val = map_all_var_val.at(par);
    par_var_val.insert(pair<int, int>(par, given_val));
  }

  return par_var_val;
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

  set_parent_indexes.erase(p_idx);
  vec_disc_parent_indexes.erase(std::find(vec_disc_parent_indexes.begin(), vec_disc_parent_indexes.end(), p_idx));
  map_disc_parents_domain_size.erase(p_idx);
}


void Node::GenDiscParCombs(set<Node*> set_parent_ptrs) {
  set_discrete_parents_combinations.clear();

  // If the node has no parent, then it should have ONE empty parent configuration.
  if (set_parent_ptrs.empty()) {
    DiscreteConfig empty_config;
    set_discrete_parents_combinations.insert(empty_config);
    return;
  }

  // Preprocess. Construct set of sets.
  set<DiscreteConfig> set_of_sets;

  for (auto par_ptr : set_parent_ptrs) {
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
  set_discrete_parents_combinations.clear();

  vec_disc_parent_indexes.clear();
  set_parent_indexes.clear();
  map_disc_parents_domain_size.clear();
}

void Node::ClearChildren() {
  set_children_indexes.clear();
}