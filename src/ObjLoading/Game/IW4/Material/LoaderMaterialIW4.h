#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW4/IW4.h"
#include "Gdt/IGdtQueryable.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace IW4
{
    std::unique_ptr<AssetCreator<AssetMaterial>> CreateMaterialLoader(MemoryManager& memory, ISearchPath& searchPath);
} // namespace IW4
