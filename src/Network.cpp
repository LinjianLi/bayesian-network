#pragma clang diagnostic push
#pragma ide diagnostic ignored "openmp-use-default-none"
//
// Created by Linjian Li on 2018/11/29.
//

#include "Network.h"
#include "ScoreFunction.h"

Network::Network(): Network(true) {}

Network::Network(bool pure_disc) {
  this->pure_discrete = pure_disc;
}


void Network::PrintEachNodeParents() {
  for (const auto &id_node_ptr : map_idx_node_ptr) {
    auto node_ptr = id_node_ptr.second;
    cout << node_ptr->node_name << ":\t";
    for (const auto &par_node_ptr : GetParentPtrsOfNode(node_ptr->GetNodeIndex())) {
      cout << par_node_ptr->node_name << '\t';
    }
    cout << endl;
  }
}

void Network::PrintEachNodeChildren() {
  for (const auto &id_node_ptr : map_idx_node_ptr) {
    auto node_ptr = id_node_ptr.second;
    cout << node_ptr->node_name << ":\t";
    for (const auto &chi_node_ptr : GetChildrenPtrsOfNode(node_ptr->GetNodeIndex())) {
      cout << chi_node_ptr->node_name << '\t';
    }
    cout << endl;
  }
}

Node* Network::FindNodePtrByIndex(const int &index) const {
  if (index < 0 || index >= num_nodes) {  // The node indexes are consecutive integers start at 0.
    fprintf(stderr, "Error in function %s! \nInvalid index [%d]!", __FUNCTION__, index);
    exit(1);
  }
  return map_idx_node_ptr.at(index);
}


Node* Network::FindNodePtrByName(const string &name) const {
  Node* node_ptr = nullptr;
  for (const auto i_n_ptr : map_idx_node_ptr) {
    auto n_ptr = i_n_ptr.second;
    if (n_ptr->node_name==name) {
      node_ptr = n_ptr;
      break;
    }
  }
  return node_ptr;
}


void Network::ConstructNaiveBayesNetwork(Dataset *dts) {
  num_nodes = dts->num_vars;
  // Assign an index for each node.
#pragma omp parallel for
  for (int i = 0; i < num_nodes; ++i) {
    DiscreteNode *node_ptr = new DiscreteNode(i);  // For now, only support discrete node.
    node_ptr->SetDomainSize(dts->num_of_possible_values_of_disc_vars[i]);
    for (auto v : dts->map_disc_vars_possible_values[i]) {
      node_ptr->vec_potential_vals.push_back(v);
    }
#pragma omp critical
    {
      map_idx_node_ptr[i] = node_ptr;
    }
  }

  // Set parents and children.
  Node *class_node_ptr = FindNodePtrByIndex(dts->class_var_index);
  for (auto &i_n : map_idx_node_ptr) {
    if (i_n.second == class_node_ptr) { continue; }
    SetParentChild(class_node_ptr, i_n.second);
  }

  // Generate configurations of parents.
  GenDiscParCombsForAllNodes();

  // Generate topological ordering and default elimination ordering.
  vector<int> topo = GetTopoOrd();
  vec_default_elim_ord.reserve(num_nodes - 1);
#pragma omp parallel for
  for (int i = 0; i < num_nodes-1; ++i) {
    vec_default_elim_ord.push_back(topo.at(num_nodes-1-i));
  }
}


void Network::StructLearnCompData(Dataset *dts, bool print_struct, string topo_ord_constraint) {
  fprintf(stderr, "Not be implemented yet!");

  cout << "==================================================" << '\n'
       << "Begin structural learning with complete data......" << endl;

  struct timeval start, end;
  double diff;
  gettimeofday(&start,NULL);

  num_nodes = dts->num_vars;
  // Assign an index for each node.
  #pragma omp parallel for
  for (int i = 0; i < num_nodes; ++i) {
    DiscreteNode *node_ptr = new DiscreteNode(i);  // For now, only support discrete node.
    if (dts->vec_var_names.size() == num_nodes) {
      node_ptr->node_name = dts->vec_var_names.at(i);
    } else {
      node_ptr->node_name = to_string(i);
    }
    node_ptr->SetDomainSize(dts->num_of_possible_values_of_disc_vars[i]);
    for (auto v : dts->map_disc_vars_possible_values[i]) {
      node_ptr->vec_potential_vals.push_back(v);
    }
    #pragma omp critical
    {
      map_idx_node_ptr[i] = node_ptr;
    }
  }

  vector<int> ord;
  ord.reserve(num_nodes);
  for (int i=0; i<num_nodes; ++i) {
    ord.push_back(i);   // Because the nodes are created the same order as in the dataset.
  }
  cout << "topo_ord_constraint: " << topo_ord_constraint << endl;
  if (topo_ord_constraint == "dataset-ord") {
    StructLearnByOtt(dts, ord);
  } else if (topo_ord_constraint == "random") {
    std::srand ( unsigned ( std::time(0) ) );
    std::random_shuffle ( ord.begin(), ord.end() );
    StructLearnByOtt(dts, ord);
  } else if (topo_ord_constraint == "best") {
    StructLearnByOtt(dts);
  } else {
    fprintf(stderr, "Error in function [%s]!\nInvalid topological ordering restriction!", __FUNCTION__);
    exit(1);
  }


  cout << "==================================================" << '\n'
       << "Finish structural learning." << endl;

  gettimeofday(&end,NULL);
  diff = (end.tv_sec-start.tv_sec) + ((double)(end.tv_usec-start.tv_usec))/1.0E6;
  setlocale(LC_NUMERIC, "");
  cout << "==================================================" << '\n'
       << "The time spent to learn the structure is " << diff << " seconds" << endl;


  if (print_struct) {

    cout << "==================================================" << '\n'
         << "Topological sorted permutation generated using width-first-traversal: " << endl;
    auto topo = GetTopoOrd();
    for (int m = 0; m < num_nodes; ++m) {
      cout << topo.at(m) << ", ";
    }
    cout << endl;

    cout << "==================================================" << '\n'
         << "Each node's parents: " << endl;
    this->PrintEachNodeParents();

  }

}


void Network::AddNode(Node *node_ptr) {
  map_idx_node_ptr[node_ptr->GetNodeIndex()] = node_ptr;
  ++num_nodes;
}


void Network::RemoveNode(int node_index) {
  map_idx_node_ptr.erase(node_index);
  --num_nodes;
}


void Network::SetParentChild(int p_index, int c_index) {
  Node *p = FindNodePtrByIndex(p_index), *c = FindNodePtrByIndex(c_index);
  SetParentChild(p,c);
}


