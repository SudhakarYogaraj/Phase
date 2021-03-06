#include <fstream>

#include "ImmersedBoundary.h"
#include "SurfaceTensionForce.h"
#include "StepImmersedBoundaryObject.h"
#include "QuadraticImmersedBoundaryObject.h"
#include "GhostCellImmersedBoundaryObject.h"
#include "HighOrderImmersedBoundaryObject.h"
#include "TranslatingMotion.h"
#include "OscillatingMotion.h"
#include "SolidBodyMotion.h"

ImmersedBoundary::ImmersedBoundary(const Input &input, Solver &solver)
        :
        solver_(solver),
        cellStatus_(solver.addIntegerField("cellStatus"))
{
    Label id = 0;

    auto ibInput = input.boundaryInput().get_child_optional("ImmersedBoundaries");

    if (ibInput)
        for (const auto &ibObjectInput: ibInput.get())
        {
            solver.grid().comm().printf("Initializing immersed boundary object \"%s\".\n", ibObjectInput.first.c_str());

            std::string method = ibObjectInput.second.get<std::string>("method", "ghost-cell");
            std::transform(method.begin(), method.end(), method.begin(), ::tolower);
            solver.grid().comm().printf("Immersed boundary method: %s\n", method.c_str());

            std::shared_ptr<ImmersedBoundaryObject> ibObject;

            if (method == "step")
                ibObject = std::make_shared<StepImmersedBoundaryObject>(ibObjectInput.first, id++, solver.grid());
            else if (method == "quadratic")
                ibObject = std::make_shared<QuadraticImmersedBoundaryObject>(ibObjectInput.first, id++, solver.grid());
            else if (method == "ghost-cell")
                ibObject = std::make_shared<GhostCellImmersedBoundaryObject>(ibObjectInput.first, id++, solver.grid());
            else if (method == "high-order")
                ibObject = std::make_shared<HighOrderImmersedBoundaryObject>(ibObjectInput.first, id++, solver.grid());
            else
                throw Exception("ImmersedBoundary", "ImmersedBoundary",
                                "invalid immersed boundary method \"" + method + "\".");

            //- Initialize the geometry
            std::string shape = ibObjectInput.second.get<std::string>("geometry.type");
            Point2D center = Point2D(ibObjectInput.second.get<std::string>("geometry.center"));

            if (shape == "circle")
            {
                ibObject->initCircle(center, ibObjectInput.second.get<Scalar>("geometry.radius"));
            }
            else if (shape == "box")
            {
                ibObject->initBox(center, ibObjectInput.second.get<Scalar>("geometry.width"),
                                  ibObjectInput.second.get<Scalar>("geometry.height"));
            }
            else if (shape == "polygon")
            {
                std::ifstream fin;
                std::vector<Point2D> verts;
                std::string filename = "case/";
                filename += ibObjectInput.second.get<std::string>("geometry.file").c_str();

                fin.open(filename.c_str());

                if (!fin.is_open())
                    throw Exception("ImmersedBoundary", "ImmersedBoundary",
                                    "failed to open file \"" + filename + "\".");

                solver.grid().comm().printf("Reading data for \"%s\" from file \"%s\".\n", ibObjectInput.first.c_str(),
                                            filename.c_str());

                while (!fin.eof())
                {
                    Scalar x, y;
                    fin >> x;
                    fin >> y;

                    verts.push_back(Point2D(x, y));
                }

                fin.close();

                Vector2D translation = center - Polygon(verts.begin(), verts.end()).centroid();

                for (Point2D &vert: verts)
                    vert += translation;

                ibObject->initPolygon(verts.begin(), verts.end());
            }
            else
                throw Exception("ImmersedBoundaryObject", "ImmersedBoundaryObject",
                                "invalid geometry type \"" + shape + "\".");

            //- Optional geometry parameters
            boost::optional<Scalar> scaleFactor = ibObjectInput.second.get_optional<Scalar>("geometry.scale");

            if (scaleFactor)
            {
                solver.grid().comm().printf("Scaling \"%s\" by a factor of %lf.\n", ibObjectInput.first.c_str(),
                                            scaleFactor.get());
                ibObject->shape().scale(scaleFactor.get());
            }

            boost::optional<Scalar> rotationAngle = ibObjectInput.second.get_optional<Scalar>("geometry.rotate");

            if (rotationAngle)
            {
                solver.grid().comm().printf("Rotating \"%s\" by an angle of %lf degrees.\n",
                                            ibObjectInput.first.c_str(),
                                            rotationAngle.get());

                if (ibObject->shape().type() == Shape2D::BOX)
                {
                    Box *box = (Box *) &ibObject->shape();
                    auto verts = box->vertices();

                    ibObject->initPolygon(verts.begin(), verts.end());
                }

                ibObject->shape().rotate(rotationAngle.get() * M_PI / 180.);
            }

            //- Properties
            ibObject->rho = ibObjectInput.second.get<Scalar>("properties.rho", 0.);

            //- Boundary information
            for (const auto &child: ibObjectInput.second)
            {
                if (child.first == "geometry" || child.first == "interpolation"
                    || child.first == "motion" || child.first == "method" || child.first == "properties")
                    continue;

                std::string type = child.second.get<std::string>("type");
                ImmersedBoundaryObject::BoundaryType boundaryType;

                if (type == "fixed")
                {
                    boundaryType = ImmersedBoundaryObject::FIXED;
                    ibObject->addBoundaryRefValue(child.first, child.second.get<std::string>("value"));
                }
                else if (type == "normal_gradient")
                {
                    boundaryType = ImmersedBoundaryObject::NORMAL_GRADIENT;
                    ibObject->addBoundaryRefValue(child.first, child.second.get<std::string>("value"));
                }
                else if (type == "partial_slip")
                {
                    boundaryType = ImmersedBoundaryObject::PARTIAL_SLIP;
                    ibObject->addBoundaryRefValue(child.first, child.second.get<std::string>("value"));
                }
                else
                    throw Exception("ImmersedBoundary", "ImmersedBoundary",
                                    "unrecognized boundary type \"" + type + "\".");

                solver.grid().comm().printf("Setting boundary type \"%s\" for field \"%s\".\n", type.c_str(),
                                            child.first.c_str());
                ibObject->addBoundaryType(child.first, boundaryType);
            }

            //- Set the motion type
            std::shared_ptr<Motion> motion = nullptr;
            std::string motionType = ibObjectInput.second.get<std::string>("motion.type", "none");

            if (motionType == "translating")
            {
                motion = std::make_shared<TranslatingMotion>(
                        ibObject,
                        ibObjectInput.second.get<std::string>("motion.velocity"),
                        ibObjectInput.second.get<std::string>("motion.acceleration", "(0,0)")
                );
            }
            else if (motionType == "oscillating")
            {
                motion = std::make_shared<OscillatingMotion>(
                        ibObject,
                        ibObjectInput.second.get<std::string>("motion.frequency"),
                        ibObjectInput.second.get<std::string>("motion.amplitude"),
                        ibObjectInput.second.get<std::string>("motion.phase", "(0,0)"),
                        0
                );
            }
            else if (motionType == "solidBody")
            {
                motion = std::make_shared<SolidBodyMotion>(
                        ibObject,
                        ibObjectInput.second.get<std::string>("motion.velocity", "(0,0)")
                );
            }
            else if (motionType == "none")
            {
                motion = nullptr;
            }
            else
                throw Exception("ImmersedBoundary", "ImmersedBoundary", "invalid motion type \"" + motionType + "\".");

            ibObject->setMotion(motion);

            ibObjs_.push_back(ibObject);
        }

    auto ibArrayInput = input.boundaryInput().get_child_optional("ImmersedBoundaryArray");

    if (ibArrayInput)
    {
        int shapeI = ibArrayInput.get().get<int>("shapeI");
        int shapeJ = ibArrayInput.get().get<int>("shapeJ");
        Point2D anchor = ibArrayInput.get().get<std::string>("anchor");
        Vector2D spacing = ibArrayInput.get().get<std::string>("spacing");
        std::string name = ibArrayInput.get().get<std::string>("Boundary.name");
        std::string method = ibArrayInput.get().get<std::string>("Boundary.method");
        std::string shape = ibArrayInput.get().get<std::string>("Boundary.Geometry.type");
        std::string motionType = ibArrayInput.get().get<std::string>("Boundary.Motion.type");
        Scalar rho = ibArrayInput.get().get<Scalar>("Boundary.Properties.rho", 0.);

        std::shared_ptr<ImmersedBoundaryObject> ibObj;
        std::shared_ptr<Motion> motion;

        for (int j = 0; j < shapeJ; ++j)
            for (int i = 0; i < shapeI; ++i)
            {
                Point2D center = anchor + Vector2D(spacing.x * i, spacing.y * j);
                std::string ibObjName = name + "_" + std::to_string(i) + "_" + std::to_string(j);

                if (method == "step")
                    ibObj = std::make_shared<StepImmersedBoundaryObject>(ibObjName, id++, solver.grid());
                else if (method == "ghost-cell")
                    ibObj = std::make_shared<GhostCellImmersedBoundaryObject>(ibObjName, id++, solver.grid());
                else if (method == "quadratic")
                    ibObj = std::make_shared<QuadraticImmersedBoundaryObject>(ibObjName, id++, solver.grid());
                if (shape == "circle")
                {
                    ibObj->initCircle(
                            center,
                            ibArrayInput.get().get<Scalar>("Boundary.Geometry.radius")
                    );
                }

                ibObj->rho = rho;

                for (const auto &fieldInput: ibArrayInput.get().get_child("Boundary.Fields"))
                {
                    ibObj->addBoundaryType(
                            fieldInput.first,
                            fieldInput.second.get<std::string>("type")
                    );
                }

                if (motionType == "solidBody")
                    motion = std::make_shared<SolidBodyMotion>(ibObj);
                else
                    motion = nullptr;

                ibObj->setMotion(motion);
                ibObjs_.push_back(ibObj);
            }
    }

    //- Collision model
    collisionModel_ = std::make_shared<CollisionModel>(
            input.boundaryInput().get<Scalar>("ImmersedBoundaries.Collisions.stiffness", 1e-4),
            input.boundaryInput().get<Scalar>("ImmersedBoundaries.Collisions.range", 0.)
    );

    if (ibObjs_.empty())
        solver_.grid().comm().printf("No immersed boundaries present.\n");

    for(const Node& node: grid().nodes())
    {
        if(!ibObj(node))
            fluidNodes_.add(node);
    }
}

