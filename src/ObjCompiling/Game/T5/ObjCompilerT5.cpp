#include "ObjCompilerT5.h"

#include "Game/T5/T5.h"
#include "Image/ImageIwdPostProcessor.h"

#include <memory>

using namespace T5;
namespace fs = std::filesystem;

namespace
{
    void ConfigureCompilers(AssetCreatorCollection& collection, Zone& zone, ISearchPath& searchPath)
    {
        auto& memory = *zone.GetMemory();

        // No compilers yet
    }

    void ConfigurePostProcessors(
        AssetCreatorCollection& collection, Zone& zone, const ZoneDefinitionContext& zoneDefinition, ISearchPath& searchPath, const fs::path& outDir)
    {
        auto& memory = *zone.GetMemory();

        if (ImageIwdPostProcessor<AssetImage>::AppliesToZoneDefinition(zoneDefinition))
            collection.AddAssetPostProcessor(std::make_unique<ImageIwdPostProcessor<AssetImage>>(zoneDefinition, searchPath, outDir));
    }
} // namespace

void ObjCompiler::ConfigureCreatorCollection(AssetCreatorCollection& collection,
                                             Zone& zone,
                                             const ZoneDefinitionContext& zoneDefinition,
                                             ISearchPath& searchPath,
                                             IGdtQueryable& gdt,
                                             const fs::path& outDir,
                                             const fs::path& cacheDir) const
{
    ConfigurePostProcessors(collection, zone, zoneDefinition, searchPath, outDir);
}
