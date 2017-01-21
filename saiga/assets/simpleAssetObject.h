#pragma once

#include "saiga/config.h"
#include "saiga/rendering/object3d.h"
#include <memory>

class Asset;
class Camera;

class SAIGA_GLOBAL SimpleAssetObject : public Object3D{
public:
    std::shared_ptr<Asset> asset;

    void render(Camera *cam);
    void renderDepth(Camera *cam);
    void renderWireframe(Camera *cam);
    void renderRaw();
};
