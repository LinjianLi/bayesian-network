//
// Created by LinjianLi on 2019/2/9.
//

#include "DiscreteNode.h"

#include <utility>

DiscreteNode::DiscreteNode() {
  is_discrete = true;
}

DiscreteNode::DiscreteNode(int index) {
  is_discrete = true;
  SetNodeIndex(index);
}

DiscreteNode::DiscreteNode(int index, string name): DiscreteNode(index) {
  node_name = std::move(name);
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

int DiscreteNode::GetNumParams() const {
  int scale = this->set_discrete_parents_combinations.empty() ? 1 : this->set_discrete_parents_combinations.size();
  return this->GetDomainSize() * scale;
}

void DiscreteNode::ClearParams() {
  if (set_parent_indexes.empty()) {
    for (auto &kv : map_marg_prob_table) {
      kv.second = 0;
    }
  } else {
    for (auto &kv : map_cond_prob_table) {
      for (auto &kv2 : kv.second) {
        kv2.second = 0;
      }
    }
  }

  map_cond_prob_table_statistics.clear();
  map_total_count_under_parents_config.clear();
}


void DiscreteNode::PrintProbabilityTable() {
  cout << GetNodeIndex() << ":\t";

  if (set_parent_indexes.empty()) {    // If this node has no parents
    for(int i = 0; i < GetDomainSize(); ++i) {    // For each row of MPT
      int query = vec_potential_vals.at(i);
      cout << "P(" << query << ")=" << map_marg_prob_table[query] << '=' << GetProbability(GetIndexOfValue(query), 0) << '\t';
    }
    cout << endl;

  } else {  // If this node has parents

    for(int i = 0; i<GetDomainSize(); ++i) {    // For each row of CPT
      int query = vec_potential_vals.at(i);
      for (const auto &comb : set_discrete_parents_combinations) {  // For each column of CPT
        string condition;
        for (auto &p : comb) {
          condition += ("\"" + to_string(p.first) + "\"=" + to_string(p.second));
        }
        cout << "P(" << query << '|' << condition << ")=" << map_cond_prob_table[query][comb] << '\t';
      }
    }
    cout << endl;



    // todo: delete the following
    for(int i = 0; i<GetDomainSize(); ++i) {    // For each row of CPT
      int query = vec_potential_vals.at(i);
      for (int j = 0; j<GetNumParentsConfig(); ++j) {  // For each column of CPT
        cout << "P(" << query << '|' << j << ")=" << GetProbability(query, j) << '\t';
      }
    }
    cout << endl;



  }
}

int DiscreteNode::SampleNodeGivenParents(DiscreteConfig evidence) {
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
    for (int i=0; i<GetDomainSize(); ++i) {
      int w = (int)(map_marg_prob_table[vec_potential_vals.at(i)]*10000);
      weights.push_back(w);
    }
  } else {
    for (int i=0; i<GetDomainSize(); ++i) {
      int w = (int)(map_cond_prob_table[vec_potential_vals.at(i)][par_evi]*10000);
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


void DiscreteNode::AddCount(int query_val_index, int parents_config, int count) {
  map_total_count_under_parents_config[parents_config] += count;
  if (map_cond_prob_table_statistics.find(query_val_index) == map_cond_prob_table_statistics.end()
      || map_cond_prob_table_statistics[query_val_index].find(parents_config) == map_cond_prob_table_statistics[query_val_index].end()) {
    map_cond_prob_table_statistics[query_val_index][parents_config] = 0;
  }
  map_cond_prob_table_statistics[query_val_index][parents_config] += count;
}

double DiscreteNode::GetProbability(int query_val_index, int parents_config) {
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

