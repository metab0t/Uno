// Copyright (c) 2022 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#include "ConstraintRelaxationStrategy.hpp"

ConstraintRelaxationStrategy::ConstraintRelaxationStrategy(bool penalty_parameter_control, const Options& options):
      penalty_parameter_control(penalty_parameter_control), residual_norm(norm_from_string(options.at("residual_norm"))),
      small_step_threshold(stod(options.at("small_step_threshold"))) {
}

bool ConstraintRelaxationStrategy::is_small_step(const Direction& direction) const {
   return (direction.norm <= this->small_step_threshold);
}

void ConstraintRelaxationStrategy::compute_nonlinear_residuals(const ReformulatedProblem& problem, Iterate& iterate) const {
   iterate.evaluate_constraints(problem.model);
   iterate.constraint_violation = problem.model.compute_constraint_violation(iterate.original_evaluations.constraints, L1_NORM);
   iterate.evaluate_lagrangian_gradient(problem.model, problem.get_objective_multiplier(), iterate.multipliers.constraints,
         iterate.multipliers.lower_bounds, iterate.multipliers.upper_bounds);
   iterate.stationarity_error = norm(iterate.lagrangian_gradient, this->residual_norm);
}

double ConstraintRelaxationStrategy::compute_predicted_infeasibility_reduction(const Model& model, const Iterate& current_iterate,
      const Direction& direction, double step_length) {
   // determine the linearized constraint violation term: c(x_k) + alpha*\nabla c(x_k)^T d
   const auto residual_function = [&](size_t j) {
      const double component_j = current_iterate.original_evaluations.constraints[j] + step_length * dot(direction.primals,
            current_iterate.original_evaluations.constraint_jacobian[j]);
      return model.compute_constraint_violation(component_j, j);
   };

   const double linearized_constraint_violation = norm_1(residual_function, Range(model.number_constraints));
   return current_iterate.constraint_violation - linearized_constraint_violation;
}