// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_LPSUBPROBLEM_H
#define UNO_LPSUBPROBLEM_H

#include <memory>
#include "InequalityConstrainedMethod.hpp"
#include "solvers/LP/LPSolver.hpp"
#include "tools/Options.hpp"

class LPSubproblem : public InequalityConstrainedMethod {
public:
   LPSubproblem(size_t max_number_variables, size_t max_number_constraints, size_t max_number_objective_gradient_nonzeros,
         size_t max_number_jacobian_nonzeros, const Options& options);

   void generate_initial_iterate(const NonlinearProblem& problem, Iterate& initial_iterate) override;
   [[nodiscard]] Direction solve(Statistics& statistics, const NonlinearProblem& problem, Iterate& current_iterate,
         const WarmstartInformation& warmstart_information) override;
   [[nodiscard]] std::function<double(double)> compute_predicted_optimality_reduction_model(const NonlinearProblem& problem,
         const Iterate& current_iterate, const Direction& direction, double step_length) const override;
   [[nodiscard]] size_t get_hessian_evaluation_count() const override;

private:
   // pointer to allow polymorphism
   const std::unique_ptr<LPSolver> solver; /*!< Solver that solves the subproblem */

   void evaluate_functions(const NonlinearProblem& problem, Iterate& current_iterate, const WarmstartInformation& warmstart_information);
};

#endif // UNO_LPSUBPROBLEM_H
