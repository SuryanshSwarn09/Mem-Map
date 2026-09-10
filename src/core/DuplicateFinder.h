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

struct DuplicateGroup {
    QString hash;
    int64_t fileSize{0};
    std::vector<DuplicateFile> files;

    int64_t wastedBytes() const {
        return files.size() > 1 ? (static_cast<int64_t>(files.size()) - 1) * fileSize : 0;
    }
};

enum class HashAlgorithm {
    Md5,
    Sha256
};

struct DuplicateScanResult {
    std::vector<DuplicateGroup> groups;
    int64_t totalWastedBytes{0};
    int64_t totalDuplicateFiles{0};
    int64_t totalGroups{0};
    int64_t scannedCandidates{0};
    int64_t elapsedMs{0};
};
