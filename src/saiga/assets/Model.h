/**
 * Copyright (c) 2017 Darius Rückert 
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include <saiga/geometry/triangle_mesh.h>


namespace Saiga {

template<typename vertex_t, typename index_t>
class SAIGA_GLOBAL Model
{
public:
    using VertexType = vertex_t;
    using IndexType = index_t;

    AABB boundingBox;
    vec3 offset = vec3(0);
    TriangleMesh<vertex_t,index_t> mesh;

    void normalizePosition();

    void normalizeScale();
    /**
     * Transforms the vertices and normals that the up axis is Y when before the up axis was Z.
     *
     * Many 3D CAD softwares (i.e. Blender) are using a right handed coordinate system with Z pointing upwards.
     * This frameworks uses a right haned system with Y pointing upwards.
     */


    void ZUPtoYUP();
};

template<typename vertex_t, typename index_t>
void Model<vertex_t,index_t>::normalizePosition()
{
    offset = boundingBox.getPosition();
    mat4 t = glm::translate(mat4(1),-offset);
    mesh.transform(t);
    boundingBox.setPosition(vec3(0));
}


template<typename vertex_t, typename index_t>
void Model<vertex_t,index_t>::normalizeScale()
{
    //TODO
    vec3 d = boundingBox.max - boundingBox.min;
    mat4 t = glm::translate(mat4(1),-offset);
    mesh.transform(t);
    boundingBox.setPosition(vec3(0));
}



template<typename vertex_t, typename index_t>
void Model<vertex_t,index_t>::ZUPtoYUP()
{
    const mat4 m(
                1, 0, 0, 0,
                0, 0, -1, 0,
                0, 1, 0, 0,
                0, 0, 0, 1
                );
    mesh.transform(m);
    mesh.transformNormal(m);
}



}
