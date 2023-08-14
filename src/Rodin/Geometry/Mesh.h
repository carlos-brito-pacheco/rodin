/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_GEOMETRY_MESH_H
#define RODIN_GEOMETRY_MESH_H

#include <set>
#include <string>
#include <deque>

#include <mfem.hpp>

#include <boost/filesystem.hpp>

#include "Rodin/Math.h"
#include "Rodin/Types.h"
#include "Rodin/IO/ForwardDecls.h"
#include "Rodin/Variational/ForwardDecls.h"

#include "ForwardDecls.h"
#include "Connectivity.h"
#include "Simplex.h"
#include "SimplexIterator.h"
#include "SimplexTransformation.h"

namespace Rodin::Geometry
{
  /**
  * @brief Abstract base class for Mesh objects.
  */
  class MeshBase
  {
    public:
      class BuilderBase
      {
        public:
          virtual ~BuilderBase() = default;

          virtual void finalize() = 0;
      };

      virtual ~MeshBase() = default;

      virtual MeshBase& scale(Scalar c) = 0;

      /**
       * @brief Displaces the mesh nodes by the displacement @f$ u @f$.
       * @param[in] u Displacement at each node
       *
       * Given a grid function @f$ u @f$, the method will perform the
       * displacement
       * @f[
       *   x \mapsto x + u(x)
       * @f]
       * at each node @f$ x @f$ of the mesh.
       *
       * @note The vector dimension of @f$ u @f$ must be equal to the
       * space dimension.
       *
       * @returns Reference to this (for method chaining)
       */
      template <class FES>
      MeshBase& displace(const Variational::GridFunction<FES>& u)
      {
        assert(u.getFiniteElementSpace().getVectorDimension() == getSpaceDimension());
        getHandle().MoveNodes(u.getHandle());
        flush();
        return *this;
      }

      /**
       * @brief Gets the maximum number @f$ t @f$ by which the mesh will
       * remain valid, when displacing by @f$ u @f$.
       * @param[in] u Displacement at each node
       *
       * This function will calculate the maximum number @f$ t @f$ so that
       * the displacement
       * @f[
       *   x \mapsto x + t u(x)
       * @f]
       * gives a valid mesh without actually displacing the mesh.
       *
       * @note The vector dimension of @f$ u @f$ must be equal to the
       * space dimension.
       *
       * @returns Maximum time so that the mesh remains valid.
       */
      template <class FES>
      Scalar getMaximumDisplacement(const Variational::GridFunction<FES>& u)
      {
        Scalar res;
        getHandle().CheckDisplacements(u.getHandle(), res);
        return res;
      }

      // /**
      //  * @brief Performs connected-component labelling.
      //  * @param[in] p Function which returns true if two adjacent elements
      //  * belong to the same component, false otherwise.
      //  * @returns List of sets, each set representing a component containing
      //  * the indices of its elements.
      //  *
      //  * @note Both elements passed to the function will always be adjacent
      //  * to each other, i.e. it is not necessary to verify this is the case.
      //  */
      // std::deque<std::set<int>> ccl(
      //     std::function<bool(const Element&, const Element&)> p) const;

      // /**
      //  * @brief Edits the specified elements in the mesh via the given function.
      //  * @param[in] f Function which takes an ElementView to modify each
      //  * element.
      //  * @param[in] elements Set of indices corresponding to the elements
      //  * which will be modified.
      //  */
      // MeshBase& edit(std::function<void(ElementView)> f, const std::set<int>& elements);

      // /**
      //  * @brief Obtains a set of elements satisfying the given condition.
      //  * @param[in] condition Function which returns true if the element
      //  * satisfies the condition.
      //  * @returns Set containing the element indices satisfying the
      //  * condition.
      //  */
      // std::set<int> where(std::function<bool(const Element&)> condition) const;

      // std::set<int> where(std::function<bool(const Point&)> condition) const;

      /**
       * @brief Indicates whether the mesh is a surface or not.
       * @returns True if mesh is a surface, false otherwise.
       *
       * A surface mesh is a mesh of codimension 1. That is, the difference
       * between its space dimension and dimension is 1.
       */
      bool isSurface() const;

