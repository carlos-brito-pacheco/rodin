/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_EXTERNAL_MMG_IMPLICITDOMAINMESHER_H
#define RODIN_EXTERNAL_MMG_IMPLICITDOMAINMESHER_H

#include <boost/unordered_map.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "Rodin/Variational.h"

#include "Mesh.h"

#include "MMG5.h"
#include "Common.h"

namespace Rodin::External::MMG
{
  /**
   * @brief Class to perform the discretization and optimization of a
   * surface implicitly defined by a level set function.
   */
  class ImplicitDomainMesher : public MMG5
  {
    public:
      /**
       * @brief Constructs an ImplicitDomainMesher2D with default values.
       */
      ImplicitDomainMesher()
        : m_ls(0.0),
        m_meshTheSurface(false),
        m_idGen(0, std::numeric_limits<int16_t>::max())
    {}

      ImplicitDomainMesher& surface(bool meshTheSurface = true);

      /**
       * @brief Specifies the level set to discretize.
       *
       * The default value is 0.
       *
       * @returns Reference to self (for method chaining)
       */
      ImplicitDomainMesher& setLevelSet(double ls);

      /**
       * @brief Specifies the removal of small parasitic components.
       * @returns Reference to self (for method chaining)
       */
      ImplicitDomainMesher& setRMC(double rmc = 1e-5);

      ImplicitDomainMesher& setBaseReferences(MaterialReference ref)
      {
        return setBaseReferences(std::set<MaterialReference>{ref});
      }

      ImplicitDomainMesher& setBaseReferences(const std::set<MaterialReference>& refs);

      /**
       * @brief Sets the material reference for the discretized boundary
       * @f$ \partial \Omega @f$
       *
       * Specifies what the label for the boundary @f$ \partial \Omega @f$ will
       * be. By default, the value is 10.
       *
       * @returns Reference to self (for method chaining)
       */
      ImplicitDomainMesher& setBoundaryReference(const MaterialReference& ref);

      /**
       * @brief Specifies how to split the materials into an interior and
       * exterior domains.
       *
       * This map specifies for each input material reference the values of the
       * 2 new domains created by the level-set splitting. By default, the
       * material references of the interior and exterior, are 2 and 3
       * respectively.
       *
       * @returns Reference to self (for method chaining)
       */
      ImplicitDomainMesher& setSplit(const SplitMap& split);

      /**
       * @brief Indicates that a material reference should be split.
       * @param[in] ref Material to split
       * @param[in] s Interior and exterior labels
       * @returns Reference to self (for method chaining)
       */
      ImplicitDomainMesher& split(const MaterialReference& ref, const Split& s);

      /**
       * @brief Indicates that a material reference should not be split.
       * @param[in] ref Material to split
       * @param[in] s Interior and exterior labels
       * @returns Reference to self (for method chaining)
       */
      ImplicitDomainMesher& noSplit(const MaterialReference& ref);

