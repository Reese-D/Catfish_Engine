#ifndef RENDER_COMPONENTS_H
#define RENDER_COMPONENTS_H

#include <memory>
#include "model.h"

namespace Components {

struct RenderMesh {
    std::shared_ptr<VulkanHelpers::Model> model;
};

} // namespace Components

#endif // RENDER_COMPONENTS_H
