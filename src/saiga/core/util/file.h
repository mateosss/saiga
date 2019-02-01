﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/config.h"

#include <saiga/core/util/ArrayView.h>
#include <vector>

namespace Saiga
{
namespace File
{
SAIGA_CORE_API std::vector<unsigned char> loadFileBinary(const std::string& file);

SAIGA_CORE_API std::string loadFileString(const std::string& file);

// returns an array of lines
SAIGA_CORE_API std::vector<std::string> loadFileStringArray(const std::string& file);


SAIGA_CORE_API void saveFileBinary(const std::string& file, const void* data, size_t size);
}  // namespace File
}  // namespace Saiga
