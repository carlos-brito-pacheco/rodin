/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#include <Rodin/Solver.h>
#include <Rodin/Geometry.h>
#include <Rodin/Variational.h>
#include <Rodin/Variational/LinearElasticity.h>
#include <RodinExternal/MMG.h>

using namespace Rodin;
using namespace Rodin::External;
using namespace Rodin::Geometry;
using namespace Rodin::Variational;

// Define boundary attributes
static constexpr Geometry::Attribute Gamma0 = 1; // Traction free boundary
static constexpr Geometry::Attribute GammaD = 2; // Homogenous Dirichlet boundary
static constexpr Geometry::Attribute GammaN = 3; // Inhomogenous Neumann boundary

// Lamé coefficients
static constexpr Scalar mu    = 0.3846;
static constexpr Scalar lambda = 0.5769;

// Optimization parameters
static constexpr size_t maxIt = 40;
static constexpr Scalar eps  = 1e-6;
static constexpr Scalar hmax  = 0.1;
static constexpr Scalar ell  = 5.0;
static constexpr Scalar alpha = 4 * hmax;


// Compliance
inline Scalar compliance(const GridFunction<H1<Math::Vector, Context::Serial>>& w)
{
  auto& vh = w.getFiniteElementSpace();
  TrialFunction u(vh);
  TestFunction  v(vh);
  BilinearForm  bf(u, v);
  bf = Integral(lambda * Div(u), Div(v))
     + Integral(mu * (Jacobian(u) + Jacobian(u).T()), 0.5 * (Jacobian(v) + Jacobian(v).T()));
  return bf(w, w);
};

int main(int, char**)
{
  const char* meshFile = "../resources/mfem/simple-cantilever2d-example.mesh";

  // Load mesh
  MMG::Mesh Omega;
  Omega.load(meshFile);
  Omega.save("Omega0.mesh");
  Alert::Info() << "Saved initial mesh to Omega0.mesh" << Alert::Raise;

  // UMFPack
  Solver::UMFPack solver;

  // Optimization loop
  std::vector<double> obj;
  for (size_t i = 0; i < maxIt; i++)
  {
    Alert::Info() << "----- Iteration: " << i << Alert::Raise;

    // Finite element spaces
    int d = 2;
    H1 vh(Omega, d);

    // Pull-down force
    VectorFunction f{0, -1};

    // Elasticity equation
    TrialFunction u(vh);
    TestFunction  v(vh);
    Problem elasticity(u, v);
    elasticity = LinearElasticityIntegral(u, v)(lambda, mu)
               - BoundaryIntegral(f, v).over(GammaN)
               + DirichletBC(u, VectorFunction{0, 0}).on(GammaD);
    elasticity.solve(solver);

    // Hilbert extension-regularization procedure
    TrialFunction g(vh);
    TestFunction  w(vh);

    auto e = 0.5 * (Jacobian(u.getSolution()) + Jacobian(u.getSolution()).T());
    auto Ae = 2.0 * mu * e + lambda * Trace(e) * IdentityMatrix(d);

    Problem hilbert(g, w);
    hilbert = Integral(alpha * Jacobian(g), Jacobian(w))
            + Integral(g, w)
            - BoundaryIntegral(Dot(Ae, e) - ell, Dot(BoundaryNormal(Omega), w)).over(Gamma0)
            + DirichletBC(g, VectorFunction{0, 0}).on({GammaD, GammaN});
    hilbert.solve(solver);

    // Update objective
    obj.push_back(compliance(u.getSolution()) + ell * Omega.getVolume());

    Alert::Info() << "   | Objective: " << obj[i] << Alert::Raise;

    // Make the displacement
    Scalar dt = Omega.getMaximumDisplacement(g.getSolution());
    g.getSolution() *= hmax * dt;
    Omega.displace(g.getSolution());

    // Refine the mesh using MMG
    MMG::MeshOptimizer().setHMax(hmax).optimize(Omega);

    // Save mesh
    Omega.save("Omega.mesh");
  }

  Alert::Info() << "Saved final mesh to Omega.mesh" << Alert::Raise;

  return 0;
}
