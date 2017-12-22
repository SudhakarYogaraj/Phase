#include "Patch.h"

Patch::Patch(const std::string &name, Label id, const std::shared_ptr<PatchRegistry> &registry)
    :
      Group(name)
{
    id_ = id;
    registry_ = registry ? registry: std::make_shared<PatchRegistry>();
}

void Patch::add(const Face &face)
{
    auto insertion = registry_->insert(std::make_pair(face.id(), std::ref(*this)));

    if(insertion.second)
        FaceGroup::add(face);
    else
    {
        insertion.first->second.get().remove(face);
        add(face);
    }
}

void Patch::add(const FaceGroup& group)
{
    items_.reserve(size() + group.size());
    itemSet_.reserve(size() + group.size());

    for(const Face& item: std::vector<Ref<const Face>>(group.items()))
        add(item);
}

void Patch::remove(const Face &face)
{
    if(isInGroup(face))
    {
        registry_->erase(face.id());
        FaceGroup::remove(face);
    }
}

void Patch::remove(const FaceGroup& faces)
{
    for(const Face& face: faces)
        if(isInGroup(face))
            registry_->erase(face.id());

    FaceGroup::remove(faces);
}

void Patch::clear()
{
    for(const Face& face: *this)
        registry_->erase(face.id());

    FaceGroup::clear();
}

void Patch::setRegistry(std::shared_ptr<Patch::PatchRegistry> &registry)
{
    std::vector<Ref<const Face>> items = this->items();
    clear();
    registry_ = registry;
    FaceGroup::add(items.begin(), items.end());
}