void Network::SetParentChild(Node *p, Node *c) {
  if (map_idx_node_ptr.find(p->GetNodeIndex()) == map_idx_node_ptr.end()
      ||
      map_idx_node_ptr.find(c->GetNodeIndex())==map_idx_node_ptr.end()) {
    fprintf(stderr, "Error in function [%s].\nThe nodes [%d] and [%d] do not belong to this network!",
            __FUNCTION__, p->GetNodeIndex(), c->GetNodeIndex());
    exit(1);
  }
  p->AddChild(c);
  c->AddParent(p);
}


void Network::RemoveParentChild(int p_index, int c_index) {
  Node *p = FindNodePtrByIndex(p_index), *c = FindNodePtrByIndex(c_index);
  RemoveParentChild(p,c);
}

void Network::RemoveParentChild(Node *p, Node *c) {
  if (map_idx_node_ptr.find(p->GetNodeIndex()) == map_idx_node_ptr.end()
      ||
      map_idx_node_ptr.find(c->GetNodeIndex())==map_idx_node_ptr.end()) {
    fprintf(stderr, "The nodes do not belong to this network!");
    exit(1);
  }
  p->RemoveChild(c);
  c->RemoveParent(p);
}

set<Node*> Network::GetParentPtrsOfNode(int node_index) {
  set<Node*> set_par_ptrs;
  Node *node = map_idx_node_ptr.at(node_index);
  for (const auto &idx : node->set_parent_indexes) {
    set_par_ptrs.insert(map_idx_node_ptr.at(idx));
  }
  return set_par_ptrs;
}

set<Node*> Network::GetChildrenPtrsOfNode(int node_index) {
  set<Node*> set_chi_ptrs;
  Node *node = map_idx_node_ptr.at(node_index);
  for (const auto &idx : node->set_children_indexes) {
    set_chi_ptrs.insert(map_idx_node_ptr.at(idx));
  }
  return set_chi_ptrs;
}


vector<int> Network::ConvertInstanceIntValuesToValueIndexesOfDiscreteNodes(vector<int> instance) {
  assert(instance.size() >= this->num_nodes);
  vector<int> result = instance;
#pragma omp parallel for
  for (int i = 0; i < this->num_nodes; ++i) {
    int val = instance.at(i);
    Node *node_ptr = FindNodePtrByIndex(i);
    if (node_ptr->is_discrete) {
      auto d_node_ptr = (DiscreteNode*)node_ptr;
      int val_index = d_node_ptr->GetIndexOfValue(val);
      result.at(i) = val_index;
    }
  }
  return result;
}


void Network::GenDiscParCombsForAllNodes() {
  for (auto id_np : this->map_idx_node_ptr) {
    auto np = id_np.second;
    np->GenDiscParCombs(GetParentPtrsOfNode(np->GetNodeIndex()));
  }
}

vector<int> Network::GetTopoOrd() {
  if (topo_ord.empty()) {
    this->GenTopoOrd();
  }
  return topo_ord;
}

vector<int> Network::GetReverseTopoOrd() {
  auto ord = this->GetTopoOrd();
  reverse(ord.begin(), ord.end());
  return ord;
}

vector<int> Network::GenTopoOrd() {

  if (this->pure_discrete) {

    // First, convert the network to a directed adjacency matrix.
    int **graph = new int*[num_nodes];
    #pragma omp for
    for (int i=0; i<num_nodes; ++i) {graph[i] = new int[num_nodes]();}
    for (auto &i_n_p : map_idx_node_ptr) {
      auto n_p = i_n_p.second;
      for (auto &c_p : GetChildrenPtrsOfNode(n_p->GetNodeIndex())) {
        graph[n_p->GetNodeIndex()][c_p->GetNodeIndex()] = 1;
      }
    }

    topo_ord = TopoSortOfDAGZeroInDegreeFirst(graph, num_nodes);

    for (int i=0; i<num_nodes; ++i) { delete[] graph[i]; }
    delete[] graph;

  } else {  

    // If the network is not pure discrete, then it is conditional Gaussian.
    // Discrete nodes should not have continuous parents.
    // Continuous nodes can have both discrete and continuous parents.
    // In topological ordering, all discrete nodes should occur before any continuous node.
    // todo: test correctness of the case of Gaussian network

    set<Node*> set_disc_node_ptr, set_cont_node_ptr;
    for (const auto &i_n_p : map_idx_node_ptr) {
      auto n_p = i_n_p.second;
      if (n_p->is_discrete) {
        set_disc_node_ptr.insert(n_p);
      } else {
        set_cont_node_ptr.insert(n_p);
      }
    }
    int **graph_disc = new int*[set_disc_node_ptr.size()];
    int **graph_cont = new int*[set_cont_node_ptr.size()];
    for (int i=0; i<set_disc_node_ptr.size(); ++i) {
      graph_disc[i] = new int[set_disc_node_ptr.size()]();
    }
    for (int i=0; i<set_cont_node_ptr.size(); ++i) {
      graph_disc[i] = new int[set_disc_node_ptr.size()]();
      graph_cont[i] = new int[set_cont_node_ptr.size()]();
    }

    // Generate the ordering for discrete nodes.
    map<int, int> disc_order_index, disc_index_order;
    int disc_ord = 0;
    for (const auto &n_p : set_disc_node_ptr) {
      disc_order_index[disc_ord] = n_p->GetNodeIndex();
      disc_index_order[n_p->GetNodeIndex()] = disc_ord;
      ++disc_ord;
    }
    for (const auto &n_p : set_disc_node_ptr) {
      for (const auto &c_p : GetChildrenPtrsOfNode(n_p->GetNodeIndex())) {
        if (!c_p->is_discrete) { continue; }
        graph_disc[ disc_index_order[n_p->GetNodeIndex()] ]
                  [ disc_index_order[c_p->GetNodeIndex()] ] = 1;
      }
    }
    vector<int> topo_ord_disc = TopoSortOfDAGZeroInDegreeFirst(graph_disc, set_disc_node_ptr.size());

    // Generate the ordering for continuous nodes.
    map<int, int> cont_order_index, cont_index_order;
    int cont_ord = 0;
    for (const auto &n_p : set_cont_node_ptr) {
      cont_order_index[cont_ord] = n_p->GetNodeIndex();
      cont_index_order[n_p->GetNodeIndex()] = cont_ord;
      ++cont_ord;
    }
    for (const auto &n_p : set_cont_node_ptr) {
      for (const auto &c_p : GetChildrenPtrsOfNode(n_p->GetNodeIndex())) {
        graph_cont[ cont_index_order[n_p->GetNodeIndex()] ]
                  [ cont_index_order[c_p->GetNodeIndex()] ] = 1;
      }
    }
    vector<int> topo_ord_cont = TopoSortOfDAGZeroInDegreeFirst(graph_cont, set_cont_node_ptr.size());
    // Restore the index from the ordering.
    for (auto &o : topo_ord_cont) {
      o = cont_order_index[o];
    }

    // Concatinate topo_ord_disc and topo_ord_cont.
    topo_ord_disc.insert(topo_ord_disc.end(), topo_ord_cont.begin(), topo_ord_cont.end());
    this->topo_ord = topo_ord_disc;

    for (int i=0; i<set_disc_node_ptr.size(); ++i) {
      delete[] graph_disc[i];
    }
    for (int i=0; i<set_cont_node_ptr.size(); ++i) {
      delete[] graph_cont[i];
    }
    delete[] graph_disc;
    delete[] graph_cont;

  }

  return topo_ord;
}


