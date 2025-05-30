#pragma once
#include <cstdint>
#include <string>

class XBlock
{
public:
    enum class Type
    {
        BLOCK_TYPE_TEMP,
        BLOCK_TYPE_RUNTIME,
        BLOCK_TYPE_DELAY,
        BLOCK_TYPE_NORMAL
    };

    std::string m_name;
    unsigned m_index;
    Type m_type;

    uint8_t* m_buffer;
    size_t m_buffer_size;

    XBlock(const std::string& name, int index, Type type);
    ~XBlock();

    void Alloc(size_t blockSize);
};
