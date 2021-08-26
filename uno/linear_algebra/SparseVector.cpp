#include "SparseVector.hpp"
#include <cmath>

// SparseVector2 methods

SparseVector2::SparseVector2(size_t capacity): indices(capacity), elements(capacity) {
}

void SparseVector2::for_each(const std::function<void (size_t, double)>& f) const {
   for (size_t i = 0; i < this->number_nonzeros; i++) {
      f(this->indices[i], this->elements[i]);
   }
}

size_t SparseVector2::size() const {
   return this->number_nonzeros;
}

void SparseVector2::insert(size_t index, double value) {
   assert(this->number_nonzeros < this->elements.size() && "The capacity of the sparse vector is too low");
   this->indices[this->number_nonzeros] = index;
   this->elements[this->number_nonzeros] = value;
   this->number_nonzeros++;
}

void SparseVector2::clear() {
   this->number_nonzeros = 0;
}

void SparseVector2::transform(const std::function<double (double)>& f) {
   for (size_t i = 0; i < this->number_nonzeros; i++) {
      this->elements[i] = f(this->elements[i]);
   }
}

// free functions

void clear(SparseVector& x) {
   x.clear();
}

void scale(SparseVector& x, double scaling_factor) {
   for (auto& element: x) {
      element.second *= scaling_factor;
   }
}

double norm_1(const SparseVector2& x) {
   double norm = 0.;
   x.for_each([&](size_t /*index*/, double value) {
      norm += std::abs(value);
   });
   return norm;
}


//// https://en.wikipedia.org/wiki/Matrix_norm#Special_cases
//double norm_1(const std::vector <SparseVector>& m) {
//   double norm = 0.;
//   for (const auto& column: m) {
//      double column_norm = norm_1(column);
//      norm = std::max(norm, column_norm);
//   }
//   return norm;
//}

double norm_2_squared(const SparseVector& x) {
   double norm_squared = 0.;
   for (std::pair<int, double> term: x) {
      double xi = term.second;
      norm_squared += xi * xi;
   }
   return norm_squared;
}

double norm_2(const SparseVector& x) {
   return std::sqrt(norm_2_squared(x));
}

double norm_inf(const SparseVector& x) {
   double norm = 0.;
   for (std::pair<int, double> term: x) {
      double xi = term.second;
      norm = std::max(norm, std::abs(xi));
   }
   return norm;
}

// https://en.wikipedia.org/wiki/Matrix_norm#Special_cases
double norm_inf(const std::vector<SparseVector>& m) {
   // compute maximum row index
   unsigned int number_rows = 0;
   for (size_t j = 0; j < m.size(); j++) {
      // TODO
      //number_rows = std::max(number_rows, 1 + m[j].begin()->first);
   }
   // read the matrix column-wise and fill in the row_vectors norm vector
   std::vector<double> row_vectors(number_rows);
   for (size_t j = 0; j < m.size(); j++) {
      for (const auto[i, value]: m[j]) {
         row_vectors[i] += std::abs(value);
      }
   }
   // compute the maximal component of the row_vectors vector
   double norm = 0.;
   for (double& row_vector : row_vectors) {
      norm = std::max(norm, row_vector);
   }
   return norm;
}

double dot(const std::vector<double>& x, const SparseVector& y) {
   double dot = 0.;
   for (const auto[i, yi]: y) {
      if (i < x.size()) {
         dot += x[i] * yi;
      }
      else {
         throw std::length_error("Vector.dot: x and y have different sizes");
      }
   }
   return dot;
}

double dot(const SparseVector& x, const SparseVector& y) {
   double dot = 0.;
   for (const auto[i, xi]: x) {
      try {
         dot += xi * y.at(i);
      }
      catch (std::out_of_range&) {
      }
   }
   return dot;
}

void print_vector(std::ostream &stream, const SparseVector& x, const char end) {
   for (const auto [i, xi]: x) {
      stream << "x[" << i << "] = " << xi << ", ";
   }
   stream << end;
}

void print_vector(const Level& level, const SparseVector& x, const char end) {
   for (const auto [i, xi]: x) {
      level << "x[" << i << "] = " << xi << ", ";
   }
   level << end;
}