int** Network::ConvertDAGNetworkToAdjacencyMatrix() {
  int **adjac_matrix = new int* [num_nodes];
  for (int i=0; i<num_nodes; ++i) {
    adjac_matrix[i] = new int[num_nodes]();
  }
  for (auto &id_node_ptr : map_idx_node_ptr) {
    auto node_ptr = id_node_ptr.second;
    int from, from2, to;
    from = node_ptr->GetNodeIndex();
    for (auto &child_ptr : GetChildrenPtrsOfNode(node_ptr->GetNodeIndex())) {
      to = child_ptr->GetNodeIndex();
      adjac_matrix[from][to] = 1;
    }
  }
  return adjac_matrix;
}


void Network::LearnParamsKnowStructCompData(const Dataset *dts, int alpha, bool print_params){
  cout << "==================================================" << '\n'
       << "Begin learning parameters with known structure and complete data." << '\n'
       << "Laplace smoothing param: alpha = " << alpha << endl;

  struct timeval start, end;
  double diff;
  gettimeofday(&start,NULL);

  int num_cores = omp_get_num_procs();
  omp_set_num_threads(num_cores);
  int max_work_per_thread = (dts->num_vars + num_cores - 1) / num_cores;
//  #pragma omp parallel
  {
    // For every node.
//    for (int i = max_work_per_thread*omp_get_thread_num();
//         i < max_work_per_thread*(omp_get_thread_num()+1) && i < dts->num_vars;
//         ++i) {
    for (int i=0; i<dts->num_vars; ++i) {
      DiscreteNode *this_node = dynamic_cast<DiscreteNode*>(FindNodePtrByIndex(i));   // todo: support continuous node


      for (int s = 0; s < dts->num_instance; ++s) {
        vector<int> var_val_indexes = ConvertInstanceIntValuesToValueIndexesOfDiscreteNodes(vector<int>(dts->dataset_all_vars[s], dts->dataset_all_vars[s]+dts->num_vars));
        this_node->AddInstanceOfValueIndex(var_val_indexes);
      }


      if (!this_node->HasParents()) {

        map<int, double> *MPT = &(dynamic_cast<DiscreteNode*>(this_node)->map_marg_prob_table);
        int denominator = dts->num_instance;
        for (int s = 0; s < dts->num_instance; ++s) {
          int query = dts->dataset_all_vars[s][i];
          (*MPT)[query] += 1;
        }
        for (int ii = 0; ii < this_node->GetDomainSize(); ++ii) {
          int query = this_node->vec_potential_vals.at(ii);
          // Laplace smoothing.
          (*MPT)[query] = ((*MPT)[query] + alpha) / (denominator + alpha * this_node->GetDomainSize());
        }

        // todo: delete the for loop below
        for (auto query : this_node->vec_potential_vals) {
          cout << this_node->GetNodeIndex() << ":\t";
          cout << (*MPT)[query] << '\t' << this_node->GetProbability(this_node->GetIndexOfValue(query), 0) << endl;
        }

      } else {  // If the node has parents.

        map<int, map<DiscreteConfig, double> > *CPT = &(dynamic_cast<DiscreteNode*>(this_node)->map_cond_prob_table);
        set<DiscreteConfig> *ptr_set_par_combs = &(this_node->set_discrete_parents_combinations);
        for (auto &par_comb : *ptr_set_par_combs) {    // For each column in CPT. Because the sum over column of CPT must be 1.
          int denominator = 0;
          for (int s = 0; s < dts->num_instance; ++s) {
            int compatibility = 1;  // We assume compatibility is 1,
            // and set it to 0 if we find that (*it_par_comb) is not compatible.
            // If we support learning with incomplete data,
            // the compatibility can be between 0 and 1.

            for (const auto &index_value : par_comb) {
              if (dts->dataset_all_vars[s][index_value.first] != index_value.second) {
                compatibility = 0;
                break;
              }
            }
            denominator += compatibility;
            int query = dts->dataset_all_vars[s][i];
            (*CPT)[query][par_comb] += compatibility;
          }
          // Normalize so that the sum is 1.
          for (int j = 0; j < this_node->GetDomainSize(); ++j) {
            int query = this_node->vec_potential_vals.at(j);
            // Laplace smoothing.
            (*CPT)[query][par_comb] = ((*CPT)[query][par_comb] + alpha) / (denominator + alpha * this_node->GetDomainSize());
          }
        }

        // todo: delete the for loop below
        for (auto query : this_node->vec_potential_vals) {
          cout << this_node->GetNodeIndex() << ":\t";
          cout << (*CPT)[query][*(this_node->set_discrete_parents_combinations.begin())] << '\t' << this_node->GetProbability(this_node->GetIndexOfValue(query), 0) << endl;
        }

      }
    }
  }   // end of: #pragma omp parallel
  cout << "==================================================" << '\n'
       << "Finish training with known structure and complete data." << endl;

  if (print_params) {
    cout << "==================================================" << '\n'
         << "Each node's conditional probability table: " << endl;
    for (const auto &id_node_ptr : map_idx_node_ptr) {  // For each node
      dynamic_cast<DiscreteNode*>(id_node_ptr.second)->PrintProbabilityTable();
    }
  }

  gettimeofday(&end,NULL);
  diff = (end.tv_sec-start.tv_sec) + ((double)(end.tv_usec-start.tv_usec))/1.0E6;
  setlocale(LC_NUMERIC, "");
  cout << "==================================================" << '\n'
       << "The time spent to learn the parameters is " << diff << " seconds" << endl;

}


int Network::GetNumParams() const {
  int result = 0;
  for (const auto &i_n : map_idx_node_ptr) {
    result += i_n.second->GetNumParams();
  }
  return result;
}


void Network::ClearStructure() {
  for (auto &i_n_p : this->map_idx_node_ptr) {
    i_n_p.second->ClearParams();
    i_n_p.second->ClearParents();
    i_n_p.second->ClearChildren();
  }
}


void Network::ClearParams() {
  for (auto &i_n_p : this->map_idx_node_ptr) {
    i_n_p.second->ClearParams();
  }
}


vector<int> Network::SimplifyDefaultElimOrd(DiscreteConfig) {
  fprintf(stderr, "Function [%s] not implemented yet!", __FUNCTION__);
  exit(1);
}

