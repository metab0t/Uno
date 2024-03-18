// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#include <cmath>
#include <cassert>
#include "TrustRegionStrategy.hpp"
#include "optimization/WarmstartInformation.hpp"
#include "tools/Logger.hpp"

TrustRegionStrategy::TrustRegionStrategy(ConstraintRelaxationStrategy& constraint_relaxation_strategy,
         const Options& options) :
      GlobalizationMechanism(constraint_relaxation_strategy, options),
      radius(options.get_double("TR_radius")),
      increase_factor(options.get_double("TR_increase_factor")),
      decrease_factor(options.get_double("TR_decrease_factor")),
      aggressive_decrease_factor(options.get_double("TR_aggressive_decrease_factor")),
      activity_tolerance(options.get_double("TR_activity_tolerance")),
      minimum_radius(options.get_double("TR_min_radius")),
      radius_reset_threshold(options.get_double("TR_radius_reset_threshold")) {
   assert(0 < this->radius && "The trust-region radius should be positive");
   assert(1. < this->increase_factor && "The trust-region increase factor should be > 1");
   assert(1. < this->decrease_factor && "The trust-region decrease factor should be > 1");
}

void TrustRegionStrategy::initialize(Statistics& statistics, Iterate& initial_iterate, const Options& options) {
   statistics.add_column("TR iter", Statistics::int_width + 3, options.get_int("statistics_minor_column_order"));
   statistics.add_column("TR radius", Statistics::double_width - 3, options.get_int("statistics_TR_radius_column_order"));
   statistics.set("TR radius", this->radius);
   
   this->constraint_relaxation_strategy.set_trust_region_radius(this->radius);
   this->constraint_relaxation_strategy.initialize(statistics, initial_iterate, options);
}

Iterate TrustRegionStrategy::compute_next_iterate(Statistics& statistics, const Model& model, Iterate& current_iterate) {
   WarmstartInformation warmstart_information{};
   warmstart_information.set_hot_start();
   DEBUG2 << "Current iterate\n" << current_iterate << '\n';

   size_t number_iterations = 0;
   while (true) { // TODO not very elegant
      try {
         number_iterations++;
         this->print_iteration(number_iterations);
         if (1 < number_iterations) {
            statistics.start_new_line();
         }
         // compute the direction within the trust region
         this->constraint_relaxation_strategy.set_trust_region_radius(this->radius);
         Direction direction = this->constraint_relaxation_strategy.compute_feasible_direction(statistics, current_iterate, warmstart_information);

         // deal with errors in the subproblem
         if (direction.status == SubproblemStatus::UNBOUNDED_PROBLEM) {
            this->set_statistics(statistics, direction, number_iterations);
            this->decrease_radius_aggressively();
            warmstart_information.set_cold_start();
         }
         else if (direction.status == SubproblemStatus::ERROR) {
            this->set_statistics(statistics, direction, number_iterations);
            this->decrease_radius();
            warmstart_information.set_cold_start();
         }
         else {
            // check whether the trial iterate (current iterate + full step) is acceptable
            Iterate trial_iterate = this->assemble_trial_iterate(model, current_iterate, direction);
            if (this->is_iterate_acceptable(statistics, model, current_iterate, trial_iterate, direction, number_iterations)) {
               this->reset_radius();
               return trial_iterate;
            }
            else {
               this->decrease_radius(direction.norm);
               // after the first iteration, only the variable bounds are updated
               warmstart_information.only_variable_bounds_changed();
            }
         }
      }
      catch (const std::runtime_error& e) {
         throw;
      }
      // if an evaluation error occurs, decrease the radius
      catch (const EvaluationError& e) {
         // WARNING << YELLOW << e.what() << RESET;
         this->set_statistics(statistics, number_iterations);
         statistics.set("status", "eval. error");
         if (Logger::level == INFO) statistics.print_current_line();
         warmstart_information.set_cold_start();
         this->decrease_radius();
      }
   }
   throw std::runtime_error("TR strategy failed for unknown reasons, this should not happen.");
}

Iterate TrustRegionStrategy::assemble_trial_iterate(const Model& model, Iterate& current_iterate, const Direction& direction) {
   Iterate trial_iterate = GlobalizationMechanism::assemble_trial_iterate(current_iterate, direction, direction.primal_dual_step_length,
         direction.primal_dual_step_length, direction.bound_dual_step_length);
   // project the trial iterate onto the bounds to avoid numerical errors
   model.project_onto_variable_bounds(trial_iterate.primals);

   // reset bound multipliers of active trust region
   this->reset_active_trust_region_multipliers(model, direction, trial_iterate);
   return trial_iterate;
}

