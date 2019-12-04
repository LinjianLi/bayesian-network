//
// Created by LinjianLi on 2019/2/9.
//

#include "DiscreteNode.h"
#include "Network.h"

#include <utility>

DiscreteNode::DiscreteNode(): DiscreteNode(-1) {}

DiscreteNode::DiscreteNode(int index): DiscreteNode(index, to_string(index)) {}

DiscreteNode::DiscreteNode(int index, string name) {
  is_discrete = true;
  SetNodeIndex(index);
  node_name = std::move(name);

  num_potential_vals = -1;
}

void DiscreteNode::SetDomain(vector<string> str_domain) {
  SetDomainSize(str_domain.size());
  vec_str_potential_vals = str_domain;

  for (int i = 0; i < GetDomainSize(); ++i) {
    vec_potential_vals.push_back(i);
  }
}

void DiscreteNode::SetDomain(vector<int> int_domain) {
  SetDomainSize(int_domain.size());
  vec_potential_vals = int_domain;
}

int DiscreteNode::GetDomainSize() const {
  return num_potential_vals;
}

void DiscreteNode::SetDomainSize(int size) {
  num_potential_vals = size;
  vec_potential_vals.reserve(size);
}

void DiscreteNode::AddParent(Node *p) {
  if (!p->is_discrete) {
    fprintf(stderr, "Error in function %s! \n"
                    "Discrete node must not have continuous parent!", __FUNCTION__);
    exit(1);
  }
  int p_idx = p->GetNodeIndex();
  if (set_parent_indexes.find(p_idx) == set_parent_indexes.end()) {
    set_parent_indexes.insert(p_idx);
    vec_disc_parent_indexes.push_back(p_idx);
    map_disc_parents_domain_size[p_idx] = ((DiscreteNode*)p)->GetDomainSize();
  }
}

int DiscreteNode::GetNumParams() {
  int scale = this->GetNumParentsConfig();
  return this->GetDomainSize() * scale;
}

void DiscreteNode::ClearParams() {
  map_cond_prob_table_statistics.clear();
  map_total_count_under_parents_config.clear();
}


void DiscreteNode::PrintProbabilityTable() {
  cout << GetNodeIndex() << ":\t";

  if (this->HasParents()) {    // If this node has parents

    for(int i = 0; i<GetDomainSize(); ++i) {    // For each head variable of CPT
      int query = vec_potential_vals.at(i);
      for (int j = 0; j < GetNumParentsConfig(); ++j){  // For tail variables of CPT
        string condition = "parent_config_" + to_string(j);

        cout << "P(" << query << '|' << condition << ")=" << GetProbability(i, j) << '\t';
      }
    }
    cout << endl;

  } else {

    for(int i = 0; i < GetDomainSize(); ++i) {    // For each row of MPT
      int query = vec_potential_vals.at(i);
      cout << "P(" << query << ")=" << GetProbability(GetIndexOfValue(query), 0) << '\t';
    }
    cout << endl;

  }
}

int DiscreteNode::SampleNodeGivenParents(DiscreteConfig &evidence, Network *net) {
  // The evidence should contain all parents of this node.
  // The evidence about other nodes (including children) are IGNORED!!!
  DiscreteConfig par_evi;
  for (auto &e : evidence) {
    if (set_parent_indexes.find(e.first) != set_parent_indexes.end()) {
      par_evi.insert(e);
    }
  }

  vector<int> weights;
  if (par_evi.empty()) {
    for (int i = 0; i < GetDomainSize(); ++i) {
      int w = (int) (GetProbability(i, 0) * 10000);
      weights.push_back(w);
    }
  } else {

    vector<int> complete_instance_val_indexes = net->SparseInstanceToCompleteValueIndexes(par_evi);
    int par_config = GetParConfigGivenAllVarValueIndexes(complete_instance_val_indexes);

    for (int i = 0; i < GetDomainSize(); ++i) {
      int w = (int) GetProbability(GetIndexOfValue(vec_potential_vals.at(i)), par_config) * 10000;;
      weights.push_back(w);
    }
  }

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine rand_gen(seed);
  discrete_distribution<int> this_distribution(weights.begin(),weights.end());
  return vec_potential_vals.at(this_distribution(rand_gen));
}


void DiscreteNode::AddInstanceOfValueIndex(vector<int> instance_of_value_index) {
  int parents_config = GetParConfigGivenAllVarValueIndexes(instance_of_value_index);
  AddCount(instance_of_value_index.at(GetNodeIndex()), parents_config, 1);
}


void DiscreteNode::InitializeCPT() {
  for (int j = 0; j < GetNumParentsConfig(); ++j) {
    map_total_count_under_parents_config[j] = 0;
    for (int i = 0; i < GetDomainSize(); ++i) {
      map_cond_prob_table_statistics[i][j] = 0;
    }
  }
  cpt_initialized = true;
}


void DiscreteNode::AddCount(int query_val_index, int parents_config, int count) {

  if (!cpt_initialized) { InitializeCPT(); }

  map_total_count_under_parents_config[parents_config] += count;
//  if (map_cond_prob_table_statistics.find(query_val_index) == map_cond_prob_table_statistics.end()
//      || map_cond_prob_table_statistics[query_val_index].find(parents_config) == map_cond_prob_table_statistics[query_val_index].end()) {
//    map_cond_prob_table_statistics[query_val_index][parents_config] = 0;
//  }

  map_cond_prob_table_statistics[query_val_index][parents_config] += count;
}

double DiscreteNode::GetProbability(int query_val_index, int parents_config) {

  // todo: fix the bug when the given query or parent config are not seen in the training stage.

  int frequency_count =  map_cond_prob_table_statistics[query_val_index][parents_config];
  int total = map_total_count_under_parents_config[parents_config];
  double prob = (frequency_count + laplace_smooth) / (total + laplace_smooth * GetDomainSize());
  return prob;
}

int DiscreteNode::GetIndexOfValue(int val) {
  auto it = std::find(this->vec_potential_vals.begin(), this->vec_potential_vals.end(), val);
  int val_index = std::distance(this->vec_potential_vals.begin(), it);
  return val_index;
}

