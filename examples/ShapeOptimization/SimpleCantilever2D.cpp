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

using FES = VectorP1<Mesh<Context::Local>>;

// Define boundary attributes
static constexpr Geometry::Attribute Gamma0 = 1; // Traction free boundary
static constexpr Geometry::Attribute GammaD = 2; // Homogenous Dirichlet boundary
static constexpr Geometry::Attribute GammaN = 3; // Inhomogenous Neumann boundary

// Lamé coefficients
static constexpr Real mu    = 0.3846;
static constexpr Real lambda = 0.5769;

// Optimization parameters
static constexpr size_t maxIt = 50;
static constexpr Real hmax  = 0.1;
static constexpr Real hmin  = 0.1 * hmax;
static constexpr Real ell  = 5.0;
static constexpr Real alpha = 4 * hmax;


// Compliance
inline Real compliance(const GridFunction<FES>& w)
{
  auto& vh = w.getFiniteElementSpace();
  TrialFunction u(vh);
  TestFunction  v(vh);
  BilinearForm  bf(u, v);
  bf = LinearElasticityIntegral(u, v)(lambda, mu);
  return bf(w, w);
};

int main(int, char**)
{
  const char* meshFile = "../resources/examples/ShapeOptimization/SimpleCantilever2D.mfem.mesh";

  // Load mesh
  MMG::Mesh Omega;
  Omega.load(meshFile);
  Omega.save("Omega0.mesh");

  Alert::Info() << "Saved initial mesh to Omega0.mesh" << Alert::Raise;

  // Optimization loop
  for (size_t i = 0; i < maxIt; i++)
  {
    Omega.getConnectivity().compute(1, 2);
    Alert::Info() << "----- Iteration: " << i << Alert::Raise;

    // Finite element spaces
    int d = Omega.getSpaceDimension();
    P1 sh(Omega);
    P1 vh(Omega, d);

    // Pull-down force
    VectorFunction f{0, -1};

    // Elasticity equation
    TrialFunction u(vh);
    TestFunction  v(vh);
    Problem elasticity(u, v);
    elasticity = LinearElasticityIntegral(u, v)(lambda, mu)
               - BoundaryIntegral(f, v).over(GammaN)
               + DirichletBC(u, VectorFunction{0, 0}).on(GammaD);
    Solver::SparseLU(elasticity).solve();

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
    Solver::SparseLU(hilbert).solve();


    g.getSolution().save("Theta.gf");
    Omega.save("Theta.mesh");

    const auto& dJ = g.getSolution();

    // Update objective
    const Real objective = compliance(u.getSolution()) + ell * Omega.getArea();
    Alert::Info() << "   | Objective: " << objective << Alert::Raise;

    // Make the displacement
    GridFunction norm(sh);
    norm = Frobenius(dJ);
    g.getSolution() *= hmin / norm.max();
    Omega.displace(g.getSolution());

    // Refine the mesh using MMG
    MMG::Optimizer().setHMax(hmax).setHMin(hmin).optimize(Omega);

    // Save mesh
    Omega.save("Omega.mesh");
  }

  Alert::Info() << "Saved final mesh to Omega.mesh" << Alert::Raise;

  return 0;
}