// the trial iterate is accepted by the constraint relaxation strategy or if the step is small and we cannot switch to solving the feasibility problem
bool TrustRegionStrategy::is_iterate_acceptable(Statistics& statistics, const Model& model, Iterate& current_iterate, Iterate& trial_iterate,
      const Direction& direction, size_t number_iterations) {
   bool acceptable_iterate = this->constraint_relaxation_strategy.is_iterate_acceptable(statistics, current_iterate, trial_iterate, direction,
         direction.primal_dual_step_length);
         
   this->set_statistics(statistics, trial_iterate, direction, number_iterations);
   if (Logger::level == INFO) statistics.print_current_line();
         
   if (acceptable_iterate) {
      // possibly increase the radius if trust region is active
      this->possibly_increase_radius(direction.norm);

      // check termination criteria
      trial_iterate.status = this->check_convergence(model, trial_iterate);
      acceptable_iterate = true;
   }
   else if (this->radius < this->minimum_radius) { // rejected, but small radius
      acceptable_iterate = this->check_termination_with_small_step(model, trial_iterate);
   }
   return acceptable_iterate;
}

void TrustRegionStrategy::possibly_increase_radius(double step_norm) {
   // increase the radius if the trust-region is active
   if (step_norm >= this->radius - this->activity_tolerance) {
      this->radius *= this->increase_factor;
   }
}

void TrustRegionStrategy::decrease_radius(double step_norm) {
   // reduce the radius to a value smaller than the primal step norm (otherwise, the reduction won't have an effect)
   this->radius = std::min(this->radius, step_norm) / this->decrease_factor;
}

void TrustRegionStrategy::decrease_radius() {
   this->radius /= this->decrease_factor;
}

void TrustRegionStrategy::decrease_radius_aggressively() {
   this->radius /= this->aggressive_decrease_factor;
}

void TrustRegionStrategy::reset_radius() {
   this->radius = std::max(this->radius, this->radius_reset_threshold);
}

void TrustRegionStrategy::reset_active_trust_region_multipliers(const Model& model, const Direction& direction, Iterate& trial_iterate) const {
   assert(0 < this->radius && "The trust-region radius should be positive");
   // set multipliers for bound constraints active at trust region to 0 (except if one of the original bounds is active)
   for (size_t variable_index: direction.active_set.bounds.at_lower_bound) {
      if (variable_index < model.number_variables && std::abs(direction.primals[variable_index] + this->radius) <= this->activity_tolerance &&
            this->activity_tolerance < std::abs(trial_iterate.primals[variable_index] - model.variable_lower_bound(variable_index))) {
         trial_iterate.multipliers.lower_bounds[variable_index] = 0.;
      }
   }
   for (size_t variable_index: direction.active_set.bounds.at_upper_bound) {
      if (variable_index < model.number_variables && std::abs(direction.primals[variable_index] - this->radius) <= this->activity_tolerance &&
            this->activity_tolerance < std::abs(model.variable_upper_bound(variable_index) - trial_iterate.primals[variable_index])) {
         trial_iterate.multipliers.upper_bounds[variable_index] = 0.;
      }
   }
}

bool TrustRegionStrategy::check_termination_with_small_step(const Model& model, Iterate& trial_iterate) const {
   // evaluate infeasibility
   trial_iterate.evaluate_constraints(model);
   trial_iterate.residuals.infeasibility = model.constraint_violation(trial_iterate.evaluations.constraints, this->progress_norm);

   // terminate with a feasible point
   if (trial_iterate.residuals.infeasibility <= this->tight_tolerance) {
      trial_iterate.status = TerminationStatus::FEASIBLE_SMALL_STEP;
      return true;
   }
   else if (this->constraint_relaxation_strategy.solving_feasibility_problem()) { // terminate with an infeasible point
      trial_iterate.status = TerminationStatus::INFEASIBLE_SMALL_STEP;
      return true;
   }
   else { // do not terminate, infeasible non stationary
      return false;
   }
}

void TrustRegionStrategy::set_statistics(Statistics& statistics, size_t number_iterations) const {
   statistics.set("TR iter", number_iterations);
   statistics.set("TR radius", this->radius);
}

void TrustRegionStrategy::set_statistics(Statistics& statistics, const Direction& direction, size_t number_iterations) const {
   statistics.set("step norm", direction.norm);
   this->set_statistics(statistics, number_iterations);
}

void TrustRegionStrategy::set_statistics(Statistics& statistics, const Iterate& trial_iterate, const Direction& direction, size_t number_iterations) const {
   if (trial_iterate.is_objective_computed) {
      statistics.set("objective", trial_iterate.evaluations.objective);
   }
   this->set_statistics(statistics, direction, number_iterations);
}

void TrustRegionStrategy::print_iteration(size_t number_iterations) {
   DEBUG << "\t### Trust-region inner iteration " << number_iterations << " with radius " << this->radius << "\n\n";
}