DiscreteConfig Network::ConstructEvidence(int *nodes_indexes, int *observations, int num_of_observations) {
  DiscreteConfig result;
  pair<int, int> p;
  for (int i=0; i<num_of_observations; ++i) {
    p.first = nodes_indexes[i];
    p.second = observations[i];
    result.insert(p);
  }
  return result;
}


vector<Factor> Network::ConstructFactors(vector<int> Z, Node *Y) {
  vector<Factor> factors_list;
  factors_list.push_back(Factor(dynamic_cast<DiscreteNode*>(Y), this));
  for (int i = 0; i < Z.size(); ++i) {
    Node* n = FindNodePtrByIndex(Z.at(i));
    factors_list.push_back(Factor(dynamic_cast<DiscreteNode*>(n), this));
  }
  return factors_list;
}


void Network::LoadEvidenceIntoFactors(vector<Factor> *factors_list, DiscreteConfig E, set<int> all_related_vars) {

  // I do not know why this function cannot use omp to parallel.
  // If I set number of threads more than 1, the accuracy will decrease!

//  int num_cores = omp_get_num_procs();
//  omp_set_num_threads(num_cores);
//  int max_work_per_thread = (factors_list->size()+num_cores-1)/num_cores;
//  #pragma omp parallel
  {
//    for (int i = omp_get_thread_num() * max_work_per_thread;
//         i < (omp_get_thread_num()+1) * max_work_per_thread && i < factors_list->size();
//         ++i) {
    for (int i = 0; i < factors_list->size(); ++i) {
      Factor &f = factors_list->at(i);   // For each factor. "std::vector::at" returns reference.
      for (const auto &e : E) {  // For each node's observation in E
        // If this factor is related to this node
        if (f.related_variables.find(e.first) != f.related_variables.end()) {
          // Update each row of map_potentials
          for (const auto &comb : f.set_combinations) {
            // If this entry is not compatible to the evidence.
            if (comb.find(e) == comb.end()) {
              f.map_potentials[comb] = 0;
            }
          }
        }
      }

      //--------------------------------------------------------------------------------
      // This block is to fix the bug occurring when the target node
      // is not the root and the variable elimination order do not start at root.
      // For example:  A --> B --> C
      // When given the markov blanket of node "C", which is "{B}",
      // there is no need to calculate the other nodes, which is "{A}".
      // However, when using this function,
      // the parent of parent of this node, which is "A",
      // still appears in the constructed factor of the parent which is "B".
      // todo: check correctness
      set<int> related_vars_of_f = f.related_variables;
      for (auto &v : related_vars_of_f) {
        if (all_related_vars.find(v) == all_related_vars.end()) {
          f.CopyFactor(f.SumOverVar(v));
        }
      }
      //--------------------------------------------------------------------------------
    }

  }   // end of: #pragma omp parallel
}


Factor Network::SumProductVarElim(vector<Factor> factors_list, vector<int> Z) {
  for (int i = 0; i < Z.size(); ++i) {
    vector<Factor> tempFactorsList;
    Node* nodePtr = FindNodePtrByIndex(Z.at(i));
    // Move every factor that is related to the node Z[i] from factors_list to tempFactorsList.
    /*
     * Note: This for loop does not contain "++it" in the parentheses.
     *      Because if we do so, it may cause some logic faults which,
     *      however, may or may not cause runtime error, causing the program hard to debug.
     *      For example:
     *        When "it" reaches the second to last element, and this element is related to the node.
     *        Then this element will be erase from factors_list,
     *        and then "++it" which will move "it" to the end.
     *        Then the for loop will end because "it" has reached the end.
     *        However, at this time, the last element has been ignored,
     *        even if it is related to the node.
     */
    for (auto it=factors_list.begin(); it!=factors_list.end(); /* no ++it */) {
      if ((*it).related_variables.find(nodePtr->GetNodeIndex())!=(*it).related_variables.end()) {
        tempFactorsList.push_back(*it);
        factors_list.erase(it);
        continue;
      } else {
        ++it;
      }
    }
    while(tempFactorsList.size()>1) {
      Factor temp1, temp2, product;
      temp1 = tempFactorsList.back();
      tempFactorsList.pop_back();
      temp2 = tempFactorsList.back();
      tempFactorsList.pop_back();
      product = temp1.MultiplyWithFactor(temp2);
      tempFactorsList.push_back(product);
    }
    Factor newFactor = tempFactorsList.back().SumOverVar(dynamic_cast<DiscreteNode*>(nodePtr));
    factors_list.push_back(newFactor);
  }

  /*
   *   If we are calculating a node's posterior probability given evidence about its children,
   *   then when the program runs to here,
   *   the "factors_list" will contain several factors
   *   about the same node which is the query node Y.
   *   When it happens, we need to multiply these several factors.
   */
  while (factors_list.size()>1) {
    Factor temp1, temp2, product;
    temp1 = factors_list.back();
    factors_list.pop_back();
    temp2 = factors_list.back();
    factors_list.pop_back();
    product = temp1.MultiplyWithFactor(temp2);
    factors_list.push_back(product);
  }
  // After all the processing shown above, the only remaining factor is the factor about Y.
  return factors_list.back();
}


Factor Network::VarElimInferReturnPossib(vector<int> Z, DiscreteConfig E, Node *Y) {
  // Z is the array of variable elimination order.
  // E is the evidences.
  vector<Factor> factorsList = ConstructFactors(Z, Y);

  //--------------------------------------------------------------------------------
  // This block is to fix the bug occurring when the target node
  // is not the root and the variable elimination order do not start at root.
  // For example:  A --> B --> C
  // When given the markov blanket, which is "{B}", of node "C",
  // there is no need to calculate the other nodes, which is "{A}".
  // However, when using this function,
  // the parent of parent of this node, which is "A",
  // still appears in the constructed factor of the parent which is "B".
  set<int> all_related_vars;
  all_related_vars.insert(Y->GetNodeIndex());
  for (int i = 0; i < Z.size(); ++i) { all_related_vars.insert(Z.at(i)); }
  //--------------------------------------------------------------------------------


  LoadEvidenceIntoFactors(&factorsList, E, all_related_vars);
  Factor F = SumProductVarElim(factorsList, Z);
  F.Normalize();
  return F;
}


Factor Network::VarElimInferReturnPossib(DiscreteConfig E, Node *Y) {
  vector<int> vec_simplified_elimination_order = SimplifyDefaultElimOrd(E);
  int *simplified_elimination_order = new int[vec_simplified_elimination_order.size()];
  for (int i = 0; i < vec_simplified_elimination_order.size(); ++i) {
    simplified_elimination_order[i] = vec_simplified_elimination_order.at(i);
  }
//  pair<int*, int> simplified_elimination_order = pair<int*, int>(default_elim_ord, num_nodes-1);
  return this->VarElimInferReturnPossib(
                  vec_simplified_elimination_order,
                  E,
                  Y
               );
}