      size_t getVertexCount() const
      {
        return getCount(0);
      }

      size_t getFaceCount() const
      {
        return getCount(getDimension() - 1);
      }

      size_t getElementCount() const
      {
        return getCount(getDimension());
      }

      Attribute getFaceAttribute(Index index) const
      {
        return getAttribute(getDimension() - 1, index);
      }

      Attribute getElementAttribute(Index index) const
      {
        return getAttribute(getDimension(), index);
      }

      /**
       * @brief Gets the total volume of the mesh.
       * @returns Sum of all element volumes.
       */
      Scalar getVolume();

      /**
       * @brief Gets the sum of the volumes of the elements given by the
       * specified attribute.
       * @param[in] attr Attribute of elements
       * @returns Sum of element volumes with given attribute
       * @note If the element attribute does not exist then this function
       * will return 0 as the volume.
       */
      Scalar getVolume(Attribute attr);

      /**
       * @brief Gets the total perimeter of the mesh.
       * @returns Sum of all element perimeters.
       */
      Scalar getPerimeter();

      /**
       * @brief Gets the sum of the perimeters of the elements given by the
       * specified attribute.
       * @param[in] attr Attribute of elements
       * @returns Sum of element perimeters with given attribute
       * @note If the element attribute does not exist then this function
       * will return 0 as the perimeter.
       */
      Scalar getPerimeter(Attribute attr);

      /**
       * @brief Gets the labels of the domain elements in the mesh.
       * @returns Set of all the attributes in the mesh object.
       * @see getBoundaryAttributes() const
       */
      std::set<Attribute> getAttributes() const;

      /**
       * @brief Gets the labels of the boundary elements in the mesh.
       * @returns Set of all the boundary attributes in the mesh object.
       * @see getAttributes() const
       */
      std::set<Attribute> getBoundaryAttributes() const;

      bool operator==(const MeshBase& other) const
      {
        return this == &other;
      }

      bool operator!=(const MeshBase& other) const
      {
        return this != &other;
      }

      /**
       * @brief Gets the dimension of the elements.
       * @returns Dimension of the elements.
       * @see getSpaceDimension() const
       */
      virtual size_t getDimension() const = 0;

      /**
       * @brief Gets the dimension of the ambient space
       * @returns Dimension of the space which the mesh is embedded in
       * @see getDimension() const
       */
      virtual size_t getSpaceDimension() const = 0;

      virtual MeshBase& load(
        const boost::filesystem::path& filename,
        IO::FileFormat fmt = IO::FileFormat::MFEM) = 0;

      virtual void save(
        const boost::filesystem::path& filename,
        IO::FileFormat fmt = IO::FileFormat::MFEM, size_t precison = 16) const = 0;

      /**
       * @brief Indicates whether the mesh is a submesh or not.
       * @returns True if mesh is a submesh, false otherwise.
       *
       * A Mesh which is also a SubMesh may be casted into down to access
       * the SubMesh functionality. For example:
       * @code{.cpp}
       * if (mesh.isSubMesh())
       * {
       *   // Cast is well defined
       *   auto& submesh = static_cast<SubMesh&>(mesh);
       * }
       * @endcode
       *
       */
      virtual bool isSubMesh() const = 0;

      virtual bool isInterface(Index faceIdx) const = 0;

      virtual bool isBoundary(Index faceIdx) const = 0;

      virtual FaceIterator getBoundary() const = 0;

      virtual FaceIterator getInterface() const = 0;

      virtual size_t getCount(size_t dim) const = 0;

      virtual ElementIterator getElement(Index idx = 0) const = 0;

      virtual FaceIterator getFace(Index idx = 0) const = 0;

      virtual VertexIterator getVertex(Index idx = 0) const = 0;

      virtual SimplexIterator getSimplex(size_t dimension, Index idx) const = 0;

      virtual const SimplexTransformation& getSimplexTransformation(
          size_t dimension, Index idx) const = 0;

