#ifndef RODIN_VARIATIONAL_TRIALFUNCTION_H
#define RODIN_VARIATIONAL_TRIALFUNCTION_H

#include "Component.h"
#include "GridFunction.h"
#include "ShapeFunction.h"

namespace Rodin::Variational
{
  template <class FESType>
  class TrialFunction final
    : public ShapeFunction<TrialFunction<FESType>, FESType, TrialSpace>
  {
    public:
      using FES = FESType;
      using Parent = ShapeFunction<TrialFunction<FESType>, FESType, TrialSpace>;

      constexpr
      TrialFunction(const FES& fes)
        : Parent(fes)
      {}

      constexpr
      TrialFunction(const TrialFunction& other)
        : Parent(other)
      {}

      constexpr
      TrialFunction(TrialFunction&& other)
        : Parent(std::move(other))
      {}

      void operator=(const TrialFunction&) = delete;

      void operator=(TrialFunction&&) = delete;

      inline
      constexpr
      auto x() const
      {
        assert(this->getFiniteElementSpace().getVectorDimension() >= 1);
        return Component(*this, 0);
      }

      inline
      constexpr
      auto y() const
      {
        assert(this->getFiniteElementSpace().getVectorDimension() >= 2);
        return Component(*this, 1);
      }

      inline
      constexpr
      auto z() const
      {
        assert(this->getFiniteElementSpace().getVectorDimension() >= 3);
        return Component(*this, 2);
      }

      inline
      constexpr
      const TrialFunction& getLeaf() const
      {
        return *this;
      }

      inline TrialFunction* copy() const noexcept override
      {
        return new TrialFunction(*this);
      }
  };

  template <class FES>
  TrialFunction(const FES&) -> TrialFunction<FES>;
}
#endif

