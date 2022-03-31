/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_VARIATIONAL_SCALARCOEFFICIENT_H
#define RODIN_VARIATIONAL_SCALARCOEFFICIENT_H

#include <map>
#include <set>
#include <memory>
#include <optional>
#include <type_traits>

#include <mfem.hpp>

#include "ForwardDecls.h"

#include "FormLanguage/Base.h"
#include "FormLanguage/ForwardDecls.h"

namespace Rodin::Variational
{
   /**
    * @brief Abstract base class for objects representing scalar coefficients.
    */
   class ScalarCoefficientBase
      : public FormLanguage::Buildable<mfem::Coefficient>
   {
      public:
         ScalarCoefficientBase() = default;

         ScalarCoefficientBase(const ScalarCoefficientBase&) = default;

         /**
          * @brief Sets an attribute which will be interpreted as the domain to
          * trace.
          *
          * Convenience function to call traceOf(std::set<int>) with only one
          * attribute.
          *
          * @returns Reference to self (for method chaining)
          */
         ScalarCoefficientBase& traceOf(int attr)
         {
            return traceOf(std::set<int>{attr});
         }

         /**
          * @brief Sets which attributes will be interpreted as the domain to
          * trace.
          * @returns Reference to self (for method chaining)
          *
          * When integrating along interior boundaries sometimes it is
          * necessary to specify which attributes should be interpreted as the
          * respective "interior" domain. For example, coefficients which
          * involve the derivatives of a GridFunction need to know the element
          * to "trace".
          *
          * @note Setting the trace domain of a ScalarCoefficientBase instance
          * does not guarantee that it will taken into consideration when
          * computing its value. That said, it is up to the subclass to decide
          * how it will use this information which can be obtained via the
          * getTraceDomain() method.
          *
          * @see @ref ScalarCoefficientBase::getTraceDomain() "getTraceDomain()"
          *
          */
         ScalarCoefficientBase& traceOf(std::set<int> attrs)
         {
            m_traceDomain = attrs;
            return *this;
         }

         /**
          * @brief Gets the set of attributes which will be interpreted as the
          * domains to "trace".
          *
          * The domains to trace are interpreted as the domains where there
          * shall be a continuous extension from values to the interior
          * boundaries. If the trace domain is empty, then this has the
          * semantic value that it has not been specified yet.
          */
         const std::set<int>& getTraceDomain() const
         {
            return m_traceDomain;
         }

         std::unique_ptr<mfem::Coefficient> build() const override;

         virtual ~ScalarCoefficientBase() = default;

         virtual Restriction<ScalarCoefficientBase> restrictTo(int attr);

         virtual Restriction<ScalarCoefficientBase> restrictTo(const std::set<int>& attrs);

         virtual double getValue(mfem::ElementTransformation& trans, const mfem::IntegrationPoint& ip
               ) const = 0;

         virtual ScalarCoefficientBase* copy() const noexcept override = 0;

      private:
         std::set<int> m_traceDomain;
   };

   template <>
   class ScalarCoefficient<ScalarCoefficientBase> : public ScalarCoefficientBase
   {
      public:
         ScalarCoefficient(const ScalarCoefficientBase& nested)
            : m_nested(nested.copy())
         {}

         ScalarCoefficient(const ScalarCoefficient& other)
            : m_nested(other.m_nested->copy())
         {}

         ScalarCoefficient(ScalarCoefficient&& other) = default;

         double getValue(mfem::ElementTransformation& trans, const mfem::IntegrationPoint& ip) const override
         {
            return m_nested->getValue(trans, ip);
         }

         ScalarCoefficient* copy() const noexcept override
         {
            return new ScalarCoefficient(*this);
         }

      private:
         std::unique_ptr<ScalarCoefficientBase> m_nested;
   };
   ScalarCoefficient(const ScalarCoefficientBase&)
      -> ScalarCoefficient<ScalarCoefficientBase>;

   /**
    * @brief Represents a ScalarCoefficient of arithmetic type `T`.
    *
    * @tparam T Arithmetic type
    * @see [std::is_arithmetic](https://en.cppreference.com/w/cpp/types/is_arithmetic)
    */
   template <class T>
   class ScalarCoefficient : public ScalarCoefficientBase
   {
      public:
         static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

         /**
          * @brief Constructs a ScalarCoefficient from an arithmetic value.
          * @param[in] x Arithmetic value
          */
         template <typename U = T>
         constexpr
         ScalarCoefficient(typename std::enable_if_t<std::is_arithmetic_v<U>, U> x)
            : m_x(x)
         {}

         constexpr
         ScalarCoefficient(const ScalarCoefficient& other) = default;

         constexpr
         ScalarCoefficient(ScalarCoefficient&&) = default;

         constexpr
         T getValue() const
         {
            return m_x;
         }

         double getValue(mfem::ElementTransformation&, const mfem::IntegrationPoint&) const override
         {
            return m_x;
         }

         ScalarCoefficient* copy() const noexcept override
         {
            return new ScalarCoefficient(*this);
         }

