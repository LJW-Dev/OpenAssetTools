#pragma once

#include "SearchPath/IOutputPath.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

class MockOutputFile
{
public:
    std::string m_name;
    std::vector<std::uint8_t> m_data;

    MockOutputFile();
    MockOutputFile(std::string name, std::vector<std::uint8_t> data);

    [[nodiscard]] std::string AsString() const;
};

class MockOutputPath final : public IOutputPath
{
public:
    std::unique_ptr<std::ostream> Open(const std::string& fileName) override;

    [[nodiscard]] const MockOutputFile* GetMockedFile(const std::string& name) const;
    [[nodiscard]] const std::vector<MockOutputFile>& GetMockedFileList() const;

private:
    std::vector<MockOutputFile> m_files;
};
