#include "ConstraintRelaxationStrategy.hpp"
#include "ingredients/subproblem/SubproblemFactory.hpp"
#include "optimization/Constraint.hpp"

ConstraintRelaxationStrategy::ConstraintRelaxationStrategy(const Problem& problem, const Scaling& scaling, const Options& options):
      subproblem(SubproblemFactory::create(problem, scaling,
            problem.number_variables + ConstraintRelaxationStrategy::count_elastic_variables(problem, true),
            options)),
      elastic_variables(ConstraintRelaxationStrategy::count_elastic_variables(problem, this->subproblem->uses_slacks)),
      elastic_objective_coefficient(stod(options.at("elastic_objective_coefficient"))),
      // save the original number of variables in the subproblem
      number_subproblem_variables(this->subproblem->number_variables),
      max_number_subproblem_variables(this->subproblem->max_number_variables),
      number_constraints(problem.number_constraints) {
   // generate elastic variables to relax the constraints
   ConstraintRelaxationStrategy::generate_elastic_variables(problem, this->elastic_variables, this->subproblem->number_variables,
         this->subproblem->uses_slacks);
}

size_t ConstraintRelaxationStrategy::count_elastic_variables(const Problem& problem, bool subproblem_uses_slacks) {
   size_t number_elastic_variables = 0;
   // if the subproblem uses slack variables, the bounds of the constraints are [0, 0]
   for (size_t j = 0; j < problem.number_constraints; j++) {
      if (subproblem_uses_slacks || is_finite_lower_bound(problem.constraint_bounds[j].lb)) {
         number_elastic_variables++;
      }
      if (subproblem_uses_slacks || is_finite_upper_bound(problem.constraint_bounds[j].ub)) {
         number_elastic_variables++;
      }
   }
   return number_elastic_variables;
}

void ConstraintRelaxationStrategy::generate_elastic_variables(const Problem& problem, ElasticVariables& elastic_variables, size_t number_variables,
      bool subproblem_uses_slacks) {
   // generate elastic variables p and n on the fly to relax the constraints
   // if the subproblem uses slack variables, the bounds of the constraints are [0, 0]
   size_t elastic_index = number_variables;
   for (size_t j = 0; j < problem.number_constraints; j++) {
      if (subproblem_uses_slacks || is_finite_lower_bound(problem.constraint_bounds[j].lb)) {
         // nonpositive variable n that captures the negative part of the constraint violation
         elastic_variables.negative.insert(j, elastic_index);
         elastic_index++;
      }
      if (subproblem_uses_slacks || is_finite_upper_bound(problem.constraint_bounds[j].ub)) {
         // nonnegative variable p that captures the positive part of the constraint violation
         elastic_variables.positive.insert(j, elastic_index);
         elastic_index++;
      }
   }
}

void ConstraintRelaxationStrategy::evaluate_constraints(const Problem& problem, const Scaling& scaling, Iterate& iterate) {
   this->subproblem->evaluate_constraints(problem, scaling, iterate);
}

void ConstraintRelaxationStrategy::evaluate_relaxed_constraints(const Problem& problem, const Scaling& scaling, Iterate& iterate) {
   // evaluate the constraints of the subproblem
   this->evaluate_constraints(problem, scaling, iterate);
   // add the elastic variables
   this->elastic_variables.positive.for_each([&](size_t j, size_t i) {
      iterate.subproblem_constraints[j] -= iterate.x[i];
   });
   this->elastic_variables.negative.for_each([&](size_t j, size_t i) {
      iterate.subproblem_constraints[j] += iterate.x[i];
   });
}

bool ConstraintRelaxationStrategy::is_small_step(const Direction& direction) {
   //return (direction.norm == 0.);
   const double tolerance = 1e-8;
   const double small_step_factor = 100.;
   return (direction.norm <= tolerance / small_step_factor);
}

