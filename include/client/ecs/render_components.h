#ifndef RENDER_COMPONENTS_H
#define RENDER_COMPONENTS_H

#include "model.h"
#include <memory>

namespace Components {

struct RenderMesh {
    std::shared_ptr<VulkanHelpers::Model> model;
};

} // namespace Components

#endif // RENDER_COMPONENTS_H
