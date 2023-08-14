/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_VARIATIONAL_TRACE_H
#define RODIN_VARIATIONAL_TRACE_H

#include "ForwardDecls.h"

#include "ScalarFunction.h"

namespace Rodin::Variational
{
  /**
   * @defgroup TraceSpecializations
   * @brief Template specializations of the Trace class.
   * @see Trace
   */

  /**
   * @ingroup TraceSpecializations
   * @brief Trace of a FunctionBase instance.
   */
  template <class NestedDerived>
  class Trace<FunctionBase<NestedDerived>> final
    : public ScalarFunctionBase<Trace<FunctionBase<NestedDerived>>>
  {
    public:
      using Operand = FunctionBase<NestedDerived>;
      using Parent = ScalarFunctionBase<Trace<Operand>>;

      /**
       * @brief Constructs the Trace of the given matrix
       * @param[in] m Square matrix
       */
      constexpr
      Trace(const Operand& m)
        : m_operand(m.copy())
      {}

      constexpr
      Trace(const Trace& other)
        : Parent(other),
          m_operand(other.m_operand->copy())
      {}

      constexpr
      Trace(Trace&& other)
        : Parent(std::move(other)),
          m_operand(std::move(other.m_operand))
      {}

      inline
      constexpr
      auto getValue(const Geometry::Point& p) const
      {
        using OperandRange = typename FormLanguage::Traits<Operand>::RangeType;
        static_assert(std::is_same_v<OperandRange, Math::Matrix>);
        return getOperand().getValue(p).trace();
      }

      inline
      constexpr
      const Operand& getOperand() const
      {
        assert(m_operand);
        return *m_operand;
      }

      inline
      Trace& traceOf(Geometry::Attribute attrs)
      {
        m_operand.traceOf(attrs);
        return *this;
      }

      inline Trace* copy() const noexcept override
      {
        return new Trace(*this);
      }

    private:
      std::unique_ptr<Operand> m_operand;
  };

  template <class NestedDerived>
  Trace(const FunctionBase<NestedDerived>&) -> Trace<FunctionBase<NestedDerived>>;
}

#endif
