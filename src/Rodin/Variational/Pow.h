/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_VARIATIONAL_POW_H
#define RODIN_VARIATIONAL_POW_H

#include <cmath>

#include "RangeShape.h"
#include "Function.h"
#include "ScalarFunction.h"

namespace Rodin::Variational
{
  /**
   * @defgroup PowSpecializations Pow Template Specializations
   * @brief Template specializations of the Pow class.
   * @see Pow
   */

  /**
   * @ingroup PowSpecializations
   * @brief Represent the power function.
   *
   * This class represents the function @f$ f : \mathbb{R} \rightarrow
   * \mathbb{R} @f$ defined by the exponentiation of a base value @f$ x \in
   * \mathbb{R} @f$ to the power @f$ p @f$:
   * @f$
   *  f(x) = x^p \ .
   * @f$
   */
  template <class BaseDerived, class Number>
  class Pow<FunctionBase<BaseDerived>, Number>
    : public ScalarFunctionBase<Pow<FunctionBase<BaseDerived>, Number>>
  {
    static_assert(std::is_arithmetic_v<Number>);
    public:
      using Base = FunctionBase<BaseDerived>;
      using Exponent = Number;
      using Parent = ScalarFunctionBase<Pow<FunctionBase<BaseDerived>, Number>>;

      /**
       * @brief Constructs the power object
       * @param[in] s Base value
       * @param[in] p Power
       */
      constexpr
      Pow(const Base& s, Exponent p)
        : m_s(s), m_p(p)
      {}

      constexpr
      Pow(const Pow& other)
        : Parent(other),
          m_s(other.m_s), m_p(other.m_p)
      {}

      constexpr
      Pow(Pow&& other)
        : Parent(std::move(other)),
          m_s(std::move(other.m_s)),
          m_p(std::move(other.m_p))
      {}

      inline
      constexpr
      Pow& traceOf(Geometry::Attribute attrs)
      {
        m_s.traceOf(attrs);
        return *this;
      }

      inline
      constexpr
      auto getValue(const Geometry::Point& p) const
      {
        return std::pow(m_s.getValue(p), m_p);
      }

    private:
      Base m_s;
      const Exponent m_p;
  };

  template <class BaseDerived, class Number>
  Pow(const FunctionBase<BaseDerived>&, Number) -> Pow<FunctionBase<BaseDerived>, Number>;
}

#endif
