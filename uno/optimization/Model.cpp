// Copyright (c) 2022 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#include <cmath>
#include <iostream>
#include <cassert>
#include <utility>
#include "Model.hpp"
#include "linear_algebra/Vector.hpp"
#include "tools/Range.hpp"
#include "tools/Infinity.hpp"

std::map<FunctionType, std::string> Model::type_to_string = {
      {LINEAR, "linear"},
      {QUADRATIC, "quadratic"},
      {NONLINEAR, "nonlinear"}
};

// abstract Problem class
Model::Model(std::string name, size_t number_variables, size_t number_constraints, FunctionType type) :
      name(std::move(name)), number_variables(number_variables), number_constraints(number_constraints), problem_type(type),
      equality_constraints(this->number_constraints), inequality_constraints(this->number_constraints),
      linear_constraints(this->number_constraints), slacks(this->number_constraints) {
}

void Model::determine_bounds_types(std::vector<Interval>& bounds, std::vector<BoundType>& status) {
   assert(bounds.size() == status.size());
   // build the "status" vector as a mapping (map/transform operation) of the "bounds" vector
   std::transform(begin(bounds), end(bounds), begin(status), [](const Interval& bounds_i) {
      if (bounds_i.lb == bounds_i.ub) {
         return EQUAL_BOUNDS;
      }
      else if (is_finite(bounds_i.lb) && is_finite(bounds_i.ub)) {
         return BOUNDED_BOTH_SIDES;
      }
      else if (is_finite(bounds_i.lb)) {
         return BOUNDED_LOWER;
      }
      else if (is_finite(bounds_i.ub)) {
         return BOUNDED_UPPER;
      }
      else {
         return UNBOUNDED;
      }
   });
}

void Model::determine_constraints() {
   size_t current_equality_constraint = 0;
   size_t current_inequality_constraint = 0;
   for (size_t j = 0; j < this->number_constraints; j++) {
      if (this->get_constraint_bound_type(j) == EQUAL_BOUNDS) {
         this->equality_constraints.insert(j, current_equality_constraint);
         current_equality_constraint++;
      }
      else {
         this->inequality_constraints.insert(j, current_inequality_constraint);
         current_inequality_constraint++;
      }
   }
}

void Model::project_primals_onto_bounds(std::vector<double>& x) const {
   for (size_t i = 0; i < x.size(); i++) {
      if (x[i] < this->get_variable_lower_bound(i)) {
         x[i] = this->get_variable_lower_bound(i);
      }
      else if (this->get_variable_upper_bound(i) < x[i]) {
         x[i] = this->get_variable_upper_bound(i);
      }
   }
}

bool Model::is_constrained() const {
   return (0 < this->number_constraints);
}

double Model::compute_constraint_lower_bound_violation(double constraint_value, size_t j) const {
   const double lower_bound = this->get_constraint_lower_bound(j);
   return std::max(0., lower_bound - constraint_value);
}

double Model::compute_constraint_upper_bound_violation(double constraint_value, size_t j) const {
   const double upper_bound = this->get_constraint_upper_bound(j);
   return std::max(0., constraint_value - upper_bound);
}

double Model::compute_constraint_violation(double constraint_value, size_t j) const {
   const double lower_bound_violation = this->compute_constraint_lower_bound_violation(constraint_value, j);
   const double upper_bound_violation = this->compute_constraint_upper_bound_violation(constraint_value, j);
   return std::max(lower_bound_violation, upper_bound_violation);
}

// compute ||c_S|| for a given set of constraints
double Model::compute_constraint_violation(const std::vector<double>& constraints, const std::vector<size_t>& constraint_set,
      Norm residual_norm) const {
   const auto jth_component = [&](size_t j) {
      return this->compute_constraint_violation(constraints[j], j);
   };
   return norm<double>(jth_component, constraint_set, residual_norm);
}

// compute ||c||
double Model::compute_constraint_violation(const std::vector<double>& constraints, Norm residual_norm) const {
   // create a lambda to avoid allocating an std::vector
   const auto jth_component = [&](size_t j) {
      return this->compute_constraint_violation(constraints[j], j);
   };
   return norm<double>(jth_component, Range(constraints.size()), residual_norm);
}