int Network::PredictUseVarElimInfer(vector<int> Z, DiscreteConfig E, int Y_index) {
  Node *Y = FindNodePtrByIndex(Y_index);
  Factor F = VarElimInferReturnPossib(Z, E, Y);
  double max_prob = 0;
  DiscreteConfig comb_predict;
  for (auto &comb : F.set_combinations) {
    if (F.map_potentials[comb] > max_prob) {
      max_prob = F.map_potentials[comb];
      comb_predict = comb;
    }
  }
  int label_predict = comb_predict.begin()->second;
  return label_predict;
}


int Network::PredictUseVarElimInfer(DiscreteConfig E, int Y_index) {
  Node *Y = FindNodePtrByIndex(Y_index);
  Factor F = VarElimInferReturnPossib(E, Y);

  double max_prob = 0;
  DiscreteConfig comb_predict;
  for (auto &comb : F.set_combinations) {
    if (F.map_potentials[comb] > max_prob) {
      max_prob = F.map_potentials[comb];
      comb_predict = comb;
    }
  }
  int label_predict = comb_predict.begin()->second;
  return label_predict;
}


double Network::TestNetReturnAccuracy(Dataset *dts) {

  cout << "==================================================" << '\n'
       << "Begin testing the trained network." << endl;

  struct timeval start, end;
  double diff;
  gettimeofday(&start,NULL);

  cout << "Progress indicator: ";
  int num_of_correct=0, num_of_wrong=0, m=dts->num_instance, m20= m / 20, progress=0;

  int class_var_index = dts->class_var_index;


  // For each sample in test set
  #pragma omp parallel for
  for (int i=0; i<m; ++i) {

    #pragma omp critical
    { ++progress; }
    string progress_detail = to_string(progress) + '/' + to_string(m);
    fprintf(stdout, "%s\n", progress_detail.c_str());
    fflush(stdout);

    if (progress % m20 == 0) {
      string progress_percentage = to_string((double)progress/m * 100) + "%...\n";
      fprintf(stdout, "Progress: %s\n", progress_percentage.c_str());
      double acc_so_far = num_of_correct / (double)(num_of_correct+num_of_wrong);
      fprintf(stdout, "Accuracy so far: %f\n", acc_so_far);
      fflush(stdout);
    }

    // For now, only support complete data.
    int e_num = num_nodes - 1, *e_index = new int[e_num], *e_value = new int[e_num];
    for (int j = 0; j < num_nodes; ++j) {
      if (j == class_var_index) {continue;}
      e_index[j < class_var_index ? j : j - 1] = j;
      e_value[j < class_var_index ? j : j - 1] = dts->dataset_all_vars[i][j];
    }
    DiscreteConfig E = ConstructEvidence(e_index, e_value, e_num);
    int label_predict = PredictUseVarElimInfer(E, class_var_index);
//    string pred_true = to_string(label_predict) + ':' + to_string(dts->dataset_all_vars[i][class_var_index]);
//    fprintf(stdout, "%s\n", pred_true.c_str());
//    fflush(stdout);
    if (label_predict == dts->dataset_all_vars[i][class_var_index]) {
      #pragma omp critical
      { ++num_of_correct; }
    } else {
      #pragma omp critical
      { ++num_of_wrong; }
    }

    delete[] e_index;
    delete[] e_value;
  }

  gettimeofday(&end,NULL);
  diff = (end.tv_sec-start.tv_sec) + ((double)(end.tv_usec-start.tv_usec))/1.0E6;
  setlocale(LC_NUMERIC, "");
  cout << "==================================================" << '\n'
       << "The time spent to test the accuracy is " << diff << " seconds" << endl;

  double accuracy = num_of_correct / (double)(num_of_correct+num_of_wrong);
  cout << '\n' << "Accuracy: " << accuracy << endl;
  return accuracy;
}


double Network::TestNetByApproxInferReturnAccuracy(Dataset *dts, int num_samp) {


  cout << "==================================================" << '\n'
       << "Begin testing the trained network." << endl;

  struct timeval start, end;
  double diff;
  gettimeofday(&start,NULL);

  cout << "Progress indicator: ";

  int num_of_correct=0, num_of_wrong=0, m=dts->num_instance, m20= m / 20, progress=0;

  int class_var_index = dts->class_var_index;

  vector<DiscreteConfig> samples = this->DrawSamplesByProbLogiSamp(num_samp);
  cout << "Finish drawing samples." << endl;

  #pragma omp parallel for
  for (int i=0; i<m; ++i) {  // For each sample in test set

    #pragma omp critical
    { ++progress; }
    string progress_detail = to_string(progress) + '/' + to_string(m);
    fprintf(stdout, "%s\n", progress_detail.c_str());
    fflush(stdout);

    if (progress % m20 == 0) {
      string progress_percentage = to_string((double)progress/m * 100) + "%...\n";
      fprintf(stdout, "Progress: %s\n", progress_percentage.c_str());
      double acc_so_far = num_of_correct / (double)(num_of_correct+num_of_wrong);
      fprintf(stdout, "Accuracy so far: %f\n", acc_so_far);
      fflush(stdout);
    }

    // For now, only support complete data.
    int e_num=num_nodes-1, *e_index=new int[e_num], *e_value=new int[e_num];
    for (int j=0; j<num_nodes; ++j) {
      if (j == class_var_index) { continue; }
      e_index[j < class_var_index ? j : j - 1] = j + 1;
      e_value[j < class_var_index ? j : j - 1] = dts->dataset_all_vars[i][j];
    }
    DiscreteConfig E = ConstructEvidence(e_index, e_value, e_num);
    int label_predict = ApproxInferByProbLogiRejectSamp(E, class_var_index, samples);
//    string pred_true = to_string(label_predict) + ':' + to_string(dts->dataset_all_vars[i][class_var_index]);
//    fprintf(stdout, "%s\n", pred_true.c_str());
//    fflush(stdout);
    if (label_predict == dts->dataset_all_vars[i][class_var_index]) {
      ++num_of_correct;
    } else {
      ++num_of_wrong;
    }

  }

  gettimeofday(&end,NULL);
  diff = (end.tv_sec-start.tv_sec) + ((double)(end.tv_usec-start.tv_usec))/1.0E6;
  setlocale(LC_NUMERIC, "");
  cout << "==================================================" << '\n'
       << "The time spent to test the accuracy is " << diff << " seconds" << endl;

  double accuracy = num_of_correct / (double)(num_of_correct+num_of_wrong);
  cout << '\n' << "Accuracy: " << accuracy << endl;
  return accuracy;
}

