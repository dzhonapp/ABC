/*
 * BinaryIntAutomaton.cpp
 *
 *  Created on: Oct 16, 2015
 *      Author: baki
 *   Copyright: Copyright 2015 The ABC Authors. All rights reserved. 
 *              Use of this source code is governed license that can
 *              be found in the COPYING file.
 */

#include "BinaryIntAutomaton.h"

#include <glog/logging.h>
#include <mona/dfa.h>
#include <iostream>
#include <iterator>
#include <map>
#include <string>

namespace Vlab {
namespace Theory {

const int BinaryIntAutomaton::VLOG_LEVEL = 9;

BinaryIntAutomaton::BinaryIntAutomaton() :
     Automaton(Automaton::Type::BINARYINT), formula (nullptr) {
}

BinaryIntAutomaton::BinaryIntAutomaton(DFA_ptr dfa, int num_of_variables) :
     Automaton(Automaton::Type::BINARYINT, dfa, num_of_variables), formula (nullptr) {

}

BinaryIntAutomaton::BinaryIntAutomaton(const BinaryIntAutomaton& other) :
     Automaton(other) {
  if (other.formula) {
    formula = other.formula->clone();
  }
}

BinaryIntAutomaton::~BinaryIntAutomaton() {
  delete formula;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::clone() const {
  BinaryIntAutomaton_ptr cloned_auto = new BinaryIntAutomaton(*this);
  DVLOG(VLOG_LEVEL) << cloned_auto->id << " = [" << this->id << "]->clone()";
  return cloned_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makePhi(ArithmeticFormula_ptr formula) {
  DFA_ptr non_accepting_dfa = nullptr;
  BinaryIntAutomaton_ptr non_accepting_binary_auto = nullptr;
  int num_variables = formula->getNumberOfVariables();
  int* indices = Automaton::getIndices(num_variables);

  non_accepting_dfa = Automaton::makePhi(num_variables, indices);
  delete[] indices; indices = nullptr;
  non_accepting_binary_auto = new BinaryIntAutomaton(non_accepting_dfa, num_variables);
  non_accepting_binary_auto->setFormula(formula);

  DVLOG(VLOG_LEVEL) << non_accepting_binary_auto->id << " = makePhi(" << *formula << ")";

  return non_accepting_binary_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeAutomaton(ArithmeticFormula_ptr formula) {
  BinaryIntAutomaton_ptr result_auto = nullptr;

  switch (formula->getType()) {
    case ArithmeticFormula::Type::EQ: {
      result_auto = BinaryIntAutomaton::makeEquality(formula);
      break;
    }
    case ArithmeticFormula::Type::NOTEQ: {
      result_auto = BinaryIntAutomaton::makeNotEquality(formula);
      break;
    }
    case ArithmeticFormula::Type::GT: {
      result_auto = BinaryIntAutomaton::makeGreaterThan(formula);
      break;
    }
    case ArithmeticFormula::Type::GE: {
      result_auto = BinaryIntAutomaton::makeGreaterThanOrEqual(formula);
      break;
    }
    case ArithmeticFormula::Type::LT: {
      result_auto = BinaryIntAutomaton::makeLessThan(formula);
      break;
    }
    case ArithmeticFormula::Type::LE: {
      result_auto = BinaryIntAutomaton::makeLessThanOrEqual(formula);
      break;
    }
    default:
      LOG(FATAL)<< "Equation type is not specified, please set type for input formula: " << *formula;
      break;
  }

  return result_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeAutomaton(SemilinearSet_ptr semilinear_set, std::string var_name,
        ArithmeticFormula_ptr formula, bool add_leading_zeros) {
  BinaryIntAutomaton_ptr binary_auto = nullptr;
  DFA_ptr binary_dfa = nullptr, tmp_dfa = nullptr;
  int var_index = formula->getNumberOfVariables() - formula->getVariableIndex(var_name) - 1;
  int number_of_variables = formula->getNumberOfVariables(),
          leading_zero_state = 0,
          sink_state = 0,
          number_of_binary_states = 0,
          number_of_states = 0,
          lz_index = 0;
  if (add_leading_zeros) {
    number_of_variables = number_of_variables + 1;
    lz_index = number_of_variables - 1;
  }

  DVLOG(VLOG_LEVEL)<< *semilinear_set;
  std::vector<BinaryState_ptr> binary_states;
  int* indices = getIndices(number_of_variables);
  char* statuses = nullptr;
  char* bit_transition = new char[number_of_variables + 1];

  for (int i = 0; i < number_of_variables; i++) {
    bit_transition[i] = 'X';
  }
  bit_transition[number_of_variables] = '\0';

  compute_binary_states(binary_states, semilinear_set);

  number_of_binary_states = binary_states.size();
  number_of_states = number_of_binary_states + 1;
  if (add_leading_zeros) {
    number_of_states = number_of_states + 1;
    leading_zero_state = number_of_states - 2;
  }
  sink_state = number_of_states - 1;

  dfaSetup(number_of_states, number_of_variables, indices);
  statuses = new char[number_of_states + 1];
  bool is_final_state = false;
  for (int i = 0; i < number_of_binary_states; i++) {
    is_final_state = is_accepting_binary_state(binary_states[i], semilinear_set);

    if (add_leading_zeros and is_final_state) {
      if (binary_states[i]->getd0() >= 0 && binary_states[i]->getd1() >= 0) {
        dfaAllocExceptions(3);
        bit_transition[var_index] = '0';
        bit_transition[lz_index] = '0';
        dfaStoreException(binary_states[i]->getd0(), bit_transition);
        bit_transition[var_index] = '1';
        bit_transition[lz_index] = 'X';
        dfaStoreException(binary_states[i]->getd1(), bit_transition);
        bit_transition[var_index] = '0';
        bit_transition[lz_index] = '1';
        dfaStoreException(leading_zero_state, bit_transition);
      } else if (binary_states[i]->getd0() >= 0 && binary_states[i]->getd1() < 0) {
        dfaAllocExceptions(2);
        bit_transition[var_index] = '0';
        bit_transition[lz_index] = '0';
        dfaStoreException(binary_states[i]->getd0(), bit_transition);
        bit_transition[var_index] = '0';
        bit_transition[lz_index] = '1';
        dfaStoreException(leading_zero_state, bit_transition);
      } else if (binary_states[i]->getd0() < 0 && binary_states[i]->getd1() >= 0) {
        dfaAllocExceptions(2);
        bit_transition[var_index] = '1';
        bit_transition[lz_index] = 'X';
        dfaStoreException(binary_states[i]->getd1(), bit_transition);
        bit_transition[var_index] = '0';
        bit_transition[lz_index] = '1';
        dfaStoreException(leading_zero_state, bit_transition);
      } else {
        dfaAllocExceptions(1);
        bit_transition[var_index] = '0';
        bit_transition[lz_index] = '1';
        dfaStoreException(leading_zero_state, bit_transition);
      }
      bit_transition[lz_index] = 'X';
    } else {
      if (binary_states[i]->getd0() >= 0 && binary_states[i]->getd1() >= 0) {
        dfaAllocExceptions(2);
        bit_transition[var_index] = '0';
        dfaStoreException(binary_states[i]->getd0(), bit_transition);
        bit_transition[var_index] = '1';
        dfaStoreException(binary_states[i]->getd1(), bit_transition);
      } else if (binary_states[i]->getd0() >= 0 && binary_states[i]->getd1() < 0) {
        dfaAllocExceptions(1);
        bit_transition[var_index] = '0';
        dfaStoreException(binary_states[i]->getd0(), bit_transition);
      } else if (binary_states[i]->getd0() < 0 && binary_states[i]->getd1() >= 0) {
        dfaAllocExceptions(1);
        bit_transition[var_index] = '1';
        dfaStoreException(binary_states[i]->getd1(), bit_transition);
      } else {
        dfaAllocExceptions(0);
      }
    }

    dfaStoreState(sink_state);

    if (add_leading_zeros) {
      statuses[i] = '-';
    } else if (is_final_state) {
      statuses[i] = '+';
    } else {
      statuses[i] = '-';
    }
  }

  // for the leading zero state
  if (add_leading_zeros) {
    dfaAllocExceptions(1);
    bit_transition[var_index] = '0';
    bit_transition[lz_index] = '1';
    dfaStoreException(leading_zero_state, bit_transition);
    dfaStoreState(sink_state);
    statuses[leading_zero_state] = '+';
  }

  // for the sink state
  dfaAllocExceptions(0);
  dfaStoreState(sink_state);
  statuses[sink_state] = '-';

  int zero_state = binary_states[0]->getd0(); // adding leading zeros makes acceptin zero 00, fix here
  if ( zero_state > -1 and is_accepting_binary_state(binary_states[zero_state], semilinear_set)) {
    statuses[zero_state] = '+';
  }

  statuses[number_of_states] = '\0';
  binary_dfa = dfaBuild(statuses);

  for (auto &bin_state : binary_states) {
    delete bin_state; bin_state = nullptr;
  }

  delete[] statuses;
  delete[] indices;
  delete[] bit_transition;

  if (add_leading_zeros) {
    tmp_dfa = binary_dfa;
    binary_dfa = dfaProject(binary_dfa, (unsigned) (lz_index));
    delete tmp_dfa; tmp_dfa = nullptr;
    number_of_variables = number_of_variables - 1;
  }

  binary_auto = new BinaryIntAutomaton(dfaMinimize(binary_dfa), number_of_variables);
  binary_auto->setFormula(formula);
  delete binary_dfa; binary_dfa = nullptr;

  DVLOG(VLOG_LEVEL)  << binary_auto->getId() << " = BinaryIntAutomaton::makeAutomaton(<semilinear_set>, " << var_name << ", " << *formula << ")";

  return binary_auto;
}

ArithmeticFormula_ptr BinaryIntAutomaton::getFormula() {
  return formula;
}

void BinaryIntAutomaton::setFormula(ArithmeticFormula_ptr formula) {
  this->formula = formula;
}

bool BinaryIntAutomaton::hasNegative1() {
  CHECK_EQ(1, num_of_variables)<< "implemented for single track binary automaton";
  std::vector<char> exception = {'1'};
  std::map<int, bool> is_visited;
  int current_state = this->dfa->s;
  while (not is_visited[current_state]) {
    is_visited[current_state] = true;
    current_state = getNextState(current_state, exception);
    if (current_state > -1 and isAcceptingState(current_state)) {
      return true;
    }
  }

  return false;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::complement() {
  BinaryIntAutomaton_ptr complement_auto = nullptr;
  DFA_ptr complement_dfa = dfaCopy(this->dfa);

  dfaNegation(complement_dfa);

  complement_auto = new BinaryIntAutomaton(complement_dfa, num_of_variables);
  complement_auto->setFormula(this->formula->negateOperation());

  DVLOG(VLOG_LEVEL) << complement_auto->id << " = [" << this->id << "]->complement()";
  return complement_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::intersect(BinaryIntAutomaton_ptr other_auto) {
  DFA_ptr intersect_dfa = nullptr, minimized_dfa = nullptr;
  BinaryIntAutomaton_ptr intersect_auto = nullptr;
  ArithmeticFormula_ptr intersect_formula = nullptr;

  if (not formula->isVariableOrderingSame(other_auto->formula)) {
    LOG(FATAL)<< "You cannot intersect binary automata with different variable orderings";
  }

  intersect_dfa = dfaProduct(this->dfa, other_auto->dfa, dfaAND);
  minimized_dfa = dfaMinimize(intersect_dfa);
  dfaFree(intersect_dfa);
  intersect_dfa = nullptr;

  intersect_auto = new BinaryIntAutomaton(minimized_dfa, num_of_variables);
  intersect_formula = this->formula->clone();
  intersect_formula->resetCoefficients();
  intersect_formula->setType(ArithmeticFormula::Type::INTERSECT);
  intersect_auto->setFormula(intersect_formula);

  DVLOG(VLOG_LEVEL) << intersect_auto->id << " = [" << this->id << "]->intersect(" << other_auto->id << ")";

  return intersect_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::union_(BinaryIntAutomaton_ptr other_auto) {
  DFA_ptr union_dfa = nullptr, minimized_dfa = nullptr;
  BinaryIntAutomaton_ptr union_auto = nullptr;
  ArithmeticFormula_ptr union_formula = nullptr;

  if (not formula->isVariableOrderingSame(other_auto->formula)) {
    LOG(FATAL)<< "You cannot union binary automata with different variable orderings";
  }

  union_dfa = dfaProduct(this->dfa, other_auto->dfa, dfaOR);
  minimized_dfa = dfaMinimize(union_dfa);
  dfaFree(union_dfa);
  union_dfa = nullptr;

  union_auto = new BinaryIntAutomaton(minimized_dfa, num_of_variables);
  union_formula = this->formula->clone();
  union_formula->resetCoefficients();
  union_formula->setType(ArithmeticFormula::Type::UNION);
  union_auto->setFormula(union_formula);

  DVLOG(VLOG_LEVEL) << union_auto->id << " = [" << this->id << "]->union(" << other_auto->id << ")";

  return union_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::difference(BinaryIntAutomaton_ptr other_auto) {
  BinaryIntAutomaton_ptr difference_auto = nullptr, complement_auto = nullptr;

  complement_auto = other_auto->complement();
  difference_auto = this->intersect(complement_auto);
  delete complement_auto; complement_auto = nullptr;

  DVLOG(VLOG_LEVEL) << difference_auto->id << " = [" << this->id << "]->difference(" << other_auto->id << ")";

  return difference_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::exists(std::string var_name) {
  LOG(FATAL)<< "implement me";
  return nullptr;
}
BinaryIntAutomaton_ptr BinaryIntAutomaton::getBinaryAutomatonFor(std::string var_name) {
  BinaryIntAutomaton_ptr single_var_auto = nullptr;
  ArithmeticFormula_ptr single_var_formula = nullptr;
  DFA_ptr single_var_dfa = dfaCopy(this->dfa), tmp_dfa = nullptr;
  CHECK_EQ(num_of_variables, formula->getNumberOfVariables())<< "number of variables is not consistent with formula";
  // bdd vars are ordered in the reverse order of coefficients
  int bdd_var_index = num_of_variables - formula->getVariableIndex(var_name) - 1;

  for (int i = num_of_variables - 1 ; i >= 0; i--) {
    if (i != bdd_var_index) {
      tmp_dfa = single_var_dfa;
      single_var_dfa = dfaProject(tmp_dfa, (unsigned)i);
      if (tmp_dfa != dfa) {
        dfaFree(tmp_dfa);
      }
      tmp_dfa = single_var_dfa;
      single_var_dfa = dfaMinimize(tmp_dfa);
      dfaFree(tmp_dfa);
    }
  }

  int* indices_map = getIndices(num_of_variables);
  indices_map[bdd_var_index] = 0;
  dfaReplaceIndices(single_var_dfa, indices_map);
  delete[] indices_map;

  single_var_auto = new BinaryIntAutomaton(single_var_dfa, 1);
  single_var_formula = new ArithmeticFormula();
  single_var_formula->setType(ArithmeticFormula::Type::INTERSECT);
  single_var_formula->setVariableCoefficient(var_name, 1);
  single_var_auto->setFormula(single_var_formula);

  DVLOG(VLOG_LEVEL) << single_var_auto->id << " = [" << this->id << "]->getBinaryAutomatonOf(" << var_name << ")";
  return single_var_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::getPositiveValuesFor(std::string var_name) {
  BinaryIntAutomaton_ptr positives_auto = nullptr, greater_than_or_equalt_to_zero_auto = nullptr;

  std::vector<int> indexes;
  int var_index = formula->getNumberOfVariables() - formula->getVariableIndex(var_name) - 1;
  indexes.push_back(var_index);

  greater_than_or_equalt_to_zero_auto = BinaryIntAutomaton::makeGraterThanOrEqualToZero(indexes, formula->getNumberOfVariables());
  greater_than_or_equalt_to_zero_auto->setFormula(formula->clone());

  positives_auto = this->intersect(greater_than_or_equalt_to_zero_auto);

  delete greater_than_or_equalt_to_zero_auto;
  greater_than_or_equalt_to_zero_auto = nullptr;

  DVLOG(VLOG_LEVEL) << positives_auto->id << " = [" << this->id << "]->getPositiveValuesFor(" << var_name <<")";
  return positives_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::getNegativeValuesFor(std::string var_name) {
  BinaryIntAutomaton_ptr negatives_auto = nullptr;

  LOG(FATAL)<< "implement me";
//  negatives_auto = this->intersect();

  DVLOG(VLOG_LEVEL) << negatives_auto->id << " = [" << this->id << "]->getNegativeValuesFor(" << var_name <<")";
  return negatives_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::trimLeadingZeros() {
  CHECK_EQ(1, num_of_variables)<< "trimming is implemented for single track positive binary automaton";

  BinaryIntAutomaton_ptr trimmed_auto = nullptr, tmp_auto = this->clone(),
          trim_helper_auto = nullptr;
  DFA_ptr tmp_dfa = nullptr;
  std::vector<char> exception = {'0'};
  int next_state = -1;

  for (int i = 0; i < tmp_auto->dfa->ns; i++) {
    next_state = getNextState(i, exception);
    if (isAcceptingState(next_state)) {
      tmp_auto->dfa->f[i] = 1;
    }
  }

  trim_helper_auto = BinaryIntAutomaton::makeTrimHelperAuto();
  trim_helper_auto->setFormula(tmp_auto->formula->clone());

  trimmed_auto = tmp_auto->intersect(trim_helper_auto);
  delete tmp_auto; tmp_auto = nullptr;
  delete trim_helper_auto; trim_helper_auto = nullptr;

  DVLOG(VLOG_LEVEL) << trimmed_auto->id << " = [" << this->id << "]->trimLeadingZeros()";
  return trimmed_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::addLeadingZeros() {
  LOG(FATAL)<< "implement me (similar to string->toUnary function)";
  BinaryIntAutomaton_ptr leading_zero_auto = nullptr;
  DFA_ptr leading_zero_dfa = nullptr, tmp_dfa = nullptr;

  int number_of_variables = num_of_variables + 1,
          leading_zero_state = number_of_variables - 1,
          state_key = 0;
  int* indices = getIndices(number_of_variables);

  std::vector<char> leading_zero_exception;
  std::vector<char> statuses;
  std::map<int, std::vector<char>*> exceptions;
  std::map<int, int> state_map;

  paths state_paths = nullptr, pp = nullptr;
  trace_descr tp = nullptr;

  for (int i = 0; i < number_of_variables; i++) {
    leading_zero_exception.push_back('0');
  }

  DVLOG(VLOG_LEVEL) << leading_zero_auto->id << " = [" << this->id << "]->trimLeadingZeros()";
  return leading_zero_auto;
}

/*
 *  TODO options to fix problems
 *  1 - include constants that are only beginning of a scc or not part of a scc, continue exploration by either following current algorithm, or by applying item 3
 *  2 - include constants that are only beginning of a scc or not part of a scc,
 *  take the largest one, make an automaton that is smaller than it, intersect with automaton,
 *  get constants from here, and continue period find with what is left
 *  3- do not merge periods, first do the automaton constraction then merge
 *
 */
SemilinearSet_ptr BinaryIntAutomaton::getSemilinearSet() {
  SemilinearSet_ptr semilinear_set = nullptr,
          current_set = nullptr, tmp_set = nullptr;
  BinaryIntAutomaton_ptr subject_auto = nullptr,
          tmp_1_auto = nullptr, tmp_2_auto = nullptr;
  std::vector<SemilinearSet_ptr> semilinears;
  std::string var_name = this->formula->getCoefficientIndexMap().begin()->first;
  int current_state = this->dfa->s,
          sink_state = this->getSinkState();
  std::vector<int> constants, bases;
  bool is_cyclic = false;
  std::map<int, bool> cycle_status;

  semilinear_set = new SemilinearSet();

  // 1- First get the constants that are reachable by only taking an edge of a SCC that has one state inside

  is_cyclic = getCycleStatus(cycle_status);
  getConstants(cycle_status, constants);
  Util::List::sort_and_remove_duplicate(constants);
  cycle_status.clear();
  std::cout << "initial constants: ";
  for (int i : constants) {
    std::cout << i << " ";
  }
  std::cout << std::endl;
  this->inspectAuto();

  // CASE automaton has only constants
  if (not is_cyclic) {
    semilinear_set->setConstants(constants);
    DVLOG(VLOG_LEVEL)<< *semilinear_set;
    DVLOG(VLOG_LEVEL) << "<semilinear set> = [" << this->id << "]->getSemilinearSet()";
    return semilinear_set;
  }

  /*
   * - Get the maximum constant and make an automaton Ac that accepts 0 <= x <= max
   * - Make new constants equal to the numbers that are accepted by original automaton (A)
   * intersection with Ac
   * Delete these numbers from original automaton
   */
  if (semilinear_set->hasConstants()) {

    int max_constant = constants.back(); // it is already sorted
    constants.clear();
    for (int i = 0; i <= max_constant; i++) {
      constants.push_back(i);
    }
    semilinear_set->setConstants(constants);
    constants.clear();
    tmp_1_auto = BinaryIntAutomaton::makeAutomaton(semilinear_set, var_name, formula->clone(), false);
    semilinear_set->clear();

//    tmp_1_auto->inspectAuto();

    tmp_2_auto = this->intersect(tmp_1_auto);
    delete tmp_1_auto; tmp_1_auto = nullptr;

    tmp_2_auto->inspectAuto();

    tmp_2_auto->getBaseConstants(constants); // automaton is acyclic, it will return all constants
    Util::List::sort_and_remove_duplicate(constants);
    semilinear_set->setConstants(constants);
    std::cout << "constants to remove: ";
    for (int i : constants) {
      std::cout << i << " ";
    }
    std::cout << std::endl;
    constants.clear();

    subject_auto = this->difference(tmp_2_auto);
    delete tmp_2_auto; tmp_2_auto = nullptr;
  } else {
    subject_auto = this->clone();
  }

  semilinears.push_back(semilinear_set);
  subject_auto->inspectAuto();

  unsigned i = 0;
  int cycle_head = 0;
  std::vector<int> tmp_periods;
  while (not subject_auto->isEmptyLanguage()) {
    i = 0;
    cycle_head = 0;
    tmp_periods.clear();
    semilinear_set = new SemilinearSet();

    subject_auto->getBaseConstants(bases);
    Util::List::sort_and_remove_duplicate(bases);

    std::cout << "bases: ";
    for (int i : bases) {
      std::cout << i << " ";
    }
    std::cout << std::endl;

    if (bases.size() == 1) {
      semilinear_set->setPeriod(bases[0]);
      semilinear_set->addPeriodicConstant(0);
      // no need to verify period
      tmp_1_auto = BinaryIntAutomaton::makeAutomaton(semilinear_set, var_name, formula->clone(), false);
      tmp_2_auto = subject_auto;
      subject_auto = tmp_2_auto->difference(tmp_1_auto);
      delete tmp_1_auto; tmp_1_auto = nullptr;
      delete tmp_2_auto; tmp_2_auto = nullptr;
    } else if (bases.size() > 1) {
      bool stop = false;
      int possible_period = 0;
      for(unsigned i = 0; i < bases.size() - 1; i++) {
        cycle_head = bases[0];
        semilinear_set->setCycleHead(cycle_head);
        for (unsigned j = i; j < bases.size(); j++) {
          possible_period = bases[j] - cycle_head;
//          if (possible_period % cycle_head == 0) { // case for 7 + 7k and so on... TODO check that case again and understand why
//            possible_period = cycle_head;
//          }
          // TODO validate period and if valid stop

        }

      }

    } else {
      LOG(FATAL)<< "Automaton must have an accepting state, check base extraction algorithm";
    }

    if (bases.size() == 1) {
      tmp_periods.push_back(bases[0]);
    } else if (bases.size() > 1) {
      cycle_head = bases[0];
      semilinear_set->setCycleHead(cycle_head);

      for (i = 1; i < bases.size(); i++) {
        int possible_period = bases[i] - cycle_head;
        bool add_me = true;
        if (tmp_periods.empty()) {
          tmp_periods.push_back(possible_period);
        } else {
          for (auto p : tmp_periods) {
            if ( (possible_period % p) == 0 ) {
              add_me = false;
              break;
            }
          }
          if (add_me) {
            if (possible_period % cycle_head == 0) {
              tmp_periods.push_back(cycle_head); // case for 7 + 7k and so on... formulate this
            } else {
              tmp_periods.push_back(possible_period);
            }
          }
        }
      }
    }

    if (tmp_periods.size() == 1) {
      current_set->setPeriod(tmp_periods[0]);
      current_set->addPeriodicConstant(0);
    } else {
      int period = 1;
      for (auto p : tmp_periods) {
        period = Util::Math::lcm(p, period);
      }
      current_set->setPeriod(period);
      for (auto p : tmp_periods) {
        int sum = 0;
        while (sum < period) {
          current_set->addPeriodicConstant(sum);
          sum += p;
        }
      }
    }
    Util::List::sort_and_remove_duplicate(current_set->getPeriodicConstants());

    tmp_1_auto = BinaryIntAutomaton::makeAutomaton(current_set, var_name, formula->clone(), false);
    tmp_2_auto = subject_auto;
    subject_auto = tmp_2_auto->difference(tmp_1_auto);
    delete tmp_1_auto; tmp_1_auto = nullptr;
    delete tmp_2_auto; tmp_2_auto = nullptr;

    semilinears.push_back(semilinear_set);
    semilinear_set = nullptr;
    bases.clear();
  }

  // TODO merge semilinear sets before sending

  std::cout << "semilinear sets: " << std::endl;


  LOG(FATAL) << "testing initializaiton of the algorithm";
  DVLOG(VLOG_LEVEL)<< *semilinear_set;
  DVLOG(VLOG_LEVEL) << "semilinear set = [" << this->id << "]->getSemilinearSet()";

  return semilinear_set;
}

UnaryAutomaton_ptr BinaryIntAutomaton::toUnaryAutomaton() {
  UnaryAutomaton_ptr unary_auto = nullptr;
  BinaryIntAutomaton_ptr trimmed_auto = nullptr;
  SemilinearSet_ptr semilinear_set = nullptr;

  trimmed_auto = this->trimLeadingZeros();
  semilinear_set = trimmed_auto->getSemilinearSet();
  delete trimmed_auto; trimmed_auto = nullptr;

  unary_auto = UnaryAutomaton::makeAutomaton(semilinear_set);
  delete semilinear_set; semilinear_set = nullptr;
  unary_auto->inspectAuto();
  LOG(FATAL)<< "testing toUnaryAutomaton";
  DVLOG(VLOG_LEVEL) << unary_auto->getId() << " = [" << this->id << "]->toUnaryAutomaton()";
  return unary_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeGraterThanOrEqualToZero(std::vector<int> indexes, int number_of_variables) {
  BinaryIntAutomaton_ptr postivie_numbers_auto = nullptr;
  DFA_ptr positive_numbers_dfa = nullptr;
  int* bin_variable_indices = getIndices(number_of_variables);
  int number_of_states = 3;
  std::array<char, 3> statuses {'-', '+', '-'};
  std::vector<char> exception;

  for (int i = 0; i < number_of_variables; i++) {
    exception.push_back('X');
  }
  exception.push_back('\0');

  dfaSetup(3, number_of_variables, bin_variable_indices);
  dfaAllocExceptions(1);
  for(int i : indexes) {
    exception[i] = '0';
  }
  dfaStoreException(1, &*exception.begin());
  dfaStoreState(0);

  dfaAllocExceptions(1);
  for(int i : indexes) {
    exception[i] = '1';
  }
  dfaStoreException(0, &*exception.begin());
  dfaStoreState(1);

  dfaAllocExceptions(0);
  dfaStoreState(2);

  positive_numbers_dfa = dfaBuild(&*statuses.begin());
  postivie_numbers_auto = new BinaryIntAutomaton(positive_numbers_dfa, number_of_variables);

  delete[] bin_variable_indices;

  DVLOG(VLOG_LEVEL) << postivie_numbers_auto->id << " = [BinaryIntAutomaton]->makeGraterThanOrEqualToZero(<indexes>, " << number_of_variables << ")";
  return postivie_numbers_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeEquality(ArithmeticFormula_ptr formula) {
  BinaryIntAutomaton_ptr equality_auto = nullptr;
  DFA_ptr equality_dfa = nullptr, tmp_dfa = nullptr;

  if ( not formula->simplify() ) {
    equality_auto = BinaryIntAutomaton::makePhi(formula);
    DVLOG(VLOG_LEVEL) << equality_auto->id << " = makeEquality(" << *formula << ")";
    return equality_auto;
  }

  int min = 0, max = 0, num_of_states, next_index, next_label, result, target;
  unsigned long transitions;
  const int constant = formula->getConstant();
  const int num_of_variables = formula->getCoefficients().size();
  int* indices = getIndices(num_of_variables);
  char *statuses = nullptr;
  std::map<int , StateIndices> carry_map; // maps carries to state indices

  for (int& c : formula->getCoefficients()) {
    if (c > 0) {
      max += c;
    } else {
      min += c;
    }
  }

  if ( max < constant) {
    max = constant;
  } else if (min > constant) {
    min = constant;
  }

  num_of_states = 2 * max - 2 * min + 3;
  statuses = new char[num_of_states + 1];

  for (int i = min; i < max + 1; i++) {
    carry_map[i].s = 0;
    carry_map[i].sr = 0;
    carry_map[i].i = -1;
    carry_map[i].ir = -1;
  }

  carry_map[constant].sr = 1;
  next_index = 0;
  next_label = constant;
  carry_map[constant].i = -1;
  carry_map[constant].ir = 0;



  transitions = 1 << num_of_variables; //number of transitions from each state

  dfaSetup(num_of_states, num_of_variables, indices);

  int count = 0;
  while (next_label < max + 1) { //there is a state to expand (excuding sink)
    if (carry_map[next_label].i == count) {
      carry_map[next_label].s = 2;
    } else {
      carry_map[next_label].sr = 2;
    }

    dfaAllocExceptions(transitions / 2);

    for (unsigned long j = 0; j < transitions; j++) {
      result = next_label + formula->countOnes(j);
      if ( not (result & 1) ) {
        target = result / 2;
        if (target == next_label) {
          if (carry_map[target].s == 0) {
            carry_map[target].s = 1;
            next_index++;
            carry_map[target].i = next_index;
          }
          dfaStoreException(carry_map[target].i, binaryFormat(j, num_of_variables));
        } else {
          if (carry_map[target].sr == 0) {
            carry_map[target].sr = 1;
            next_index++;
            carry_map[target].ir = next_index;
          }
          dfaStoreException(carry_map[target].ir, binaryFormat(j, num_of_variables));
        }
      }
    }

    dfaStoreState(num_of_states - 1);

    count++;

    //find next state to expand
    for (next_label = min; (next_label <= max) and
        (carry_map[next_label].i != count) and
        (carry_map[next_label].ir != count); next_label++) { }

  }

  for (; count < num_of_states; count++) {
    dfaAllocExceptions(0);
    dfaStoreState(num_of_states - 1);
  }

  //define accepting and rejecting states

  for (int i = 0; i < num_of_states; i++) {
    statuses[i] = '-';
  }

  for (next_label = min; next_label <= max; next_label++) {
    if (carry_map[next_label].s == 2) {
      statuses[carry_map[next_label].i] = '+';
    }
  }
  statuses[num_of_states] = '\0';

  tmp_dfa = dfaBuild(statuses);
  equality_dfa = dfaMinimize(tmp_dfa);
  dfaFree(tmp_dfa);
  delete[] indices;

  equality_auto = new BinaryIntAutomaton(equality_dfa, num_of_variables);
  equality_auto->setFormula(formula);

  DVLOG(VLOG_LEVEL) << equality_auto->id << " = makeEquality(" << *formula << ")";

  return equality_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeNotEquality(ArithmeticFormula_ptr formula) {
  BinaryIntAutomaton_ptr not_equal_auto = nullptr, tmp_auto = nullptr;

  formula->setType(ArithmeticFormula::Type::EQ);
  tmp_auto = BinaryIntAutomaton::makeEquality(formula);
  not_equal_auto = tmp_auto->complement();
  delete tmp_auto; tmp_auto = nullptr;

  DVLOG(VLOG_LEVEL) << not_equal_auto->id << " = makeNotEquality(" << *not_equal_auto->getFormula() << ")";
  return not_equal_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeLessThan(ArithmeticFormula_ptr formula) {
  BinaryIntAutomaton_ptr less_than_auto = nullptr;
  DFA_ptr less_than_dfa = nullptr, tmp_dfa = nullptr;

  formula->simplify();

  int min = 0, max = 0, num_of_states, next_index, next_label, result, target;
  int write1, label1, label2;
  unsigned long transitions;
  const int constant = formula->getConstant();
  const int num_of_variables = formula->getCoefficients().size();
  int* indices = getIndices(num_of_variables);
  char *statuses = nullptr;
  std::map<int , StateIndices> carry_map; // maps carries to state indices

  for (int& c : formula->getCoefficients()) {
   if (c > 0) {
     max += c;
   } else {
     min += c;
   }
  }

  if ( max < constant) {
   max = constant;
  } else if (min > constant) {
   min = constant;
  }

  num_of_states = 2 * (max - min + 1);
  statuses = new char[num_of_states + 1];

  for (int i = min; i < max + 1; i++) {
   carry_map[i].s = 0;
   carry_map[i].sr = 0;
   carry_map[i].i = -1;
   carry_map[i].ir = -1;
  }

  carry_map[constant].sr = 1;
  next_index = 0;
  next_label = constant;
  carry_map[constant].i = -1;
  carry_map[constant].ir = 0;



  transitions = 1 << num_of_variables; //number of transitions from each state

  dfaSetup(num_of_states, num_of_variables, indices);
  delete[] indices;
  int count = 0;
  while (next_label < max + 1) { //there is a state to expand (excuding sink)
   if (carry_map[next_label].i == count) {
     carry_map[next_label].s = 2;
   } else {
     carry_map[next_label].sr = 2;
   }

   dfaAllocExceptions(transitions);

   for (unsigned long j = 0; j < transitions; j++) {
     int num_of_ones = formula->countOnes(j);
     result = next_label + num_of_ones;

     if (result >= 0) {
       target = result / 2;
     } else {
       target = (result - 1) / 2;
     }

     write1 = result & 1;
     label1 = next_label;
     label2 = target;

     while (label1 != label2) {
       label1 = label2;
       result = label1 + num_of_ones;
       if (result >= 0) {
         label2 = result / 2;
       } else {
         label2 = (result - 1) / 2;
       }
       write1 = result & 1;
     }

     if (write1) {
       if (carry_map[target].s == 0) {
         carry_map[target].s = 1;
         next_index++;
         carry_map[target].i = next_index;
       }
       dfaStoreException(carry_map[target].i, binaryFormat(j, num_of_variables));
     } else {
       if (carry_map[target].sr == 0) {
         carry_map[target].sr = 1;
         next_index++;
         carry_map[target].ir = next_index;
       }
       dfaStoreException(carry_map[target].ir, binaryFormat(j, num_of_variables));
     }
   }

   dfaStoreState(count);

   count++;

   //find next state to expand
   for (next_label = min; (next_label <= max) and
       (carry_map[next_label].i != count) and
       (carry_map[next_label].ir != count); next_label++) { }

  }

  for (int i = count; i < num_of_states; i++) {
   dfaAllocExceptions(0);
   dfaStoreState(i);
  }

  //define accepting and rejecting states

  for (int i = 0; i < num_of_states; i++) {
   statuses[i] = '-';
  }

  for (next_label = min; next_label <= max; next_label++) {
   if (carry_map[next_label].s == 2) {
     statuses[carry_map[next_label].i] = '+';
   }
  }
  statuses[num_of_states] = '\0';

  tmp_dfa = dfaBuild(statuses);
  tmp_dfa->ns = tmp_dfa->ns - (num_of_states - count);
  less_than_dfa = dfaMinimize(tmp_dfa);
  dfaFree(tmp_dfa);

  less_than_auto = new BinaryIntAutomaton(less_than_dfa, num_of_variables);
  less_than_auto->setFormula(formula);

  DVLOG(VLOG_LEVEL) << less_than_auto->id << " = makeLessThan(" << *formula << ")";

  return less_than_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeLessThanOrEqual(ArithmeticFormula_ptr formula) {
  BinaryIntAutomaton_ptr less_than_or_equal_auto = nullptr;

  ArithmeticFormula_ptr less_than_formula = formula->clone();
  less_than_formula->setConstant(less_than_formula->getConstant() - 1);
  less_than_formula->setType(ArithmeticFormula::Type::LT);

  less_than_or_equal_auto = BinaryIntAutomaton::makeLessThan(less_than_formula);
  less_than_or_equal_auto->setFormula(formula);
  delete less_than_formula;

  DVLOG(VLOG_LEVEL) << less_than_or_equal_auto->id << " = makeLessThanOrEqual(" << *formula << ")";

  return less_than_or_equal_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeGreaterThan(ArithmeticFormula_ptr formula) {
  BinaryIntAutomaton_ptr greater_than_auto = nullptr;

  ArithmeticFormula_ptr less_than_formula = formula->multiply(-1);
  less_than_formula->setType(ArithmeticFormula::Type::LT);

  greater_than_auto = BinaryIntAutomaton::makeLessThan(less_than_formula);
  greater_than_auto->setFormula(formula);
  delete less_than_formula;

  DVLOG(VLOG_LEVEL) << greater_than_auto->id << " = makeGreaterThan(" << *formula << ")";

  return greater_than_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeGreaterThanOrEqual(ArithmeticFormula_ptr formula) {
  BinaryIntAutomaton_ptr greater_than_or_equal_auto = nullptr;

  ArithmeticFormula_ptr less_than_formula = formula->multiply(-1);
  less_than_formula->setConstant(less_than_formula->getConstant() - 1);
  less_than_formula->setType(ArithmeticFormula::Type::LT);

  greater_than_or_equal_auto = BinaryIntAutomaton::makeLessThan(less_than_formula);
  greater_than_or_equal_auto->setFormula(formula);
  delete less_than_formula;

  DVLOG(VLOG_LEVEL) << greater_than_or_equal_auto->id << " = makeGreaterThanOrEqual(" << *formula << ")";

  return greater_than_or_equal_auto;
}

BinaryIntAutomaton_ptr BinaryIntAutomaton::makeTrimHelperAuto() {
  BinaryIntAutomaton_ptr trim_helper_auto = nullptr;
  DFA_ptr trim_helper_dfa = nullptr;
  int number_of_variables = 1;
  int* bin_variable_indices = getIndices(number_of_variables);
  int number_of_states = 5;
  std::array<char, 5> statuses {'-', '+', '+', '-', '-'};
  std::vector<char> exception;

  exception.push_back('X');
  exception.push_back('\0');

  dfaSetup(number_of_states, number_of_variables, bin_variable_indices);
  // state 0
  dfaAllocExceptions(2);
  exception[0] = '0';
  dfaStoreException(1, &*exception.begin());
  exception[0] = '1';
  dfaStoreException(2, &*exception.begin());
  dfaStoreState(0);
  // state 1
  dfaAllocExceptions(2);
  exception[0] = '0';
  dfaStoreException(3, &*exception.begin());
  exception[0] = '1';
  dfaStoreException(2, &*exception.begin());
  dfaStoreState(1);
  // state 2
  dfaAllocExceptions(1);
  exception[0] = '0';
  dfaStoreException(4, &*exception.begin());
  dfaStoreState(2);
  // state 3
  dfaAllocExceptions(1);
  exception[0] = '1';
  dfaStoreException(2, &*exception.begin());
  dfaStoreState(3);
  // state 4
  dfaAllocExceptions(1);
  exception[0] = '1';
  dfaStoreException(2, &*exception.begin());
  dfaStoreState(4);

  trim_helper_dfa = dfaBuild(&*statuses.begin());
  trim_helper_auto = new BinaryIntAutomaton(trim_helper_dfa, number_of_variables);

  delete[] bin_variable_indices;

  DVLOG(VLOG_LEVEL) << trim_helper_auto->id << " = [BinaryIntAutomaton]->makeTrimHelperAuto()";
  return trim_helper_auto;
}

void BinaryIntAutomaton::compute_binary_states(std::vector<BinaryState_ptr>& binary_states,
        SemilinearSet_ptr semilinear_set) {
  if (semilinear_set->getPeriod() == 0) {
    add_binary_state(binary_states, semilinear_set->getConstants());
  } else {
    add_binary_state(binary_states, semilinear_set->getCycleHead(), semilinear_set->getPeriod(), BinaryState::Type::VAL, -1, -1);
  }
}

/**
 * works for positive numbers for now
 */
void BinaryIntAutomaton::add_binary_state(std::vector<BinaryState_ptr>& binary_states, std::vector<int>& constants) {
  std::map<std::pair<int, int>, int> binary_state_map;

  binary_states.push_back(new BinaryState(-1, 0));
  binary_state_map.insert(std::make_pair(std::make_pair(-1, 0), 0));

  for (auto value : constants) {
    CHECK_GE(value, 0)<< "works for positive numbers only";
    unsigned i = 0;
    int rank = 1;
    int mask = value;
    int state_value = 0;
    int current_bit = 0;

    do {
      current_bit = mask & 1;
      if (current_bit) {
        state_value = state_value | (1 << (rank - 1));
      }
      auto key = std::make_pair(state_value, rank);
      auto it = binary_state_map.find(key);

      if (it == binary_state_map.end()) {
        binary_states.push_back(new BinaryState(state_value, rank));

        int index = binary_states.size() - 1;
        binary_state_map[key] = index;
        if (current_bit) {
          binary_states[i]->setd1(index);
        } else {
          binary_states[i]->setd0(index);
        }
        i = index;
      } else {
        i = it->second;
      }

      mask >>= 1;
      rank += 1;
    } while (state_value not_eq value);
  }
}

int BinaryIntAutomaton::add_binary_state(std::vector<BinaryState_ptr>& binary_states,
        int C, int R, BinaryState::Type t, int v, int b) {
  unsigned i = 0;
  int d0 = -1, d1 = -1;

  for (i = 0; i < binary_states.size(); i++) {
    if (binary_states[i]->isEqualTo(t, v, b)) {
      return i;
    }
  }


  binary_states.push_back(new BinaryState(t, v, b));

  if (b < 0) {
    if (C == 0) {
      d1 = add_binary_state(binary_states, C, R, BinaryState::Type::REMT, 1 % R, 1 % R);
      d0 = add_binary_state(binary_states, C, R, BinaryState::Type::REMT, 0, 1 % R);
    } else if (C == 1) {
      d1 = add_binary_state(binary_states, C, R, BinaryState::Type::REMT, 1 % R, 1 % R);
      d0 = add_binary_state(binary_states, C, R, BinaryState::Type::REMF, 0, 1 % R);
    } else {
      d1 = add_binary_state(binary_states, C, R, BinaryState::Type::VAL, 1, 1);
      d0 = add_binary_state(binary_states, C, R, BinaryState::Type::VAL, 0, 1);
    }
  } else if (BinaryState::Type::VAL == t && (v + 2 * b >= C)) {
    d1 = add_binary_state(binary_states, C, R, BinaryState::Type::REMT, (v + 2 * b) % R, (2 * b) % R);
    d0 = add_binary_state(binary_states, C, R, BinaryState::Type::REMF, v % R, (2 * b) % R);
  } else if (BinaryState::Type::VAL == t && (v + 2 * b < C)) {
    d1 = add_binary_state(binary_states, C, R, BinaryState::Type::VAL, v + 2 * b, 2 * b);
    d0 = add_binary_state(binary_states, C, R, BinaryState::Type::VAL, v, 2 * b);
  } else if (BinaryState::Type::REMT == t) {
    d1 = add_binary_state(binary_states, C, R, BinaryState::Type::REMT, (v + 2 * b) % R, (2 * b) % R);
    d0 = add_binary_state(binary_states, C, R, BinaryState::Type::REMT, v % R, (2 * b) % R);
  } else if (BinaryState::Type::REMF == t) {
    d1 = add_binary_state(binary_states, C, R, BinaryState::Type::REMT, (v + 2 * b) % R, (2 * b) % R);
    d0 = add_binary_state(binary_states, C, R, BinaryState::Type::REMF, v % R, (2 * b) % R);
  }

  binary_states[i]->setd0d1(d0, d1);

  return i;
}

bool BinaryIntAutomaton::is_accepting_binary_state(BinaryState_ptr binary_state, SemilinearSet_ptr semilinear_set) {
  if (BinaryState::Type::VAL == binary_state->getType()) {
    for (auto i : semilinear_set->getConstants()) {
      if (i == binary_state->getV()) {
        return true;
      }
    }
  } else if (BinaryState::Type::REMT == binary_state->getType()) {
    for (auto i : semilinear_set->getPeriodicConstants()) {
      if ( ((i + semilinear_set->getCycleHead()) % (semilinear_set->getPeriod())) == binary_state->getV() ) {
        return true;
      }
    }
  }
  return false;
}

bool BinaryIntAutomaton::getCycleStatus(std::map<int, bool>& cycle_status) {
  std::map<int, int> disc;
  std::map<int, int> low;
  std::map<int, bool> is_stack_member;
  std::vector<int> st;
  std::vector<bool> path;
  int time = 0;
  int sink_state = getSinkState();

  disc[sink_state] = 0; // avoid exploring sink state
  is_stack_member[sink_state] = false; // avoid looping to sink state
  cycle_status[sink_state] = true;
  getCycleStatus(this->dfa->s, disc, low, st, is_stack_member, cycle_status, time);
  DVLOG(VLOG_LEVEL) << cycle_status[-2] << " = [" << this->id << "]->getCycleStatus(<constants>)";
  return cycle_status[-2]; // -2 is to keep if it is cyclic at all or not
}

void BinaryIntAutomaton::getCycleStatus(int state, std::map<int, int>& disc, std::map<int, int>& low, std::vector<int>& st,
          std::map<int, bool>& is_stack_member, std::map<int, bool>& cycle_status, int& time) {
  int next_state = 0;
  std::vector<char> exception = {'0'};
  int l, r;
//  std::cout << "visiting: " << state << std::endl;
  disc[state] = low[state] = time; time++;
  st.push_back(state);
  is_stack_member[state] = true;

  l = getNextState(state, exception);
  exception[0] = '1';
  r = getNextState(state, exception);

  for (int b = 0; b < 2; b++) {
    next_state = (b == 0) ? l : r;
    if (disc.find(next_state) == disc.end()) {
      getCycleStatus(next_state, disc, low, st, is_stack_member, cycle_status, time);
      low[state] = std::min(low[state], low[next_state]);
    } else if (is_stack_member[next_state]) {
      low[state] = std::min(low[state], disc[next_state]);
    }

  }

  if (low[state] == disc[state]) { // head of SCC
    int current_state = st.back();

    while (current_state != state) {
      st.pop_back();
      is_stack_member[current_state] = false;
      cycle_status[current_state] = true;
      cycle_status[-2] = true;
      current_state = st.back();
    }

    is_stack_member[current_state] = false;
    st.pop_back();

    if (current_state == l or current_state == r) {
      cycle_status[current_state] = true;
      cycle_status[-2] = true;
    }
  }

  return;
}

void BinaryIntAutomaton::getConstants(std::map<int, bool>& cycle_status, std::vector<int>& constants) {
  std::vector<bool> path;

  // current state cannot be accepting in binary automaton
  if ((not isSinkState(this->dfa->s)) and (not cycle_status[this->dfa->s])) {
    getConstants(this->dfa->s, cycle_status, path, constants);
  }

  DVLOG(VLOG_LEVEL) << "<void> = [" << this->id << "]->getConstants(<cycle status>, <constants>)";
  return;
}

void BinaryIntAutomaton::getConstants(int state, std::map<int, bool>& cycle_status, std::vector<bool>& path, std::vector<int>& constants) {
  int next_state = 0;
  std::vector<char> exception = {'0'};
  int l, r;


  l = getNextState(state, exception);
  exception[0] = '1';
  r = getNextState(state, exception);

  for (int b = 0; b < 2; b++) {
    next_state = (b == 0) ? l : r;

    if ((not isSinkState(next_state))) {
      path.push_back( b == 1);
      if (isAcceptingState(next_state)) {
        int c = 0;
        for (unsigned i = 0; i < path.size(); i++) {
          if (path[i]) {
            c += (1 << i);
          }
        }
        constants.push_back(c);
      }
      if (not cycle_status[next_state]) { // if next state is not in a cycle continue exploration
        getConstants(next_state, cycle_status, path, constants);
      }
      path.pop_back();
    }
  }
}

/**
 * TODO that function does not catch all the constants because of automaton structure
 * Sets constant numbers accepted by this automaton
 * (constant numbers are reachable without involving any SCC except the ones with size 1)
 * @return true if automaton is cyclic, false otherwise
 */
bool BinaryIntAutomaton::getConstants(std::vector<int>& constants) {
  std::map<int, int> disc;
  std::map<int, int> low;
  std::map<int, bool> is_stack_member;
  std::vector<int> st;
  std::vector<bool> path;
  int time = 0;
  bool result = false;
  int sink_state = getSinkState();

  if (sink_state == this->dfa->s) {
    return false;
  }

  disc[sink_state] = 0; // avoid exploring sink state
  is_stack_member[sink_state] = false; // avoid looping to sink state

  result = getConstants(this->dfa->s, disc, low, st, is_stack_member, path, constants, time);
  DVLOG(VLOG_LEVEL) << result << " = [" << this->id << "]->getConstants(<constants>)";
  return result;
}

bool BinaryIntAutomaton::getConstants(int state, std::map<int, int>& disc, std::map<int, int>& low, std::vector<int>& st,
        std::map<int, bool>& is_stack_member, std::vector<bool>& path, std::vector<int>& constants, int& time) {

  int next_state = 0;
  std::vector<char> exception = {'0'};
  int l, r;

//  std::cout << "visiting state: " << state << std::endl;
  disc[state] = low[state] = time; time++;
  st.push_back(state);
  is_stack_member[state] = true;

  l = getNextState(state, exception);
  exception[0] = '1';
  r = getNextState(state, exception);
  bool is_cyclic = true; // TODO figure out that

  for (int b = 0; b < 2; b++) {
    next_state = (b == 0) ? l : r;
//    std::cout << "next state: " << next_state << std::endl;
    if (disc.find(next_state) == disc.end()) {
      path.push_back( b == 1);
      is_cyclic = getConstants(next_state, disc, low, st, is_stack_member, path, constants, time);
      low[state] = std::min(low[state], low[next_state]);
      path.pop_back();
    } else if (is_stack_member[next_state]) {
      low[state] = std::min(low[state], disc[next_state]);
    }

  }

  bool is_in_cycle = false;
  if (low[state] == disc[state]) { // head of SCC
    int current_state = st.back();
    while (current_state != state) {
      st.pop_back();
      is_stack_member[current_state] = false;
      current_state = st.back();
      is_in_cycle = true;
    }
    is_stack_member[current_state] = false;
    st.pop_back();

    if (current_state == l or current_state == r) {
      is_in_cycle = true;
    }

    if ((not is_in_cycle) and isAcceptingState(current_state)) {

      int c = 0;
      for (unsigned i = 0; i < path.size(); i++) {
        if (path[i]) {
          c += (1 << i);
        }
      }
      constants.push_back(c);
    }
  }

  return is_in_cycle;
}

void BinaryIntAutomaton::getBaseConstants(std::vector<int>& constants) {
  bool *is_stack_member = new bool[this->dfa->ns];
  std::vector<bool> path;

  for (int i = 0; i < this->dfa->ns; i++) {
    is_stack_member[i] = false;
  }

  if (not isSinkState(this->dfa->s)) {
    getBaseConstants(this->dfa->s, is_stack_member, path, constants);
  }

  delete[] is_stack_member;

  DVLOG(VLOG_LEVEL) << "<void> = [" << this->id << "]->getBaseConstants(<base constants>)";

  return;
}

/**
 * @returns populated constants, ignores the initial state whether is an accepted or not
 */
void BinaryIntAutomaton::getBaseConstants(int state, bool *is_stack_member, std::vector<bool>& path, std::vector<int>& constants) {
  int next_state = 0;
  std::vector<char> exception = {'0'};
  int l, r;

  is_stack_member[state] = true;

  l = getNextState(state, exception);
  exception[0] = '1';
  r = getNextState(state, exception);

  for (int b = 0; b < 2; b++) {
    next_state = (b == 0) ? l : r;

    if ((not is_stack_member[next_state]) and (not isSinkState(next_state))) {
      path.push_back( b == 1);

      if (isAcceptingState(next_state)) {
        int c = 0;
        for (unsigned i = 0; i < path.size(); i++) {
          if (path[i]) {
            c += (1 << i);
          }
        }
        constants.push_back(c);
      }

      getBaseConstants(next_state, is_stack_member, path, constants);
      path.pop_back();
    }
  }
  is_stack_member[state] = false;
}




} /* namespace Theory */
} /* namespace Vlab */
