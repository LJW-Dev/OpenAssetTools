#pragma once

#include "Asset/IAssetCreator.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"
#include "Zone/Definition/ZoneDefinition.h"

#include <memory>

namespace T6
{
    namespace BSP
    {
        std::unique_ptr<IAssetCreator> CreateLoaderT6(MemoryManager& memory, ISearchPath& searchPath, Zone& zone, ZoneDefinitionMapType mapType);
    } // namespace BSP
} // namespace T6
