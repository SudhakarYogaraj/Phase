#ifndef CELL_H
#define CELL_H

#include <vector>

#include "Types.h"
#include "Polygon.h"

#include "Node.h"
#include "Face.h"
#include "BoundaryLink.h"
#include "InteriorLink.h"

class FiniteVolumeGrid2D;

class Cell
{
public:

    Cell(const std::vector<Label> &nodeIds, const FiniteVolumeGrid2D &grid);

    //- Geometry
    Scalar volume() const
    { return volume_; }

    Scalar polarVolume() const;

    const Point2D &centroid() const
    { return centroid_; }

    //- Indices
    Index &index(Size indexNo)
    { return indices_[indexNo]; }

    Index index(Size indexNo) const
    { return indices_[indexNo]; }

    void setNumIndices(Size num)
    { indices_.resize(num, -1); }

    void clearIndices()
    { indices_.clear(); }

    //- Ids
    Label id() const
    { return id_; }

    //- Connectivity links, should really only be done by grid classes
    void addDiagonalLink(const Cell &cell);

    void addBoundaryLink(const Face &face);

    void addInteriorLink(const Face &face, const Cell &cell);

    std::vector<InteriorLink> &neighbours()
    { return interiorLinks_; }

    std::vector<CellLink> &diagonals()
    { return diagonalLinks_; }

    std::vector<BoundaryLink> &boundaries()
    { return boundaryLinks_; }

    const std::vector<InteriorLink> &neighbours() const
    { return interiorLinks_; }

    const std::vector<CellLink> &diagonals() const
    { return diagonalLinks_; }

    const std::vector<BoundaryLink> &boundaries() const

    { return boundaryLinks_; }

    std::vector<Ref<const CellLink>> cellLinks() const;

    template<class UnaryPredicate>
    std::vector<Ref<const CellLink>> cellLinks(UnaryPredicate p)
    {
        auto result = cellLinks();
        result.erase(std::remove_if(result.begin(), result.end(), p), result.end());
        return result;
    }

    const Cell& faceNeighbour(const Node& lNode, const Node& rNode) const;

    //- Nodes
    const std::vector<Ref<const Node> > nodes() const;

    const Polygon &shape() const
    { return cellShape_; }

    Size nFaces() const
    { return cellShape_.vertices().size() - 1; }

    Size nInteriorFaces() const
    { return interiorLinks_.size(); }

    Size nBoundaryFaces() const
    { return boundaryLinks_.size(); }

    Size nNeighbours() const
    { return interiorLinks_.size(); }

    bool isInCell(const Point2D &point) const;

private:

    std::vector<Index> indices_; // Indices for linear algebra. May change depending on problem
    Label id_; // Indices for identification. Should not normally be changed

    Polygon cellShape_;

    Scalar volume_;
    Vector2D centroid_;

    std::vector<Label> nodeIds_;
    const std::vector<Node> &nodes_;

    std::vector<InteriorLink> interiorLinks_;
    std::vector<CellLink> diagonalLinks_;
    std::vector<BoundaryLink> boundaryLinks_;
};

bool cellsShareFace(const Cell &cellA, const Cell &cellB);

#endif
