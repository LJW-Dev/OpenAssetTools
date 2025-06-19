#pragma once

class IContentLoadingEntryPoint
{
public:
    IContentLoadingEntryPoint() = default;
    virtual ~IContentLoadingEntryPoint() = default;
    IContentLoadingEntryPoint(const IContentLoadingEntryPoint& other) = default;
    IContentLoadingEntryPoint(IContentLoadingEntryPoint&& other) noexcept = default;
    IContentLoadingEntryPoint& operator=(const IContentLoadingEntryPoint& other) = default;
    IContentLoadingEntryPoint& operator=(IContentLoadingEntryPoint&& other) noexcept = default;

    virtual void Load() = 0;
};