const Solver &ImmersedBoundary::solver() const
{
    return solver_;
}

const FiniteVolumeGrid2D &ImmersedBoundary::grid() const
{
    return solver_.grid();
}

void ImmersedBoundary::initCellZones(CellZone &zone)
{
    zone_ = &zone;

    for (auto &ibObj: ibObjs_)
    {
        ibObj->setZone(zone);
        ibObj->updateCells();
    }

    setCellStatus();
    solver_.grid().computeGlobalOrdering();
}

CellGroup ImmersedBoundary::ibCells() const
{
    CellGroup ibCellGroup;

    for (const auto &ibObj: ibObjs_)
        ibCellGroup += ibObj->ibCells();

    return ibCellGroup;
}

CellGroup ImmersedBoundary::solidCells() const
{
    CellGroup solidCellGroup;

    for (const auto &ibObj: ibObjs_)
        solidCellGroup += ibObj->solidCells();

    return solidCellGroup;
}

std::shared_ptr<const ImmersedBoundaryObject> ImmersedBoundary::ibObj(const Point2D &pt) const
{
    for (const auto &ibObj: ibObjs_)
        if (ibObj->isInIb(pt))
            return ibObj;

    return nullptr;
}

std::shared_ptr<const ImmersedBoundaryObject> ImmersedBoundary::nearestIbObj(const Point2D& pt) const
{
    return nearestIntersect(pt).first;
}

