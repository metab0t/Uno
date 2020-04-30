#include "SubproblemFactory.hpp"
#include "SQP.hpp"
#include "SLP.hpp"
#include "Sl1QP.hpp"
#include "SLPEQP.hpp"
#include "InteriorPoint.hpp"
#include "QPSolverFactory.hpp"

std::shared_ptr<Subproblem> SubproblemFactory::create(const std::string& type, QPSolver& solver, HessianEvaluation& hessian_evaluation, std::map<std::string, std::string> default_values) {
    /* active-set methods */
    if (type == "SQP") {
        return std::make_shared<SQP>(solver, hessian_evaluation);
    }
    else if (type == "SLP") {
        return std::make_shared<SLP>(solver);
    }
    else if (type == "Sl1QP") {
        return std::make_shared<Sl1QP>(solver, hessian_evaluation);
    }
    else if (type == "SLPEQP") {
        return std::make_shared<SLPEQP>(solver, hessian_evaluation);
    }
    /* interior point method */
    else if (type == "IPM") {
        return std::make_shared<InteriorPoint>(hessian_evaluation);
    }
    //else if (type == "AL") {
    //    return std::make_shared<AugmentedLagrangian>();
    //}
    else {
        throw std::invalid_argument("Subproblem method " + type + " does not exist");
    }
}