double Network::TestAccuracyByLikelihoodWeighting(Dataset *dts, int num_samp) {
  cout << "==================================================" << '\n'
       << "Begin testing the trained network." << endl;

  struct timeval start, end;
  double diff;
  gettimeofday(&start,NULL);

  cout << "Progress indicator: ";

  int num_of_correct=0, num_of_wrong=0, m=dts->num_instance, m20= m / 20, progress=0;

  int class_var_index = dts->class_var_index;

  #pragma omp parallel for
  for (int i=0; i<m; ++i) {  // For each sample in test set

    #pragma omp critical
    { ++progress; }
    string progress_detail = to_string(progress) + '/' + to_string(m);
    fprintf(stdout, "%s\n", progress_detail.c_str());
    fflush(stdout);

    if (progress % m20 == 0) {
      string progress_percentage = to_string((double)progress/m * 100) + "%...\n";
      fprintf(stdout, "Progress: %s\n", progress_percentage.c_str());
      double acc_so_far = num_of_correct / (double)(num_of_correct+num_of_wrong);
      fprintf(stdout, "Accuracy so far: %f\n", acc_so_far);
      fflush(stdout);
    }


    // For now, only support complete data.
    int e_num=num_nodes-1, *e_index=new int[e_num], *e_value=new int[e_num];
    for (int j=0; j<num_nodes; ++j) {
      if (j == class_var_index) { continue; }
      e_index[j < class_var_index ? j : j - 1] = j;
      e_value[j < class_var_index ? j : j - 1] = dts->dataset_all_vars[i][j];
    }
    DiscreteConfig E = ConstructEvidence(e_index, e_value, e_num);
    int label_predict = ApproxinferByLikelihoodWeighting(E, class_var_index, num_samp);
//    string pred_true = to_string(label_predict) + ':' + to_string(dts->dataset_all_vars[i][class_var_index]);
//    fprintf(stdout, "%s\n", pred_true.c_str());
//    fflush(stdout);
    if (label_predict == dts->dataset_all_vars[i][class_var_index]) {
      #pragma omp critical
      { ++num_of_correct; }
    } else {
      #pragma omp critical
      { ++num_of_wrong; }
    }

    delete[] e_index;
    delete[] e_value;

  }

  gettimeofday(&end,NULL);
  diff = (end.tv_sec-start.tv_sec) + ((double)(end.tv_usec-start.tv_usec))/1.0E6;
  setlocale(LC_NUMERIC, "");
  cout << "==================================================" << '\n'
       << "The time spent to test the accuracy is " << diff << " seconds" << endl;

  double accuracy = num_of_correct / (double)(num_of_correct+num_of_wrong);
  cout << '\n' << "Accuracy: " << accuracy << endl;
  return accuracy;
}



DiscreteConfig Network::ProbLogicSampleNetwork() {
  DiscreteConfig instance;
  // Cannot use OpenMP, because must draw samples in the topological ordering.
  for (const auto &index : this->GetTopoOrd()) {
    Node *n_p = FindNodePtrByIndex(index);
    int drawn_value = dynamic_cast<DiscreteNode*>(n_p)->SampleNodeGivenParents(instance); // todo: support continuous nodes
    instance.insert(pair<int,int>(index, drawn_value));
  }
  return instance;
}

pair<DiscreteConfig, double> Network::DrawOneLikelihoodWeightingSample(const DiscreteConfig &evidence) {
  DiscreteConfig instance;
  double weight = 1;
  // SHOULD NOT use OpenMP, because must draw samples in the topological ordering.
  for (const auto &index : this->GetTopoOrd()) {
    Node *n_p = FindNodePtrByIndex(index);
    bool observed = false;
    for (const auto &p : evidence) {
      if (index==p.first) {
        observed = true;
        // Set the sample value to be the same as the evidence.
        instance.insert(pair<int, int>(index, p.second));
        // Update the weight.
        if(n_p->HasParents()) {
          set<int> parents_indexes;
          for (const auto &par : GetParentPtrsOfNode(n_p->GetNodeIndex())) {
            parents_indexes.insert(par->GetNodeIndex());
          }
          DiscreteConfig parents_index_value;
          for (const auto &i : instance) {
            if (parents_indexes.find(i.first) != parents_indexes.end()) {
              parents_index_value.insert(i);
            }
          }
          weight *= dynamic_cast<DiscreteNode*>(n_p)->map_cond_prob_table[p.second][parents_index_value];
        } else {
          weight *= dynamic_cast<DiscreteNode*>(n_p)->GetProbability(dynamic_cast<DiscreteNode*>(n_p)->GetIndexOfValue(p.second), 0);//map_marg_prob_table[p.second];
        }
        break;
      }
    }
    if (!observed) {
      int drawn_value = dynamic_cast<DiscreteNode*>(n_p)->SampleNodeGivenParents(instance);   // todo: Consider continuous nodes
      instance.insert(pair<int,int>(index, drawn_value));
    }
  }
  return pair<DiscreteConfig, double>(instance, weight);
}


vector<pair<DiscreteConfig, double>> Network::DrawSamplesByLikelihoodWeighting(const DiscreteConfig &evidence, int num_samp) {
  vector<pair<DiscreteConfig, double>> results;
  #pragma omp parallel for
  for (int i=0; i<num_samp; ++i) {
    auto samp = DrawOneLikelihoodWeightingSample(evidence);
    #pragma omp critical
    { results.push_back(samp); }
  }
  return results;
}

Factor Network::CalcuMargWithLikelihoodWeightingSamples(const vector<pair<DiscreteConfig, double>> &samples, const int &node_index) {
  map<int, double> value_weight;
  DiscreteNode *n_p = dynamic_cast<DiscreteNode*>(this->FindNodePtrByIndex(node_index));

  // Initialize the map.
  for (int i=0; i<n_p->GetDomainSize(); ++i) {
    value_weight[n_p->vec_potential_vals.at(i)] = 0;
  }

  // Calculate the sum of weight for each value. Un-normalized.
  for (const auto &samp : samples) {
    for (const auto &feature_value : samp.first) {
      if (node_index==feature_value.first) {
        value_weight[feature_value.second] += samp.second;
        break;
      }
    }
  }

  // Normalization.
  double denominator = 0;
  for (const auto &kv : value_weight) {
    denominator += kv.second;
  }
  for (auto &kv : value_weight) {
    kv.second /= denominator;
  }

  // Construct a factor to return
  Factor f;
  set<int> rv;
  rv.insert(node_index);
  set<DiscreteConfig> sc;
  for (int i=0; i<n_p->GetDomainSize(); ++i) {
    DiscreteConfig c;
    c.insert(pair<int, int>(node_index, n_p->vec_potential_vals.at(i)));
    sc.insert(c);
  }
  map<DiscreteConfig, double> mp;
  for (const auto &c : sc) {
    int value = (*c.begin()).second;
    mp[c] = value_weight[value];
  }
  f.SetMembers(rv, sc, mp);
  return f;
}


