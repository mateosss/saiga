﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/opengl/shader/shader.h"
#include "saiga/opengl/shader/shaderPartLoader.h"
#include "saiga/core/util/ObjectCache.h"
#include "saiga/core/util/assert.h"
#include "saiga/core/util/fileChecker.h"
#include "saiga/core/util/singleton.h"

namespace Saiga
{
class SAIGA_OPENGL_API ShaderLoader : public Singleton<ShaderLoader>
{
    friend class Singleton<ShaderLoader>;

    ObjectCache<std::string, std::shared_ptr<Shader>, ShaderPart::ShaderCodeInjections> cache;

   public:
    virtual ~ShaderLoader() {}
    template <typename shader_t>
    std::shared_ptr<shader_t> load(const std::string& name,
                                   const ShaderPart::ShaderCodeInjections& sci = ShaderPart::ShaderCodeInjections());

    void reload();
    bool reload(std::shared_ptr<Shader> shader, const std::string& name, const ShaderPart::ShaderCodeInjections& sci);
};


template <typename shader_t>
std::shared_ptr<shader_t> ShaderLoader::load(const std::string& name, const ShaderPart::ShaderCodeInjections& sci)
{
    std::string fullName = SearchPathes::shader(name);
    if (fullName.empty())
    {
        cout << "Could not find file '" << name << "'. Make sure it exists and the search pathes are set." << endl;
        cerr << SearchPathes::shader << endl;
        SAIGA_ASSERT(0);
    }


    std::shared_ptr<Shader> objectBase;
    auto inCache = cache.get(fullName, objectBase, sci);


    std::shared_ptr<shader_t> object;
    if (inCache)
    {
        object = std::dynamic_pointer_cast<shader_t>(objectBase);
    }
    else
    {
        ShaderPartLoader spl(fullName, sci);
        if (spl.load())
        {
            object = spl.createShader<shader_t>();
        }

        cache.put(fullName, object, sci);
    }
    SAIGA_ASSERT(object);

    return object;
}



}  // namespace Saiga
