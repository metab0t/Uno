#ifndef LINEARSOLVER_H
#define LINEARSOLVER_H

#include <vector>

template <class SparseSymmetricMatrix>
class LinearSolver {
public:
   // make this type accessible in templates
   using matrix_type = SparseSymmetricMatrix;

   LinearSolver() = default;
   virtual ~LinearSolver() = default;

   // matrix is not declared const, since Fortran-based solvers may need to temporarily reindex the coordinates
   virtual void factorize(SparseSymmetricMatrix& matrix) = 0;
   virtual void do_symbolic_factorization(SparseSymmetricMatrix& matrix) = 0;
   virtual void do_numerical_factorization(SparseSymmetricMatrix& matrix) = 0;
   virtual std::vector<double> solve(SparseSymmetricMatrix& matrix, const std::vector<double>& rhs) = 0;

   [[nodiscard]] virtual std::tuple<int, int, int> get_inertia() const = 0;
   [[nodiscard]] virtual size_t number_negative_eigenvalues() const = 0;
   [[nodiscard]] virtual bool matrix_is_singular() const = 0;
   [[nodiscard]] virtual int rank() const = 0;
};

#endif // LINEARSOLVER_H
