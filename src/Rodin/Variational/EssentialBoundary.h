#ifndef RODIN_VARIATIONAL_ESSENTIALBOUNDARY_H
#define RODIN_VARIATIONAL_ESSENTIALBOUNDARY_H

#include <map>
#include <memory>
#include <variant>

#include "Rodin/Utility/Overloaded.h"

#include "Component.h"
#include "DirichletBC.h"
#include "TrialFunction.h"
#include "ScalarCoefficient.h"
#include "VectorCoefficient.h"

namespace Rodin::Variational
{
   class EssentialBoundary
   {
      using ScalarValue = std::unique_ptr<ScalarCoefficientBase>;
      using VectorValue = std::unique_ptr<VectorCoefficientBase>;

      struct TrialFunctionValue
      {
         std::variant<ScalarValue, VectorValue> value;
         std::set<int> attributes;
      };

      struct ComponentFunctionValue
      {
         ScalarValue value;
         std::set<int> attributes;
      };

      public:
         EssentialBoundary() = default;

         EssentialBoundary(const EssentialBoundary& other)
         {
            for (auto it = other.m_tfVal.begin(); it != other.m_tfVal.end(); it++)
            {
               auto uuid = it->first;
               const auto& tfValue = it->second;
               m_tfVal[uuid].attributes = tfValue.attributes;
               std::visit(
                  [&](auto&& v)
                  {
                     using T = std::decay_t<decltype(v)>;
                     m_tfVal[uuid].value.emplace<T>(v->copy());
                  }, tfValue.value);
            }

            for (const auto& [uuid, valueMap] : other.m_tfCompVal)
               for (const auto& [idx, compValue] : valueMap)
                  m_tfCompVal[uuid][idx] =
                     ComponentFunctionValue{
                        ScalarValue(compValue.value->copy()),
                        compValue.attributes
                     };
         }

         EssentialBoundary(EssentialBoundary&& other)
            : m_tfVal(std::move(other.m_tfVal)),
              m_tfCompVal(std::move(other.m_tfCompVal))
         {}

         template <class FES>
         void add(const DirichletBC<TrialFunction<FES>, ScalarCoefficientBase>& dbc)
         {
            m_tfVal[dbc.getTrialFunction().getUUID()] =
               TrialFunctionValue{
                  ScalarValue(dbc.getValue().copy()),
                  dbc.getBoundaryAttributes()
               };
         }

         template <class FES>
         void add(const DirichletBC<TrialFunction<FES>, VectorCoefficientBase>& dbc)
         {
            m_tfVal[dbc.getTrialFunction().getUUID()] =
               TrialFunctionValue{
                  VectorValue(dbc.getValue().copy()),
                  dbc.getBoundaryAttributes()
               };
         }

         template <class FES>
         void add(const DirichletBC<Component<TrialFunction<FES>>, ScalarCoefficientBase>& dbc)
         {
            auto uuid = dbc.getComponent().getTrialFunction().getUUID();
            m_tfCompVal[uuid][dbc.getComponent().getIndex()] =
               ComponentFunctionValue{
                  ScalarValue(dbc.getValue().copy()),
                  dbc.getBoundaryAttributes()
               };
         }

         const std::map<boost::uuids::uuid, TrialFunctionValue>& getTFMap() const
         {
            return m_tfVal;
         }

         const std::map<boost::uuids::uuid, std::map<int, ComponentFunctionValue>>& getTFCompMap() const
         {
            return m_tfCompVal;
         }

      private:
         std::map<boost::uuids::uuid, TrialFunctionValue> m_tfVal;
         std::map<boost::uuids::uuid, std::map<int, ComponentFunctionValue>> m_tfCompVal;
   };
}

#endif
