// Copyright (c) 2022 Charlie Vanaret
// Licensed under the MIT license. See LICENSE file in the project directory for details.

#ifndef UNO_PROGRESSMEASURES_H
#define UNO_PROGRESSMEASURES_H

struct ProgressMeasures {
   double infeasibility{}; // constraint violation
   double scaled_optimality{}; // optimality measure (scaled by penalty parameter): objective, Lagrangian, ...
   double unscaled_optimality{}; // optimality measure (independent of penalty parameter): barrier terms, proximal term, ...
};

#endif // UNO_PROGRESSMEASURES_H