std::pair<std::shared_ptr<const ImmersedBoundaryObject>, Point2D> ImmersedBoundary::nearestIntersect(const Point2D &pt) const
{
    std::shared_ptr<const ImmersedBoundaryObject> nearestIbObj = nullptr;
    Point2D minXc;
    Scalar minDistSqr = std::numeric_limits<Scalar>::infinity();

    for(const auto& ibObj: *this)
    {
        Point2D xc = ibObj->nearestIntersect(pt);
        Scalar distSqr = (xc - pt).magSqr();

        if(distSqr < minDistSqr)
        {
            nearestIbObj = ibObj;
            minXc = xc;
            minDistSqr = distSqr;
        }
    }

    return std::make_pair(nearestIbObj, minXc);
}

const ImmersedBoundaryObject &ImmersedBoundary::ibObj(const std::string &name) const
{
    for (const auto &ibObj: ibObjs_)
        if (ibObj->name() == name)
            return *ibObj;

    throw Exception("ImmersedBoundary", "ibObj", "no immersed boundary object named \"" + name + "\".");
}

std::vector<Ref<const ImmersedBoundaryObject> > ImmersedBoundary::ibObjs() const
{
    std::vector<Ref<const ImmersedBoundaryObject>> refs;

    std::transform(ibObjs_.begin(), ibObjs_.end(), std::back_inserter(refs),
                   [](const std::shared_ptr<ImmersedBoundaryObject> &ptr) { return std::cref(*ptr); });

    return refs;
}

