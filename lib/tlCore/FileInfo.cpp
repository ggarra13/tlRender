// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlCore/FileInfo.h>

#include <tlCore/Error.h>
#include <tlCore/String.h>

#include <fseq.h>

#include <algorithm>
#include <array>

namespace tl
{
    namespace file
    {
        TLRENDER_ENUM_IMPL(
            Type,
            "File",
            "Directory");
        TLRENDER_ENUM_SERIALIZE_IMPL(Type);

        FileInfo::FileInfo()
        {}

        FileInfo::FileInfo(const Path& path) :
            _path(path)
        {
            std::string error;
            _stat(&error);
        }

        bool ListOptions::operator == (const ListOptions& other) const
        {
            return
                dotAndDotDotDirs == other.dotAndDotDotDirs &&
                dotFiles == other.dotFiles &&
                sequence == other.sequence &&
                negativeNumbers == other.negativeNumbers &&
                maxNumberDigits == other.maxNumberDigits;
        }

        bool ListOptions::operator != (const ListOptions& other) const
        {
            return !(*this == other);
        }

        std::vector<FileInfo> list(const std::string& path, const ListOptions& options)
        {
            std::vector<FileInfo> out;
            FSeqDirOptions dirOptions;
            fseqDirOptionsInit(&dirOptions);
            dirOptions.dotAndDotDotDirs = options.dotAndDotDotDirs;
            dirOptions.dotFiles = options.dotFiles;
            dirOptions.sequence = options.sequence;
            dirOptions.fileNameOptions.negativeNumbers = options.negativeNumbers;
            dirOptions.fileNameOptions.maxNumberDigits = options.maxNumberDigits;
            FSeqBool error = FSEQ_FALSE;
            auto dirList = fseqDirList(path.c_str(), &dirOptions, &error);
            if (FSEQ_FALSE == error)
            {
                const std::string directory = appendSeparator(path);
                for (auto entry = dirList; entry; entry = entry->next)
                {
                    out.push_back(FileInfo(Path(
                        directory,
                        entry->fileName.base,
                        entry->fileName.number,
                        entry->framePadding,
                        entry->fileName.extension)));
                }
            }
            fseqDirListDel(dirList);
            return out;
        }
    }
}
