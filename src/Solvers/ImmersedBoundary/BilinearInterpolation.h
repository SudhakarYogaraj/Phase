#ifndef BILINEAR_INTERPOLATION_H
#define BILINEAR_INTERPOLATION_H

#include "Matrix.h"
#include "FiniteVolumeGrid2D.h"

class BilinearInterpolation
{
public:

    BilinearInterpolation(const FiniteVolumeGrid2D& grid);
    BilinearInterpolation(const Point2D pts[]);

    Scalar operator()(const Scalar vals[], const Point2D& ip) const;
    std::vector<Scalar> operator()(const Point2D& ip) const;

private:

    void constructMatrix(const Point2D pts[]);

    Matrix mat_;

};

#endif