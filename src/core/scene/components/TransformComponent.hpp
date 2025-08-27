#pragma once

#include "core/scene/components/Component.hpp"

#include "core/Transform.hpp"

class TransformComponent : public Component {
public:
   Transform m_transform;
};

