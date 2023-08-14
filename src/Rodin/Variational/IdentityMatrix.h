#ifndef RODIN_VARIATIONAL_IDENTITYMATRIX_H
#define RODIN_VARIATIONAL_IDENTITYMATRIX_H

#include "MatrixFunction.h"

namespace Rodin::Variational
{
  /**
   * @brief Represents the identity matrix function.
   *
   * This class represents the matrix function, which at each point, returns
   * the @f$ n @f$ dimensional identity matrix:
   *
   * @f$
   *   F(x) = I_n
   * @f$
   */
  class IdentityMatrix : public MatrixFunctionBase<IdentityMatrix>
  {
    public:
      /**
       * @brief Constructs the identity matrix function.
       * @param[in] n Dimension of identity matrix
       */
      IdentityMatrix(size_t n)
        : m_n(n)
      {}

      IdentityMatrix(const IdentityMatrix& other)
        : MatrixFunctionBase(other),
          m_n(other.m_n)
      {}

      IdentityMatrix(IdentityMatrix&& other)
        : MatrixFunctionBase(std::move(other)),
          m_n(other.m_n)
      {}

      inline
      size_t getRows() const
      {
        return m_n;
      }

      inline
      size_t getColumns() const
      {
        return m_n;
      }

      inline
      auto getValue(const Geometry::Point&) const
      {
        return Math::Matrix::Identity(m_n, m_n);
      }

      inline
      constexpr
      IdentityMatrix& traceOf(const std::set<Geometry::Attribute>&)
      {
        return *this;
      }

      inline IdentityMatrix* copy() const noexcept override
      {
        return new IdentityMatrix(*this);
      }

    private:
      const size_t m_n;
  };
}

#endif
