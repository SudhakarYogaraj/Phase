#ifndef PLIC_H
#define PLIC_H

#include "Equation.h"

namespace plic
{

Equation<ScalarFiniteVolumeField> div(const VectorFiniteVolumeField &u, ScalarFiniteVolumeField &field, Scalar timeStep,
                                      std::vector<Polygon>& plicPolygons,
                                      std::vector<Polygon>& fluxPolygons);

Polygon computeInterfacePolygon(const Cell& cell, Scalar gamma, const Vector2D& normal);
Polygon computeFluxPolygon(const BoundaryLink &link, const Vector2D& uf, Scalar timeStep);
}

#endif