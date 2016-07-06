#ifndef CONTINUUM_SURFACE_FORCE_H
#define CONTINUUM_SURFACE_FORCE_H

#include "SurfaceTensionForce.h"

class ContinuumSurfaceForce : public SurfaceTensionForce
{
public:

    ContinuumSurfaceForce(const Input& input,
                          const ScalarFiniteVolumeField& gamma,
                          const VectorFiniteVolumeField &u,
                          std::map<std::string, ScalarFiniteVolumeField>& scalarFields,
                          std::map<std::string, VectorFiniteVolumeField>& vectorFields);

    virtual VectorFiniteVolumeField compute();
    const VectorFiniteVolumeField& n() const { return n_; }

    virtual const ScalarFiniteVolumeField& gammaTilde() const { return gammaTilde_; }
    virtual const VectorFiniteVolumeField& gradGammaTilde() const { return gradGammaTilde_; }

    void constructSmoothingKernels();

protected:

    virtual void computeGradGammaTilde();
    virtual void computeInterfaceNormals();
    virtual void computeCurvature();

    std::vector< std::vector< Ref<const Cell> > > cellRangeSearch_;
    Scalar kernelWidth_;

    ScalarFiniteVolumeField &gammaTilde_;
    VectorFiniteVolumeField gradGammaTilde_;
};

#endif
