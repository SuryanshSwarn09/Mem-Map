#pragma once

#include <QString>
#include <cstdint>
#include <vector>

struct DuplicateFile {
    QString path;
    int64_t size{0};
    int64_t lastModified{0};
    bool isSelectedForDeletion{false};
};
