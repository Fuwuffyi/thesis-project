#pragma once

#include "../../Transform.hpp"
#include "Component.hpp"

class TransformComponent : public Component {
public:
   Transform m_transform;
};

