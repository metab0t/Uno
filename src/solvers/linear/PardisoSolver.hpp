#ifndef PARDISOSOLVER_H
#define PARDISOSOLVER_H

#include <vector>
#include <cassert>
#include "CSCSymmetricMatrix.hpp"
#include "LinearSolver.hpp"

enum PardisoPhase {
   ANALYSIS = 11,
   ANALYSIS_FACTORIZATION = 12,
   ANALYSIS_FACTORIZATION_SOLVE = 13,
   NUMERICAL_FACTORIZATION = 22,
   SELECTED_INVERSION = -22,
   NUMERICAL_FACTORIZATION_SOLVE = 23,
   SOLVE = 33,
   MEMORY_DEALLOCATION = -1,
   MATRIX_MEMORY_DEALLOCATION = 0
};

/*! \class PardisoSolver
 *
 *  Interface to the sparse symmetric linear solver MA57
 */
class PardisoSolver : public LinearSolver<CSCSymmetricMatrix> {
public:
   PardisoSolver();
   ~PardisoSolver() override = default;

   void factorize(CSCSymmetricMatrix& matrix) override;
   void do_symbolic_factorization(CSCSymmetricMatrix& matrix) override;
   void do_numerical_factorization(CSCSymmetricMatrix& matrix) override;
   std::vector<double> solve(CSCSymmetricMatrix& matrix, const std::vector<double>& rhs) override;

   [[nodiscard]] std::tuple<int, int, int> get_inertia() const override;
   [[nodiscard]] size_t number_positive_eigenvalues() const;
   [[nodiscard]] size_t number_negative_eigenvalues() const override;
   [[nodiscard]] bool matrix_is_singular() const override;
   [[nodiscard]] int rank() const override;

private:
   int n;
   /* Internal solver memory pointer pt,                  */
   /* 32-bit: int pt[64]; 64-bit: long int pt[64]         */
   /* or void *pt[64] should be OK on both architectures  */
   std::array<long int, 64> pt{};
   const int solver{0}; /* use sparse direct solver */
   const int mtype{-2}; /* Real symmetric matrix */
   /* Pardiso control parameters */
   std::array<int, 64> iparm{};
   std::array<double, 64> dparm{};
   const int nrhs{1}; // Number of right hand sides
   const int mnum{1}; // Which factorization to use
   const int maxfct{1}; // Maximum number of numerical factorizations
   const int msglvl{0}; // Print statistical information

   std::array<double, 5> cntl{};
   std::array<int, 20> icntl{};
   std::array<double, 20> rinfo{};

   //PardisoFactorization factorization;
};

#endif // PARDISOSOLVER_H
