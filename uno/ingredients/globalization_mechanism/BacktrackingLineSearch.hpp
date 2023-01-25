// Copyright (c) 2018-2023 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_BACKTRACKINGLINESEARCH_H
#define UNO_BACKTRACKINGLINESEARCH_H

#include "GlobalizationMechanism.hpp"

struct StepLengthTooSmall : public std::exception {
   [[nodiscard]] const char* what() const noexcept override {
      return "The step length in the line search is too small.";
   }
};

/*! \class LineSearch
 * \brief Line-search
 *
 *  Line-search strategy
 */
class BacktrackingLineSearch : public GlobalizationMechanism {
public:
   BacktrackingLineSearch(ConstraintRelaxationStrategy& constraint_relaxation_strategy, const Options& options);

   void initialize(Statistics& statistics, Iterate& first_iterate) override;
   [[nodiscard]] std::tuple<Iterate, double> compute_acceptable_iterate(Statistics& statistics, const Model& model, Iterate& current_iterate) override;

private:
   double step_length{1.};
   bool solving_feasibility_problem{false};
   const double backtracking_ratio;
   const double min_step_length;
   const bool use_second_order_correction;
   size_t total_number_iterations{0}; /*!< Total number of iterations (optimality and feasibility) */
   // statistics table
   const int statistics_SOC_column_order;
   const int statistics_LS_step_length_column_order;

   [[nodiscard]] Direction compute_direction(Statistics& statistics, Iterate& current_iterate);
   [[nodiscard]] std::tuple<Iterate, double> backtrack_along_direction(Statistics& statistics, Iterate& current_iterate, const Direction& direction);
   [[nodiscard]] bool termination() const;
   void print_iteration();
   void set_statistics(Statistics& statistics, const Direction& direction) const;
   void decrease_step_length();
};

#endif // UNO_BACKTRACKINGLINESEARCH_H
