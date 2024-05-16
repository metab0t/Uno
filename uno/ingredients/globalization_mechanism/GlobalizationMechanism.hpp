// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_GLOBALIZATIONMECHANISM_H
#define UNO_GLOBALIZATIONMECHANISM_H

#include "ingredients/subproblem/Direction.hpp"
#include "linear_algebra/Vector.hpp"
#include "optimization/TerminationStatus.hpp"

// forward declarations
class ConstraintRelaxationStrategy;
class Iterate;
class Model;
class Options;
class Statistics;

class GlobalizationMechanism {
public:
   GlobalizationMechanism(ConstraintRelaxationStrategy& constraint_relaxation_strategy, const Options& options);
   virtual ~GlobalizationMechanism() = default;

   virtual void initialize(Statistics& statistics, Iterate& initial_iterate, const Options& options) = 0;
   virtual void compute_next_iterate(Statistics& statistics, const Model& model, Iterate& current_iterate, Iterate& trial_iterate) = 0;

   [[nodiscard]] size_t get_hessian_evaluation_count() const;
   [[nodiscard]] size_t get_number_subproblems_solved() const;

protected:
   // reference to allow polymorphism
   ConstraintRelaxationStrategy& constraint_relaxation_strategy; /*!< Constraint relaxation strategy */
   Direction direction;
   const double tight_tolerance; /*!< Tight tolerance of the termination criteria */
   const double loose_tolerance; /*!< Loose tolerance of the termination criteria */
   size_t loose_tolerance_consecutive_iterations{0};
   const size_t loose_tolerance_consecutive_iteration_threshold;
   const Norm progress_norm;
   const double unbounded_objective_threshold;

   static void assemble_trial_iterate(const Model& model, Iterate& current_iterate, Iterate& trial_iterate, const Direction& direction,
         double primal_step_length, double dual_step_length, double bound_dual_step_length);
   [[nodiscard]] TerminationStatus check_termination(const Model& model, Iterate& current_iterate);
   [[nodiscard]] TerminationStatus check_convergence_with_given_tolerance(const Model& model, Iterate& current_iterate, double tolerance) const;
};

#endif // UNO_GLOBALIZATIONMECHANISM_H
