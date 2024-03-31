#pragma once

#include "Utils/FileUtils.h"

#include <cstdint>

namespace gltf
{
    constexpr uint32_t GLTF_MAGIC = FileUtils::MakeMagic32('g', 'l', 'T', 'F');
    constexpr uint32_t GLTF_VERSION = 2u;

    constexpr uint32_t CHUNK_MAGIC_JSON = FileUtils::MakeMagic32('J', 'S', 'O', 'N');
    constexpr uint32_t CHUNK_MAGIC_BIN = FileUtils::MakeMagic32('B', 'I', 'N', '\x00');

    constexpr auto GLTF_LENGTH_OFFSET = 8u;
    constexpr auto GLTF_JSON_CHUNK_LENGTH_OFFSET = 12u;
    constexpr auto GLTF_JSON_CHUNK_DATA_OFFSET = 20u;

    constexpr auto GLTF_DATA_URI_PREFIX = "data:application/octet-stream;base64,";
} // namespace gltf
