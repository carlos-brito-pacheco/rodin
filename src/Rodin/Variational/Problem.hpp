/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_VARIATIONAL_PROBLEM_HPP
#define RODIN_VARIATIONAL_PROBLEM_HPP

#include <chrono>

#include "Rodin/Utility.h"
#include "Assembly/Native.h"

#include "GridFunction.h"
#include "DirichletBC.h"

#include "Problem.h"
#include "UnaryMinus.h"

namespace Rodin::Variational
{
   // ------------------------------------------------------------------------
   // ---- Problem<TrialFES, TestFES, Context::Serial, mfem::SparseMatrix, mfem::Vector>
   // ------------------------------------------------------------------------

   template <class TrialFES, class TestFES>
   constexpr
   Problem<TrialFES, TestFES, Context::Serial, mfem::SparseMatrix, mfem::Vector>
   ::Problem(TrialFunction<TrialFES>& u, TestFunction<TestFES>& v)
      :  m_trialFunction(u),
         m_testFunction(v),
         m_linearForm(v),
         m_bilinearForm(u, v)
   {}

   template <class TrialFES, class TestFES>
   Problem<TrialFES, TestFES, Context::Serial, mfem::SparseMatrix, mfem::Vector>&
   Problem<TrialFES, TestFES, Context::Serial, mfem::SparseMatrix, mfem::Vector>
   ::operator=(ProblemBody&& rhs)
   {
      Parent::operator=(std::move(rhs));

      for (auto& bfi : getProblemBody().getBFIs())
         getBilinearForm().add(bfi);

      for (auto& lfi : getProblemBody().getLFIs())
         getLinearForm().add(UnaryMinus(lfi)); // Negate every linear form

      return *this;
   }

   template <class TrialFES, class TestFES>
   void
   Problem<TrialFES, TestFES, Context::Serial, mfem::SparseMatrix, mfem::Vector>::assemble()
   {
      // Assemble both sides
      getLinearForm().assemble();
      getBilinearForm().assemble();

      // Emplace data
      getTrialFunction().emplace();
      getTestFunction().emplace();

      // Project values onto the essential boundary and compute essential dofs
      for (const auto& dbc : getProblemBody().getDBCs())
      {
         dbc.project();
         m_trialEssTrueDofList.Append(dbc.getDOFs());
      }

      m_trialEssTrueDofList.Sort();
      m_trialEssTrueDofList.Unique();

      const auto& trialFes = getTrialFunction().getFiniteElementSpace();
      const auto& testFes = getTestFunction().getFiniteElementSpace();

      if constexpr (std::is_same_v<TrialFES, TestFES>)
      {
         assert(&trialFes == &testFes);

         // Form linear system
         m_stiffnessOp.Swap(getBilinearForm().getOperator());
         m_tmp.reset(new mfem::BilinearForm(&trialFes.getHandle()));
         m_tmp->Assemble();
         m_tmp->SpMat().Swap(m_stiffnessOp);
         m_tmp->FormLinearSystem(
                    m_trialEssTrueDofList,
                    getTrialFunction().getSolution().getHandle(),
                    getLinearForm().getVector(),
                    m_stiffnessOp, m_guess, m_massVector);
         m_tmp->SpMat().Swap(m_stiffnessOp);
      }
      else
      {
         assert(false); // Not supported yet
      }
   }

   template <class TrialFES, class TestFES>
   void
   Problem<TrialFES, TestFES, Context::Serial, mfem::SparseMatrix, mfem::Vector>
   ::solve(const Solver::SolverBase<OperatorType, VectorType>& solver)
   {
      // Assemble the system
      assemble();

      // Solve the system Ax = b
      solver.solve(getStiffnessOperator(), m_guess, getMassVector());

      // Recover solution
      m_tmp->RecoverFEMSolution(
            m_guess,
            getLinearForm().getVector(),
            getTrialFunction().getSolution().getHandle());
   }
}

#endif
