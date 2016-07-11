#ifndef CELL_H
#define CELL_H

#include <vector>

#include "Types.h"
#include "Polygon.h"

#include "Node.h"
#include "Face.h"
#include "BoundaryFaceLink.h"
#include "InteriorFaceLink.h"
#include "DiagonalCellLink.h"

class Cell
{
public:

    enum {INACTIVE = -1};

    Cell(const std::vector<size_t>& faceIds, std::vector<Face>& faces, bool isActive = true);

    bool isActive() const { return isActive_; }

    Scalar volume() const { return volume_; }
    const Point2D& centroid() const { return centroid_; }

    size_t id() const { return id_; }
    int globalIndex() const { return globalIndex_; }

    void addDiagonalLink(const Cell& cell);

    const std::vector< Ref<const Face> >& faces() const { return faces_; }
    const std::vector<size_t>& nodeIds() const { return nodeIds_; }
    const Polygon& shape() const { return cellShape_; }
    const std::vector<InteriorLink>& neighbours() const { return interiorLinks_; }
    const std::vector<BoundaryLink>& boundaries() const { return boundaryLinks_; }
    const std::vector<DiagonalCellLink>& diagonals() const { return diagonalLinks_; }

    size_t nFaces() const { return faces_.size(); }
    size_t nInteriorFaces() const { return interiorLinks_.size(); }
    size_t nBoundaryFaces() const { return boundaryLinks_.size(); }
    size_t nNeighbours() const { return interiorLinks_.size(); }

    bool isInCell(const Point2D& point) const;

private:

    void computeCellAdjacency();

    mutable bool isActive_;
    mutable size_t globalIndex_;

    Polygon cellShape_;

    Scalar volume_;
    Vector2D centroid_;

    size_t id_;

    std::vector< Ref<const Face> > faces_;
    std::vector<size_t> nodeIds_;

    std::vector<InteriorLink> interiorLinks_;
    std::vector<BoundaryLink> boundaryLinks_;
    std::vector<DiagonalCellLink> diagonalLinks_;

    friend class FiniteVolumeGrid2D;
};

#endif
