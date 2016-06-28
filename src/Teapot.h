#pragma once
/**
\file minimalOpenGL/teapot.h
\author Morgan McGuire, http://graphics.cs.williams.edu

Geometric data for the Utah teapot.

Distributed with the G3D Innovation Engine http://g3d.cs.williams.edu
*/

#include "Render.h"

namespace aa
{
    namespace teapot
    {
        void Draw(glm::mat4 m, glm::mat4 v, glm::mat4 p);
    }
}