      virtual Attribute getAttribute(size_t dimension, Index index) const = 0;

      virtual MeshBase& setAttribute(size_t dimension, Index index, Attribute attr) = 0;

      virtual const Connectivity& getConnectivity(size_t d, size_t dp) const = 0;

      virtual void flush() = 0;

      /**
       * @internal
       * @brief Gets the underlying handle for the internal mesh.
       * @returns Constant reference to the underlying mfem::Mesh.
       */
      virtual mfem::Mesh& getHandle() const = 0;
  };

  using SerialMesh = Mesh<Context::Serial>;

  /**
  * @brief Represents the subdivision of some domain into faces of (possibly)
  * different geometries.
  */
  template <>
  class Mesh<Context::Serial> : public MeshBase
  {
    public:
      class Builder : public BuilderBase
      {
        public:
          Builder();

          Builder(const Builder&) = delete;

          Builder(Builder&&) = default;

          Builder& operator=(const Builder&) = delete;

          Builder& operator=(Builder&&) = default;

          Builder& setReference(Mesh<Context::Serial>& mesh);

          Builder& vertex(std::initializer_list<Scalar> l)
          {
            Math::Vector x(l.size());
            std::copy(l.begin(), l.end(), x.begin());
            return vertex(x);
          }

          Builder& vertex(const Math::Vector& x);

          Builder& face(Type geom, const Array<Index>& vs,
              Attribute attr = RODIN_DEFAULT_SIMPLEX_ATTRIBUTE);

          Builder& element(Type geom, const Array<Index>& vs,
              Attribute attr = RODIN_DEFAULT_SIMPLEX_ATTRIBUTE);

          Builder& face(Type geom, std::initializer_list<Index> vs,
              Attribute attr = RODIN_DEFAULT_SIMPLEX_ATTRIBUTE)
          {
            Array<Index> as(vs.size());
            std::copy(vs.begin(), vs.end(), as.begin());
            return face(geom, as, attr);
          }

          Builder& element(Type geom, std::initializer_list<Index> vs,
              Attribute attr = RODIN_DEFAULT_SIMPLEX_ATTRIBUTE)
          {
            Array<Index> as(vs.size());
            std::copy(vs.begin(), vs.end(), as.begin());
            return element(geom, as, attr);
          }

          void finalize() override;

        private:
          std::optional<std::reference_wrapper<Mesh<Context::Serial>>> m_ref;
          size_t m_dim, m_sdim;
          std::vector<size_t> m_count;
          std::vector<std::vector<Connectivity>> m_connectivity;
          std::vector<std::vector<std::unique_ptr<SimplexTransformation>>> m_transformations;
          std::unique_ptr<mfem::Mesh> m_impl;
      };

      /**
      * @brief Constructs an empty mesh with no elements.
      */
      Mesh()
        : m_dim(0), m_sdim(0)
      {}

      Mesh(const boost::filesystem::path& filename, IO::FileFormat fmt = IO::FileFormat::MFEM)
      {
        load(filename, fmt);
      }

      /**
      * @brief Move constructs the mesh from another mesh.
      */
      Mesh(Mesh&& other) = default;

      /**
      * @brief Performs a copy of another mesh.
      */
      Mesh(const Mesh& other)
        : m_dim(other.m_dim), m_sdim(other.m_sdim),
          m_count(other.m_count),
          m_connectivity(other.m_connectivity),
          m_f2b(other.m_f2b)
      {
        m_impl.reset(new mfem::Mesh(*other.m_impl));
      }

      /**
      * @internal
      * @brief Move constructs a Rodin::Mesh from an mfem::Mesh.
      */
      explicit Mesh(mfem::Mesh&& mesh);

      /**
      * @brief Move assigns the mesh from another mesh.
      */
      Mesh& operator=(Mesh&& other) = default;

      Mesh<Context::Serial>::Builder initialize(size_t dim, size_t sdim);

