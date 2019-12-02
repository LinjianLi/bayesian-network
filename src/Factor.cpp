//
// Created by Linjian Li on 2018/11/29.
//

#include "Factor.h"
#include "Network.h"


Factor::Factor(DiscreteNode *disc_node, Network *net) {
  int node_index = disc_node->GetNodeIndex();

  related_variables.insert(node_index);

  set<pair<int,int>> set_pair_temp;
  for (int i = 0; i < disc_node->GetDomainSize(); ++i) {
    DiscreteConfig comb_temp;
    pair<int, int> pair_temp;
    pair_temp.first = node_index;
    pair_temp.second = disc_node->vec_potential_vals.at(i);
    set_pair_temp.insert(pair_temp);
  }

  // If this disc_node has no parents.
  if (!disc_node->HasParents()) {


    for (auto &p : set_pair_temp) {
      DiscreteConfig c, empty_par_config;
      c.insert(p);
      set_combinations.insert(c);
      map_potentials[c] = disc_node->GetProbability(p.second, empty_par_config);
    }


  } else {


    // If this disc_node has parents, the outer loop is for the disc_node, and the inner loop is for the parents.
    related_variables.insert(disc_node->set_parent_indexes.begin(), disc_node->set_parent_indexes.end());
    for (auto &p : set_pair_temp) {
      for (const auto & comb : disc_node->set_discrete_parents_combinations) {
        DiscreteConfig c = comb;
        auto c_old = c;
        c.insert(p);
        set_combinations.insert(c);

        DiscreteConfig par_config = disc_node->GetDiscParConfigGivenAllVarValue(c_old);

        map_potentials[c] = disc_node->GetProbability(
                p.second,
                par_config);
      }
    }


  }
}


Factor::Factor(set<int> &rv,
               set<DiscreteConfig> &sc,
               map<DiscreteConfig, double> &mp) {
  this->related_variables = rv;
  this->set_combinations = sc;
  this->map_potentials = mp;
}


Factor Factor::MultiplyWithFactor(Factor second_factor) {
  Factor newFactor;

  newFactor.related_variables.insert(this->related_variables.begin(),this->related_variables.end());
  newFactor.related_variables.insert(second_factor.related_variables.begin(),second_factor.related_variables.end());

  set<int> common_related_variables;
  set_intersection(this->related_variables.begin(),this->related_variables.end(),
            second_factor.related_variables.begin(),second_factor.related_variables.end(),
            std::inserter(common_related_variables,common_related_variables.begin()));

  for (auto first: set_combinations) {
    for (auto second : second_factor.set_combinations) {

      // solve the bug about common variables
      // If two combinations have different values on common variables,
      // which means that they conflict,
      // then these two combinations can not form a legal entry.
      if (!FirstCompatibleSecond(&first, &second)) continue;

      DiscreteConfig new_comb;
      new_comb.insert(first.begin(),first.end());
      new_comb.insert(second.begin(),second.end());
      newFactor.set_combinations.insert(new_comb);
      newFactor.map_potentials[new_comb] = this->map_potentials[first] * second_factor.map_potentials[second];
    }
  }

  return newFactor;
}

Factor Factor::SumOverVar(int index) {
  Factor newFactor;
  this->related_variables.erase(index);
  newFactor.related_variables = this->related_variables;
  for (auto comb : set_combinations) {
    pair<int, int> pair_to_be_erased;
    for (auto p : comb) {
      if (p.first==index) {
        pair_to_be_erased = p;
        break;
      }
    }
    double temp = this->map_potentials[comb];
    comb.erase(pair_to_be_erased);
    if (newFactor.set_combinations.find(comb)!=newFactor.set_combinations.end()) {
      newFactor.map_potentials[comb] += temp;
    } else {
      newFactor.set_combinations.insert(comb);
      newFactor.map_potentials[comb] = temp;
    }
  }
  return newFactor;
}

Factor Factor::SumOverVar(DiscreteNode *node) {
  return SumOverVar(node->GetNodeIndex());
}


void Factor::Normalize() {
  double denominator = 0;
  for (auto &comb : set_combinations) {
    denominator += map_potentials[comb];
  }
  for (auto &comb : set_combinations) {
    map_potentials[comb] /= denominator;
  }
}


void Factor::PrintPotentials() const {
  for (auto &potentials_key_value : map_potentials) {
    for (auto &vars_index_value : potentials_key_value.first) {
      cout << '(' << vars_index_value.first << ',' << vars_index_value.second << ") ";
    }
    cout << "\t: " << potentials_key_value.second << endl;
  }
  cout << "----------" << endl;
}