      /**
       * @brief Discretizes and optimizes an implicitly defined surface defined
       * by a level set function.
       * @param[in] ls Level set function
       *
       * The material reference of the level set (edge) boundary will be 10.
       */
      template <class FES>
      MMG::Mesh discretize(const Variational::GridFunction<FES>& ls)
      {
        MMG5_pMesh mesh = nullptr;
        try
        {
          mesh = rodinToMesh(
              dynamic_cast<const MMG::Mesh&>(ls.getFiniteElementSpace().getMesh()));
        } catch (std::bad_cast&)
        {
          Alert::Exception() << "Mesh must be of type MMG::Mesh." << Alert::Raise;
        }

        // Erase boundary elements which have the isoref
        // if (m_isoref)
        //  deleteBoundaryRef(mesh, *m_isoref);

        MMG5_pSol sol = createSolution(mesh, ls.getFiniteElementSpace().getVectorDimension());
        copySolution(ls, sol);

        MMG5::setParameters(mesh);

        bool isSurface = ls.getFiniteElementSpace().getMesh().isSurface();

        int retcode = MMG5_STRONGFAILURE;

        if (m_meshTheSurface)
        {
          generateUniqueSplit(ls.getFiniteElementSpace().getMesh().getBoundaryAttributes());
        }
        else if (ls.getFiniteElementSpace().getMesh().getDimension() == 2)
        {
          generateUniqueSplit(ls.getFiniteElementSpace().getMesh().getAttributes());
        }

        switch (mesh->dim)
        {
          case 2:
          {
            assert(!isSurface);
            retcode = discretizeMMG2D(mesh, sol);
            break;
          }
          case 3:
          {
            if (isSurface)
            {
              retcode = discretizeMMGS(mesh, sol);
            }
            else
            {
              retcode = discretizeMMG3D(mesh, sol);
            }
            break;
          }
        }

        if (retcode != MMG5_SUCCESS)
        {
          Alert::Exception()
            << "Failed to discretize the implicit domain."
            << Alert::Raise;
        }

        auto rodinMesh = meshToRodin(mesh);
        destroySolution(sol);
        destroyMesh(mesh);

        for (const auto it : m_uniqueSplit)
        {
          const auto& ref = it.first;
          const auto& split = it.second;
          std::visit(Utility::Overloaded{
              [&](const NoSplitT&) {},
              [&](const Split& s)
              {
                if (m_meshTheSurface)
                {
                  for (auto bit = rodinMesh.getBoundary(); !bit.end(); ++bit)
                  {
                    const Geometry::Index idx = bit->getIndex();
                    const Geometry::Attribute attr = rodinMesh.getFaceAttribute(idx);
                    auto it = m_originalRefMap.find(attr);
                    if (it != m_originalRefMap.end())
                    {
                      MaterialReference originalRef = it->second;
                      const auto& originalSplit = std::get<Split>(getSplitMap().at(originalRef));
                      if (attr == s.interior)
                      rodinMesh.setAttribute(rodinMesh.getDimension() - 1, idx, originalSplit.interior);
                      else if (attr == s.exterior)
                      rodinMesh.setAttribute(rodinMesh.getDimension() - 1, idx, originalSplit.exterior);
                  }
                  else
                  {
                    // The key must have come from a no split
                  }
                }
              }
              else
              {
                for (auto eit = rodinMesh.getElement(); !eit.end(); ++eit)
                {
                  const Geometry::Index idx = eit->getIndex();
                  const Geometry::Attribute attr = rodinMesh.getElementAttribute(idx);
                  auto it = m_originalRefMap.find(attr);
                  if (it != m_originalRefMap.end())
                  {
                    MaterialReference originalRef = it->second;
                    const auto& originalSplit = std::get<Split>(getSplitMap().at(originalRef));
                    if (attr == s.interior)
                      rodinMesh.setAttribute(rodinMesh.getDimension(), idx, originalSplit.interior);
                    else if (attr == s.exterior)
                      rodinMesh.setAttribute(rodinMesh.getDimension(), idx, originalSplit.exterior);
                  }
                  else
                  {
                    // The key must have come from a no split
                  }
                }
              }
              }
          }, split);
        }
        rodinMesh.getHandle().SetAttributes();

        return rodinMesh;
      }

      ImplicitDomainMesher& setAngleDetection(bool enable = true)
      {
        MMG5::setAngleDetection(enable);
        return *this;
      }

      ImplicitDomainMesher& setHMin(double hmin)
      {
        MMG5::setHMin(hmin);
        return *this;
      }

      ImplicitDomainMesher& setHMax(double hmax)
      {
        MMG5::setHMax(hmax);
        return *this;
      }

      ImplicitDomainMesher& setHausdorff(double hausd)
      {
        MMG5::setHausdorff(hausd);
        return *this;
      }

      ImplicitDomainMesher& setGradation(double hgrad)
      {
        MMG5::setGradation(hgrad);
        return *this;
      }

      const SplitMap& getSplitMap() const
      {
        return m_split;
      }

    private:
      int discretizeMMG2D(MMG5_pMesh mesh, MMG5_pSol sol);
      int discretizeMMG3D(MMG5_pMesh mesh, MMG5_pSol sol);
      int discretizeMMGS(MMG5_pMesh mesh, MMG5_pSol sol);

      void generateUniqueSplit(const std::set<Geometry::Attribute>& attr);

      void deleteBoundaryRef(MMG5_pMesh mesh, MaterialReference ref);

      double m_ls;
      SplitMap m_split;
      bool m_meshTheSurface;
      std::optional<double> m_rmc;
      std::set<int> m_lsBaseReferences;
      std::optional<MaterialReference> m_isoref;

      boost::random::mt19937 m_rng;
      boost::random::uniform_int_distribution<> m_idGen;
      SplitMap m_uniqueSplit;
      std::map<int, int> m_originalRefMap;
  };
}
#endif