      /**
      * @brief Loads a mesh from file in the given format.
      * @param[in] filename Name of file to read
      * @param[in] fmt Mesh file format
      * @returns Reference to this (for method chaining)
      */
      virtual Mesh& load(
        const boost::filesystem::path& filename,
        IO::FileFormat fmt = IO::FileFormat::MFEM) override;

      /**
      * @brief Saves a mesh to file in the given format.
      * @param[in] filename Name of file to write
      * @param[in] fmt Mesh file format
      * @returns Reference to this (for method chaining)
      */
      virtual void save(
        const boost::filesystem::path& filename,
        IO::FileFormat fmt = IO::FileFormat::MFEM, size_t precison = 16) const override;

      virtual Mesh& scale(Scalar c) override;

      virtual Mesh& setAttribute(size_t dimension, Index index, Attribute attr) override;

      /**
      * @brief Skins the mesh to obtain its boundary mesh
      * @returns SubMesh object to the boundary region of the mesh
      *
      * This function will "skin" the mesh to return the mesh boundary as a
      * new SubMesh object. The new mesh will be embedded in the original
      * space dimension.
      */
      virtual SubMesh<Context::Serial> skin() const;

      /**
      * @brief Trims the elements with the given material reference.
      * @param[in] attr Attribute to trim
      * @returns SubMesh object to the remaining region of the mesh
      *
      * Convenience function to call trim(const std::set<Attribute>&) with
      * only one attribute.
      */
      virtual SubMesh<Context::Serial> trim(Attribute attr);

      /**
      * @brief Trims the elements with the given material references.
      * @param[in] attrs Attributes to trim
      * @returns SubMesh object to the remaining region of the mesh
      *
      * This function will trim the current mesh and return a Submesh
      * object containing the elements which were not trimmed from the
      * original mesh.
      */
      virtual SubMesh<Context::Serial> trim(const std::set<Attribute>& attrs);

      virtual SubMesh<Context::Serial> keep(Attribute attr);

      virtual SubMesh<Context::Serial> keep(const std::set<Attribute>& attrs);

      // virtual SubMesh<Context::Serial> keep(std::function<bool(const Element&)> pred);

      size_t getSimplexCount(size_t dim) const
      {
        return getCount(dim);
      }

      virtual size_t getCount(size_t dim) const override;

      virtual FaceIterator getBoundary() const override;

      virtual FaceIterator getInterface() const override;

      virtual ElementIterator getElement(Index idx = 0) const override;

      virtual FaceIterator getFace(Index idx = 0) const override;

      virtual VertexIterator getVertex(Index idx = 0) const override;

      virtual SimplexIterator getSimplex(size_t dimension, Index idx) const override;

      virtual bool isSubMesh() const override
      {
        return false;
      }

      virtual bool isInterface(Index faceIdx) const override;

      virtual bool isBoundary(Index faceIdx) const override;

      virtual size_t getDimension() const override;

      virtual size_t getSpaceDimension() const override;

      virtual const SimplexTransformation& getSimplexTransformation(
          size_t dimension, Index idx) const override;

      virtual Attribute getAttribute(size_t dimension, Index index) const override;

      virtual const Connectivity& getConnectivity(size_t d, size_t dp) const override
      {
        assert(m_connectivity.size() > d);
        assert(m_connectivity[d].size() > dp);

        // TODO: Other dimensions not implemented yet
        assert(d == getDimension()); // TODO: Remove
        assert(dp == 0); // TODO: Remove
        return m_connectivity[d][dp];
      }

      virtual void flush() override
      {
        for (size_t d = 0; d < m_transformations.size(); d++)
          for (size_t i = 0; i < m_transformations[d].size(); i++)
            m_transformations[d][i].reset();
      }

      mfem::Mesh& getHandle() const override;

    private:
      size_t m_dim, m_sdim;
      std::vector<size_t> m_count;
      std::vector<std::vector<Connectivity>> m_connectivity;
      mutable std::vector<std::vector<std::unique_ptr<SimplexTransformation>>> m_transformations;

      std::map<Index, Index> m_f2b;
      std::unique_ptr<mfem::Mesh> m_impl;
  };
}

#include "Mesh.hpp"

#endif