int Network::ApproxinferByLikelihoodWeighting(DiscreteConfig e, const int &node_index, const int &num_samp) {
  vector<pair<DiscreteConfig, double>> samples_weight = this->DrawSamplesByLikelihoodWeighting(e, num_samp);
  Factor f = CalcuMargWithLikelihoodWeightingSamples(samples_weight, node_index);
  // Find the argmax.
  DiscreteConfig c;
  double max = -1;
  for (const auto &kv : f.map_potentials) {
    if (kv.second > max) {
      c = kv.first;
      max = kv.second;
    }
  }
  return (*c.begin()).second;
}


vector<DiscreteConfig> Network::DrawSamplesByProbLogiSamp(int num_samp) {
  vector<DiscreteConfig> samples;
  samples.reserve(num_samp);
  #pragma omp parallel for
  for (int i=0; i<num_samp; ++i) {
    DiscreteConfig samp = this->ProbLogicSampleNetwork();
    #pragma omp critical
    { samples.push_back(samp); }
  }
  return samples;
}


set<int> Network::GetMarkovBlanketIndexesOfNode(Node *node_ptr) {
  set<int> markov_blanket_node_index;

  // Add parents.
  for (auto &par_ptr : GetParentPtrsOfNode(node_ptr->GetNodeIndex())) {
    markov_blanket_node_index.insert(par_ptr->GetNodeIndex());
  }

  // Add children and parents of children.
  for (auto &chil_ptr : GetChildrenPtrsOfNode(node_ptr->GetNodeIndex())) {
    markov_blanket_node_index.insert(chil_ptr->GetNodeIndex());
    for (auto &par_chil_ptr : GetParentPtrsOfNode(chil_ptr->GetNodeIndex())) {
      markov_blanket_node_index.insert(par_chil_ptr->GetNodeIndex());
    }
  }


  markov_blanket_node_index.erase(node_ptr->GetNodeIndex());

  return markov_blanket_node_index;
}


vector<DiscreteConfig> Network::DrawSamplesByGibbsSamp(int num_samp, int num_burn_in) {

  vector<DiscreteConfig> samples;
  samples.reserve(num_samp);

  DiscreteConfig single_sample = this->ProbLogicSampleNetwork();

  auto it_idx_node = this->map_idx_node_ptr.begin();


  // Need burning in.
//  #pragma omp parallel for
  for (int i=1; i<num_burn_in+num_samp; ++i) {

    Node *node_ptr = (*(it_idx_node++)).second;
    if (it_idx_node == map_idx_node_ptr.end()) {
      it_idx_node = this->map_idx_node_ptr.begin();
    }

    set<int> markov_blanket_node_index = GetMarkovBlanketIndexesOfNode(node_ptr);

    DiscreteConfig markov_blanket;
    for (auto &p : single_sample) {
      if (markov_blanket_node_index.find(p.first)
          !=
          markov_blanket_node_index.end()) {
        markov_blanket.insert(p);
      }
    }

    int value_index =
            SampleNodeGivenMarkovBlanketReturnValIndex(node_ptr,markov_blanket);

    for (auto p : single_sample) {
      if (p.first == node_ptr->GetNodeIndex()) {
        single_sample.erase(p);
        p.second = dynamic_cast<DiscreteNode*>(node_ptr)->vec_potential_vals.at(value_index);
        single_sample.insert(p);
        break;
      }
    }

    // After burning in, we can store the samples now.
    #pragma omp critical
    { if (i >= num_burn_in) { samples.push_back(single_sample); } }
  }

  return samples;
}


int Network::SampleNodeGivenMarkovBlanketReturnValIndex(Node *node_ptr, DiscreteConfig markov_blanket) {
  int num_elim_ord = markov_blanket.size();
  vector<int> var_elim_ord;
  var_elim_ord.reserve(markov_blanket.size());
  for (auto &n_v : markov_blanket) {
    var_elim_ord.push_back(n_v.first);
  }

  Factor f = VarElimInferReturnPossib(var_elim_ord, markov_blanket, node_ptr);

  vector<int> weights;
  for (int i=0; i<dynamic_cast<DiscreteNode*>(node_ptr)->GetDomainSize(); ++i) {
    DiscreteConfig temp;
    temp.insert(pair<int,int>(node_ptr->GetNodeIndex(),dynamic_cast<DiscreteNode*>(node_ptr)->vec_potential_vals.at(i)));
    weights.push_back(f.map_potentials[temp]*10000);
  }

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  default_random_engine rand_gen(seed);
  discrete_distribution<int> this_distribution(weights.begin(),weights.end());
  return this_distribution(rand_gen);
}


int Network::ApproxInferByProbLogiRejectSamp(DiscreteConfig e, Node *node, vector<DiscreteConfig> &samples) {
  DiscreteConfig possb_values;
  for (int i=0; i<dynamic_cast<DiscreteNode*>(node)->GetDomainSize(); ++i) {
    possb_values.insert(pair<int,int>(node->GetNodeIndex(),dynamic_cast<DiscreteNode*>(node)->vec_potential_vals.at(i)));
  }

  int *count_each_value = new int[this->num_nodes]();
  int num_valid_sample = 0;
  for (const auto &samp : samples) {
    if(!Conflict(&e, &samp)) {
      ++num_valid_sample;
      for (const auto &pv : possb_values) {
        if (samp.find(pv)!=samp.end()) {
          ++count_each_value[pv.second];
          break;
        }
      }
    }
  }

  // If there is no valid sample, just take a random guess.
  if (num_valid_sample==0) {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    default_random_engine rand_gen(seed);
    uniform_int_distribution<int> this_distribution(0,dynamic_cast<DiscreteNode*>(node)->GetDomainSize()-1);
    return dynamic_cast<DiscreteNode*>(node)->vec_potential_vals.at(this_distribution(rand_gen));
  }

  // Find the argmax.
  int lable_index_predict = -1;
  int max_occurred = 0;
  for (int i=0; i<dynamic_cast<DiscreteNode*>(node)->GetDomainSize(); ++i) {
    if (lable_index_predict==-1 || count_each_value[i]>max_occurred) {
      lable_index_predict = i;
      max_occurred = count_each_value[i];
    }
  }

  // Return the predicted label instead of the index.
  return dynamic_cast<DiscreteNode*>(node)->vec_potential_vals.at(lable_index_predict);
}


int Network::ApproxInferByProbLogiRejectSamp(DiscreteConfig e, int node_index, vector<DiscreteConfig> &samples) {
  return ApproxInferByProbLogiRejectSamp(e, FindNodePtrByIndex(node_index), samples);
}



