#include <cmath>
#include "Argonot.hpp"
#include "PenaltyMeritFunction.hpp"
#include "Logger.hpp"

/*
 * Infeasibility detection and SQP methods for nonlinear optimization 
 * http://epubs.siam.org/doi/pdf/10.1137/080738222
 */

PenaltyMeritFunction::PenaltyMeritFunction(Subproblem& subproblem, double tolerance): GlobalizationStrategy(subproblem, tolerance), eta(1e-8) {
}

Iterate PenaltyMeritFunction::initialize(Problem& problem, std::vector<double>& x, Multipliers& multipliers, bool use_trust_region) {
    /* initialize the subproblem */
    Iterate first_iterate = this->subproblem.initialize(problem, x, multipliers, use_trust_region);

    first_iterate.KKT_residual = Argonot::compute_KKT_error(problem, first_iterate, 1.);
    first_iterate.complementarity_residual = Argonot::compute_complementarity_error(problem, first_iterate);

    return first_iterate;
}

bool PenaltyMeritFunction::check_step(Problem& problem, Iterate& current_iterate, SubproblemSolution& solution, double step_length) {
    /* stage g: line-search along fixed step */
    /* generate the trial point */
    std::vector<double> trial_x = add_vectors(current_iterate.x, solution.x, step_length);
    Iterate trial_iterate(trial_x, solution.multipliers);
    double step_norm = step_length * solution.norm;
    
    bool accept = false;
    /* check zero step */
    if (step_norm == 0.) {
        accept = true;
    }
    else {

        /* check if subproblem definition changed */
        if (this->subproblem.subproblem_definition_changed) {
            this->subproblem.subproblem_definition_changed = false;
            this->subproblem.compute_optimality_measures(problem, current_iterate);
        }

        // TODO: add the penalized term to the optimality measure
        this->subproblem.compute_optimality_measures(problem, trial_iterate);
        /* compute current exact l1 penalty: rho f + ||c|| */
        double current_exact_l1_penalty = solution.objective_multiplier * current_iterate.optimality_measure + current_iterate.feasibility_measure;
        /* compute trial exact l1 penalty */
        double trial_exact_l1_penalty = solution.objective_multiplier * trial_iterate.optimality_measure + trial_iterate.feasibility_measure;
        /* check the validity of the trial step */
        accept = false;
        if (current_exact_l1_penalty - trial_exact_l1_penalty >= this->eta * step_length * (current_iterate.feasibility_measure - solution.objective)) {
            accept = true;
        }
    }
    
    if (accept) {
        trial_iterate.compute_objective(problem);
        trial_iterate.compute_constraint_residual(problem, this->subproblem.residual_norm);
        trial_iterate.KKT_residual = Argonot::compute_KKT_error(problem, trial_iterate, solution.objective_multiplier);
        trial_iterate.complementarity_residual = Argonot::compute_complementarity_error(problem, trial_iterate);
        double step_norm = step_length * solution.norm;
        trial_iterate.status = this->compute_status(problem, trial_iterate, step_norm, solution.objective_multiplier);
        current_iterate = trial_iterate;
    }
    return accept;
}

OptimalityStatus PenaltyMeritFunction::compute_status(Problem& problem, Iterate& current_iterate, double step_norm, double objective_multiplier) {
    OptimalityStatus status = NOT_OPTIMAL;

    if (current_iterate.constraint_residual <= this->tolerance * current_iterate.x.size()) {
        if (current_iterate.KKT_residual <= this->tolerance * std::sqrt(current_iterate.x.size()) && current_iterate.complementarity_residual <= this->tolerance * (current_iterate.x.size() + problem.number_constraints)) {
            if (0. < objective_multiplier) {
                status = KKT_POINT;
            }
            else {
                status = FJ_POINT;
            }
        }
        else if (step_norm <= this->tolerance / 100.) {
            status = FEASIBLE_SMALL_STEP;
        }
    }
    else if (step_norm <= this->tolerance / 100.) {
        status = INFEASIBLE_SMALL_STEP;
    }
    
    // if convergence, correct the multipliers
    if (status != NOT_OPTIMAL && 0. < objective_multiplier) {
        for (int j = 0; j < problem.number_constraints; j++) {
            current_iterate.multipliers.constraints[j] /= objective_multiplier;
        }
        for (unsigned int i = 0; i < current_iterate.x.size(); i++) {
            current_iterate.multipliers.lower_bounds[i] /= objective_multiplier;
            current_iterate.multipliers.upper_bounds[i] /= objective_multiplier;
        }
    }
    return status;
}