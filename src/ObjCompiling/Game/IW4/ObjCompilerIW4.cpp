#include "ObjCompilerIW4.h"

#include "Game/IW4/IW4.h"
#include "Image/ImageIwdPostProcessor.h"

#include <memory>

using namespace IW4;

namespace
{
    void ConfigureCompilers(AssetCreatorCollection& collection, Zone& zone, ISearchPath& searchPath)
    {
        auto& memory = zone.Memory();

        // No compilers yet
    }

    void ConfigurePostProcessors(AssetCreatorCollection& collection,
                                 Zone& zone,
                                 const ZoneDefinitionContext& zoneDefinition,
                                 ISearchPath& searchPath,
                                 ZoneAssetCreationStateContainer& zoneStates,
                                 IOutputPath& outDir)
    {
        auto& memory = zone.Memory();

        if (ImageIwdPostProcessor<AssetImage>::AppliesToZoneDefinition(zoneDefinition))
            collection.AddAssetPostProcessor(std::make_unique<ImageIwdPostProcessor<AssetImage>>(zoneDefinition, searchPath, zoneStates, outDir));
    }
} // namespace

void ObjCompiler::ConfigureCreatorCollection(AssetCreatorCollection& collection,
                                             Zone& zone,
                                             const ZoneDefinitionContext& zoneDefinition,
                                             ISearchPath& searchPath,
                                             IGdtQueryable& gdt,
                                             ZoneAssetCreationStateContainer& zoneStates,
                                             IOutputPath& outDir,
                                             IOutputPath& cacheDir) const
{
    ConfigurePostProcessors(collection, zone, zoneDefinition, searchPath, zoneStates, outDir);
}