void ConstraintRelaxationStrategy::add_elastic_variables_to_subproblem(const Problem& problem, Iterate& current_iterate) {
   this->subproblem->add_elastic_variables(problem, current_iterate, this->elastic_objective_coefficient);
}

void ConstraintRelaxationStrategy::remove_elastic_variables_from_subproblem() {
   const auto erase_elastic_variables = [&](size_t j, size_t i) {
      this->subproblem->remove_elastic_variable(i, j);
   };
   this->elastic_variables.positive.for_each(erase_elastic_variables);
   this->elastic_variables.negative.for_each(erase_elastic_variables);
}

void ConstraintRelaxationStrategy::remove_elastic_variables_from_direction(const Problem& problem, Direction& direction) {
   // the primal variables and corresponding bound multipliers are organized as follows:
   // original | subproblem-specific (may be empty) | elastic
   direction.x.resize(this->number_subproblem_variables);
   direction.multipliers.lower_bounds.resize(this->number_subproblem_variables);
   direction.multipliers.upper_bounds.resize(this->number_subproblem_variables);
   direction.norm = norm_inf(direction.x);
   // recover active set
   this->recover_active_set(problem, direction);
}

void ConstraintRelaxationStrategy::recover_active_set(const Problem& problem, Direction& direction) {
   // TODO remove elastic variables
   for (size_t i = problem.number_variables; i < direction.x.size(); i++) {
      //direction.active_set.bounds.at_lower_bound.erase(i);
      //direction.active_set.bounds.at_upper_bound.erase(i);
   }
   // constraints: only when p-n = 0
   if (direction.constraint_partition.has_value()) {
      ConstraintPartition& constraint_partition = direction.constraint_partition.value();
      this->elastic_variables.positive.for_each([&](size_t j, size_t i) {
         // if the component is strictly positive, the constraint is violated
         if (0. < direction.x[i]) {
            constraint_partition.lower_bound_infeasible.push_back(j);
         }
      });
      this->elastic_variables.negative.for_each([&](size_t j, size_t i) {
         // if the component is strictly positive, the constraint is violated
         if (0. < direction.x[i]) {
            constraint_partition.upper_bound_infeasible.push_back(j);
         }
      });
   }

   /*
   for (size_t j = 0; j < direction.multipliers.constraints.size(); j++) {
      // compute constraint violation
      double constraint_violation = 0.;
      try {
         size_t i = this->elastic_variables.positive.at(j);
         constraint_violation += direction.x[i];
      }
      catch (const std::out_of_range& e) {
      }
      try {
         size_t i = this->elastic_variables.negative.at(j);
         constraint_violation += direction.x[i];
      }
      catch (const std::out_of_range& e) {
      }
      // update active set
      if (0. < constraint_violation) {
         //direction.active_set.constraints.at_lower_bound.erase(j);
         //direction.active_set.constraints.at_upper_bound.erase(j);
      }
   }
    */
}

Direction ConstraintRelaxationStrategy::compute_second_order_correction(const Problem& problem, const Scaling& scaling, Iterate& trial_iterate) {
   return this->subproblem->compute_second_order_correction(problem, scaling, trial_iterate);
}

PredictedReductionModel ConstraintRelaxationStrategy::generate_predicted_reduction_model(const Problem& problem, const Direction& direction) const {
   return this->subproblem->generate_predicted_reduction_model(problem, direction);
}

size_t ConstraintRelaxationStrategy::get_hessian_evaluation_count() const {
   return this->subproblem->get_hessian_evaluation_count();
}

size_t ConstraintRelaxationStrategy::get_number_subproblems_solved() const {
   return this->subproblem->number_subproblems_solved;
}

SecondOrderCorrection ConstraintRelaxationStrategy::soc_strategy() const {
   return this->subproblem->soc_strategy;
}

void ConstraintRelaxationStrategy::register_accepted_iterate(Iterate& iterate) {
   this->subproblem->register_accepted_iterate(iterate);
}