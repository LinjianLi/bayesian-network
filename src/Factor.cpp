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
      DiscreteConfig c;
      c.insert(p);
      set_combinations.insert(c);
      map_potentials[c] = disc_node->GetProbability(disc_node->GetIndexOfValue(p.second), 0);


      int par_config = 0;
      map_potentials_new[par_config * disc_node->GetDomainSize() + disc_node->GetIndexOfValue(p.second)];
    }


  } else {


    // If this disc_node has parents, the outer loop is for the disc_node, and the inner loop is for the parents.
    related_variables.insert(disc_node->set_parent_indexes.begin(), disc_node->set_parent_indexes.end());
    for (auto &p : set_pair_temp) {
      for (auto it_pc=disc_node->set_discrete_parents_combinations.begin(); it_pc!=disc_node->set_discrete_parents_combinations.end(); ++it_pc) {
        DiscreteConfig c = (*it_pc);
        auto c_old = c;
        c.insert(p);
        set_combinations.insert(c);


        vector<int> complete_instance = net->SparseInstanceFillZeroToCompleteInstance(c_old);
        vector<int> complete_instance_var_val_index = net->ConvertInstanceIntValuesToValueIndexesOfDiscreteNodes(complete_instance);
        int par_config = disc_node->GetParConfigGivenAllVarValueIndexes(complete_instance_var_val_index);


        map_potentials[c] = disc_node->GetProbability(
                disc_node->GetIndexOfValue(p.second),
                par_config);


        map_potentials_new[par_config * disc_node->GetDomainSize() + disc_node->GetIndexOfValue(p.second)] =
                disc_node->GetProbability(
                        disc_node->GetIndexOfValue(p.second),
                        par_config);
      }
    }


    vec_related_vars = disc_node->vec_disc_parent_indexes;
    vec_related_vars.push_back(disc_node->GetNodeIndex());
    map_vars_domain_size = disc_node->map_disc_parents_domain_size;
    map_vars_domain_size[disc_node->GetNodeIndex()] = disc_node->GetDomainSize();


  }
}


Factor::Factor(set<int> rv,
                        set<DiscreteConfig> sc,
                        map<DiscreteConfig, double> mp) {
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

Factor Factor::SumOverVar(int variable) {





//  int pos = -1;
//  while (vec_related_vars.at(++pos) != variable);
//  int gap = 1;
//  for (int i = pos; i < vec_related_vars.size(); ++i) {
//    gap *= map_vars_domain_size.at(vec_related_vars.at(i));
//  }
//  vector<int> vars_domain_size;
//  int num_new_config = 1;
//  for (auto var : vec_related_vars) {
//    if (var == variable) { continue; }
//    num_new_config *= map_vars_domain_size.at(var);
//    vars_domain_size.push_back(map_vars_domain_size.at(var));
//  }
//
//  vector<int> temp_vec_related_vars = this->vec_related_vars;
//  temp_vec_related_vars.erase(temp_vec_related_vars.begin() + pos);
//  map<int, int> temp_map_vars_domain_size = this->map_vars_domain_size;
//  temp_map_vars_domain_size.erase(variable);
//  map<int, double> temp_map_potentials_new;
//
//  map<int, int> map_new_config_to_old_config_floor;
//  for (int i = 0; i < num_new_config; ++i) {
//    vector<int> detail = TheNthNaryCount(vars_domain_size, i);
//    int old_config_floor = 0;
//    for (int j = 0; j < detail.size(); ++j) {
//      if (j == pos) {
//        old_config_floor *= map_vars_domain_size.at(vec_related_vars.at(pos));
//        old_config_floor += 0; // Floor. Thus, zero.
//      }
//      old_config_floor *= temp_map_vars_domain_size.at(vec_related_vars.at(j));
//      old_config_floor += detail.at(j);
//    }
//    map_new_config_to_old_config_floor[i] = old_config_floor;
//  }
//
//  for (int i = 0; i < num_new_config; ++i) {
//    double p = 0;
//    for (int j = 0; j < map_vars_domain_size.at(vec_related_vars.at(pos)); ++j) {
//      p += map_potentials_new.at(map_new_config_to_old_config_floor[i] + gap * j);
//    }
//    temp_map_potentials_new[i] = p;
//  }




  Factor newFactor;
  this->related_variables.erase(variable);
  newFactor.related_variables = this->related_variables;
  for (auto comb : set_combinations) {
    pair<int, int> pair_to_be_erased;
    for (auto p : comb) {
      if (p.first==variable) {
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




//  newFactor.vec_related_vars = temp_vec_related_vars;
//  newFactor.map_vars_domain_size = temp_map_vars_domain_size;
//  newFactor.map_potentials_new = temp_map_potentials_new;




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
  for (int i = 0; i < set_combinations.size(); ++i) {
    map_potentials_new[i] /= denominator;
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