void ImmersedBoundary::update(Scalar timeStep)
{
    for (auto &ibObj: ibObjs_)
        ibObj->update(timeStep);

    setCellStatus();
    solver_.grid().computeGlobalOrdering();

    for(const Node& node: grid().nodes())
    {
        if(!ibObj(node))
            fluidNodes_.add(node);
    }
}

Equation<Vector2D> ImmersedBoundary::velocityBcs(VectorFiniteVolumeField &u) const
{
    Equation<Vector2D> eqn(u);

    for (const auto &ibObj: ibObjs_)
        eqn += ibObj->velocityBcs(u);

    return eqn;
}

Equation<Scalar> ImmersedBoundary::pressureBcs(Scalar rho, ScalarFiniteVolumeField &p) const
{
    Equation<Scalar> eqn(p);

    for (const auto &ibObj: ibObjs_)
        eqn += ibObj->pressureBcs(rho, p);

    return eqn;
}

Equation<Scalar> ImmersedBoundary::contactLineBcs(const SurfaceTensionForce &fst, ScalarFiniteVolumeField &gamma) const
{
    Equation<Scalar> eqn(gamma);

    for (const auto &ibObj: ibObjs_)
        eqn += ibObj->contactLineBcs(gamma, fst.getTheta(*ibObj));

    return eqn;
}

void ImmersedBoundary::clearFreshCells()
{
    for (auto &ibObj: ibObjs_)
        ibObj->clearFreshCells();
}

bool ImmersedBoundary::isIbCell(const Cell &cell) const
{
    for (const auto &ibObj: ibObjs_)
    {
        if (ibObj->isInIb(cell.centroid()))
            return true;
    }

    return false;
}

void ImmersedBoundary::computeForce(Scalar rho,
                                    Scalar mu,
                                    const VectorFiniteVolumeField &u,
                                    const ScalarFiniteVolumeField &p,
                                    const Vector2D &g)
{
    for (auto ibObj: ibObjs_)
        ibObj->computeForce(rho, mu, u, p, g);

    if (collisionModel_)
        for (auto ibObjP: ibObjs_)
        {
            for (auto ibObjQ: ibObjs_)
                ibObjP->addForce(collisionModel_->force(*ibObjP, *ibObjQ));

            ibObjP->addForce(collisionModel_->force(*ibObjP, ibObjP->grid()));
        }
}

void ImmersedBoundary::computeForce(const ScalarFiniteVolumeField &rho,
                                    const ScalarFiniteVolumeField &mu,
                                    const VectorFiniteVolumeField &u,
                                    const ScalarFiniteVolumeField &p,
                                    const Vector2D &g)
{
    for (auto ibObj: ibObjs_)
        ibObj->computeForce(rho, mu, u, p, g);

    if (collisionModel_)
        for (auto ibObjP: ibObjs_)
        {
            for (auto ibObjQ: ibObjs_)
                ibObjP->addForce(collisionModel_->force(*ibObjP, *ibObjQ));

            ibObjP->addForce(collisionModel_->force(*ibObjP, ibObjP->grid()));
        }
}

//- Protected

void ImmersedBoundary::setCellStatus()
{
    cellStatus_.fill(0);

    for (const Cell &cell: solver_.grid().cellZone("fluid"))
        cellStatus_(cell) = FLUID_CELLS;

    for (const CellZone &bufferZone: solver_.grid().bufferZones())
        for (const Cell &cell: bufferZone)
            cellStatus_(cell) = BUFFER_CELLS;

    for (const auto &ibObj: ibObjs_)
    {
        for (const Cell &cell: ibObj->ibCells())
            cellStatus_(cell) = IB_CELLS;

        for (const Cell &cell: ibObj->solidCells())
            cellStatus_(cell) = SOLID_CELLS;

        for (const Cell &cell: ibObj->freshCells())
            cellStatus_(cell) = FRESH_CELLS;

        for (const Cell &cell: ibObj->deadCells())
            cellStatus_(cell) = DEAD_CELLS;
    }
}
