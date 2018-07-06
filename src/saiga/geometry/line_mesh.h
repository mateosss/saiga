﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/util/glm.h"

#include "saiga/geometry/vertex.h"
#include "saiga/geometry/aabb.h"
#include "saiga/geometry/triangle.h"

#include "saiga/util/assert.h"


namespace Saiga {


template<typename VertexType, typename IndexType = uint32_t>
class LineMesh
{
public:

    std::vector<VertexType> toLineList();

public:
    std::vector<VertexType> vertices;
    std::vector<IndexType> indices;
};


template<typename VertexType, typename IndexType>
std::vector<VertexType> LineMesh<VertexType,IndexType>::toLineList()
{
    SAIGA_ASSERT(indices.size() % 2 == 0);
    std::vector<VertexType> res(indices.size());
    for(unsigned int i = 0; i < indices.size(); i++)
    {
        res[i] = vertices[indices[i]];
    }
    return res;
}


}
