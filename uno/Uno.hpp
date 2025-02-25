// Copyright (c) 2018-2024 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_H
#define UNO_H

#include "optimization/Result.hpp"
#include "optimization/TerminationStatus.hpp"

namespace uno {
   // forward declarations
   class GlobalizationMechanism;
   class Model;
   class Options;
   class Statistics;
   class Timer;

   class Uno {
   public:
      Uno(GlobalizationMechanism& globalization_mechanism, const Options& options);

      void solve(const Model& model, Iterate& initial_iterate, const Options& options);

      static std::string current_version();
      static void print_available_strategies();
      static std::string get_strategy_combination(const Options& options);
      void print_optimization_summary(const Result& result);

   private:
      GlobalizationMechanism& globalization_mechanism; /*!< Globalization mechanism */
      const size_t max_iterations; /*!< Maximum number of iterations */
      const double time_limit; /*!< CPU time limit (can be inf) */
      const bool print_solution;
      const std::string strategy_combination;

      void initialize(Statistics& statistics, Iterate& current_iterate, const Options& options);
      [[nodiscard]] static Statistics create_statistics(const Model& model, const Options& options);
      [[nodiscard]] bool termination_criteria(TerminationStatus current_status, size_t iteration, double current_time) const;
      static void postprocess_iterate(const Model& model, Iterate& iterate, TerminationStatus termination_status);
      [[nodiscard]] Result create_result(const Model& model, Iterate& current_iterate, size_t major_iterations, const Timer& timer);
   };
} // namespace

#endif // UNO_H