pair<double, set<Node*>> Network::F(Node *node,
        set<Node*> &candidate_parents,
        Dataset *dts,
        map<Node*, map<set<Node*>, double>> &dynamic_program_for_F) {

  // The original paper chooses minimum score but this program chooses maximum score.

  map<set<Node*>, double> &this_node_dynamic_program = dynamic_program_for_F[node];

  // Look up the table.
  if (this_node_dynamic_program.find(candidate_parents) != this_node_dynamic_program.end()) {
    return pair<double, set<Node*>>(this_node_dynamic_program[candidate_parents], candidate_parents);
  }

  pair<double, set<Node*>> max_score_parents;
  max_score_parents.first = -DBL_MAX;

  // Look up the dynamic programming table.
  for (auto n : candidate_parents) {
    set<Node*> candidate_parents_temp = candidate_parents;
    candidate_parents_temp.erase(n);

    if (this_node_dynamic_program.find(candidate_parents_temp) == this_node_dynamic_program.end()) {
      this_node_dynamic_program[candidate_parents_temp] = F(node, candidate_parents_temp, dts, dynamic_program_for_F).first;
    }

    if (this_node_dynamic_program[candidate_parents_temp] > max_score_parents.first) {
      max_score_parents.first = this_node_dynamic_program[candidate_parents_temp];
      max_score_parents.second = candidate_parents_temp;
    }
  }

  if (this_node_dynamic_program.find(candidate_parents)==this_node_dynamic_program.end()) {

    DiscreteNode node_copy = *dynamic_cast<DiscreteNode*>(node);
    vector<DiscreteNode> candidate_parents_copy;
    for (auto n : candidate_parents) {
      candidate_parents_copy.push_back(*dynamic_cast<DiscreteNode*>(n));
    }

    Network temp_net;
    temp_net.AddNode(&node_copy);
    for (auto n : candidate_parents_copy) {
      temp_net.AddNode(&n);
      temp_net.SetParentChild(&n, &node_copy);
    }
    temp_net.GenDiscParCombsForAllNodes();

    ScoreFunction sf(&temp_net, dts);
    this_node_dynamic_program[candidate_parents] = sf.ScoreForNode(&node_copy,"log BDeu");

  }

  if (this_node_dynamic_program[candidate_parents] > max_score_parents.first) {
    max_score_parents.first = this_node_dynamic_program[candidate_parents];
    max_score_parents.second = candidate_parents;
  }

  return max_score_parents;
}

pair<double, vector<pair<Node*, set<Node*>>>>
 Network::Q(set<Node*> &set_nodes,
         vector<int> topo_ord,
         Dataset *dts,
         map<Node*,   map<set<Node*>, double>> &dynamic_program_for_F,
         map<pair<set<Node*>, vector<int>>,   pair<double, vector<pair<Node*, set<Node*>>>>> dynamic_program_for_Q) {

  // Look up the table.
  pair<set<Node*>, vector<int>> to_find(set_nodes, topo_ord);
  if (dynamic_program_for_Q.find(to_find)!=dynamic_program_for_Q.end()) {
    return dynamic_program_for_Q[to_find];
  }

  double score = 0;
  vector<pair<Node*, set<Node*>>> vec_node_parents;

  for (auto &n : set_nodes) {
    set<Node*> candidate_parents;
    for (const auto &i : topo_ord) {
      if (n->GetNodeIndex() == i) { break; }
      candidate_parents.insert(this->FindNodePtrByIndex(i));
    }
    pair<double, set<Node*>> best_score_parents = F(n, candidate_parents, dts, dynamic_program_for_F);

    score += best_score_parents.first;
    vec_node_parents.push_back(pair<Node*, set<Node*>>(n, best_score_parents.second));
  }

  return pair<double, vector<pair<Node*, set<Node*>>>>(score, vec_node_parents);
}

vector<int> Network::M(set<Node*> &set_nodes,
        Dataset *dts,
        map<Node*, map<set<Node*>, double>> &dynamic_program_for_F,
        map<pair<set<Node*>, vector<int>>,   pair<double, vector<pair<Node*, set<Node*>>>>> dynamic_program_for_Q,
        map<set<Node*>, vector<int>> dynamic_program_for_M) {

  if (set_nodes.empty()) { return vector<int> {}; }

  // Look up the table.
  if (dynamic_program_for_M.find(set_nodes) != dynamic_program_for_M.end()) {
    return dynamic_program_for_M[set_nodes];
  }

  Node *g_star = nullptr;
  double score = -DBL_MAX;
  for (auto n : set_nodes) {
    set<Node*> set_nodes_temp = set_nodes;
    set_nodes_temp.erase(n);
    vector<int> m_of_set_nodes_temp = M(set_nodes_temp, dts, dynamic_program_for_F, dynamic_program_for_Q, dynamic_program_for_M);
    double score_temp = F(n, set_nodes_temp, dts, dynamic_program_for_F).first + Q(set_nodes_temp, m_of_set_nodes_temp, dts, dynamic_program_for_F, dynamic_program_for_Q).first;
    if (score_temp > score) {
      score = score_temp;
      g_star = n;
    }
  }

  set<Node*> set_nodes_remove_g_star = set_nodes;
  set_nodes_remove_g_star.erase(g_star);
  vector<int> result = M(set_nodes_remove_g_star, dts, dynamic_program_for_F, dynamic_program_for_Q, dynamic_program_for_M);
  result.push_back(g_star->GetNodeIndex());

  return result;

}


void Network::StructLearnByOtt(Dataset *dts, vector<int> topo_ord_constraint) {

  map<Node*, map<set<Node*>, double>> dynamic_program_for_F;
  map<pair<set<Node*>, vector<int>>,   pair<double, vector<pair<Node*, set<Node*>>>>> dynamic_program_for_Q;

  set<Node*> set_node_ptr_container;
  for (auto id_np : map_idx_node_ptr) {
    set_node_ptr_container.insert(id_np.second);
  }

  if (topo_ord_constraint.empty() || topo_ord_constraint.size() != num_nodes) {
    map<set<Node*>, vector<int>> dynamic_program_for_M;
    vector<int> m_of_all_nodes = M(set_node_ptr_container, dts, dynamic_program_for_F, dynamic_program_for_Q,
                                   dynamic_program_for_M);
    topo_ord_constraint = m_of_all_nodes;
  }

  pair<double, vector<pair<Node*, set<Node*>>>> score_vec_node_parents = Q(set_node_ptr_container, topo_ord_constraint, dts, dynamic_program_for_F, dynamic_program_for_Q);
  vector<pair<Node*, set<Node*>>> vec_node_parents = score_vec_node_parents.second;

  cout << "==================================================" << '\n'
       << "Setting children and parents......" << endl;
  for (auto p : vec_node_parents) {
    Node *chi = p.first;
    for (auto par : p.second) {
      SetParentChild(par, chi);
    }
  }

  cout << "==================================================" << '\n'
       << "Generating parents combinations for each node......" << endl;
  GenDiscParCombsForAllNodes();
}


vector<int> Network::SparseInstanceFillZeroToCompleteInstance(DiscreteConfig sparse_instance) {
  vector<int> complete_instance(this->num_nodes, 0);
  for (const auto p : sparse_instance) {
    complete_instance.at(p.first) = p.second;
  }
  return complete_instance;
}



#pragma clang diagnostic pop