      private:
         const T m_x;
   };
   template <class T>
   ScalarCoefficient(const T&)
      -> ScalarCoefficient<std::enable_if_t<std::is_arithmetic_v<T>, T>>;

   /**
    * @brief Represents a scalar coefficient which is built from a
    * GridFunction.
    *
    * @tparam FEC Finite element collection
    */
   template <class FEC>
   class ScalarCoefficient<GridFunction<FEC>>
      : public ScalarCoefficientBase
   {
      public:
         /**
          * @brief Constructs a ScalarCoefficient from a GridFunction u
          * @param[in] u GridFunction which belongs to the finite element
          * collection FEC
          */
         constexpr
         ScalarCoefficient(const GridFunction<FEC>& u)
            :  m_u(u),
               m_mfemCoefficient(&u.getHandle())
         {
            assert(u.getFiniteElementSpace().getVectorDimension() == 1);
         }

         constexpr
         ScalarCoefficient(const ScalarCoefficient& other) = default;

         const GridFunction<FEC>& getValue() const
         {
            return m_u;
         }

         double getValue(mfem::ElementTransformation& trans, const mfem::IntegrationPoint& ip) const override
         {
            return m_mfemCoefficient.Eval(trans, ip);
         }

         ScalarCoefficient* copy() const noexcept override
         {
            return new ScalarCoefficient(*this);
         }

      private:
         const GridFunction<FEC>& m_u;
         mutable mfem::GridFunctionCoefficient m_mfemCoefficient;
   };
   template <class FEC>
   ScalarCoefficient(const GridFunction<FEC>&)
      -> ScalarCoefficient<GridFunction<FEC>>;

   template <>
   class ScalarCoefficient<std::function<double(const double*, int)>>
      : public ScalarCoefficientBase
   {
      public:
         ScalarCoefficient(std::function<double(const double*, int)> f)
            : m_f(f)
         {}

         ScalarCoefficient(const ScalarCoefficient& other) = default;

         std::function<double(const double*, int)> getValue() const
         {
            return m_f;
         }

         double getValue(mfem::ElementTransformation& trans, const mfem::IntegrationPoint& ip) const override
         {
            double x[3];
            mfem::Vector transip(x, 3);
            trans.Transform(ip, transip);
            return m_f(transip.GetData(), transip.Size());
         }

         ScalarCoefficient* copy() const noexcept override
         {
            return new ScalarCoefficient(*this);
         }

      private:
         const std::function<double(const double*, int)> m_f;
   };
   ScalarCoefficient(std::function<double(const double*, int)>)
      -> ScalarCoefficient<std::function<double(const double*, int)>>;

   template <>
   class ScalarCoefficient<std::map<int, double>>
      : public ScalarCoefficientBase
   {
      public:
         ScalarCoefficient(const std::map<int, double>& pieces)
            : m_pieces(pieces),
              m_mfemCoefficient(pieces.rbegin()->first) // Maximum attribute
         {
            int maxAttr = m_pieces.rbegin()->first;
            for (int i = 1; i <= maxAttr; i++)
            {
               auto v = m_pieces.find(i);
               if (v != m_pieces.end())
                  m_mfemCoefficient(i) = v->second;
               else
                  m_mfemCoefficient(i) = 0.0;
            }
         }

         ScalarCoefficient(const ScalarCoefficient& other)
            : m_pieces(other.m_pieces)
         {}

         const std::map<int, double>& getValue() const
         {
            return m_pieces;
         }

         double getValue(mfem::ElementTransformation& trans, const mfem::IntegrationPoint& ip) const override
         {
            return m_mfemCoefficient.Eval(trans, ip);
         }

         ScalarCoefficient* copy() const noexcept override
         {
            return new ScalarCoefficient(*this);
         }
      private:
         std::map<int, double> m_pieces;
         mutable mfem::PWConstCoefficient m_mfemCoefficient;
   };
   ScalarCoefficient(const std::map<int, double>&)
      -> ScalarCoefficient<std::map<int, double>>;
   ScalarCoefficient(std::initializer_list<std::pair<int, double>>&)
      -> ScalarCoefficient<std::map<int, double>>;
}

namespace Rodin::Variational::Internal
{
   class ProxyScalarCoefficient : public mfem::Coefficient
   {
      public:
         ProxyScalarCoefficient(const ScalarCoefficientBase& s)
            : m_s(s)
         {}

         ProxyScalarCoefficient(const ProxyScalarCoefficient& other)
            : mfem::Coefficient(other),
              m_s(other.m_s)
         {}

         ProxyScalarCoefficient(ProxyScalarCoefficient&& other)
            : mfem::Coefficient(std::move(other)),
              m_s(other.m_s)
         {}

         double Eval(mfem::ElementTransformation& trans, const mfem::IntegrationPoint& ip) override
         {
            return m_s.getValue(trans, ip);
         }
      private:
         const ScalarCoefficientBase& m_s;
   };
}

#endif
