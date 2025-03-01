// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025 Gonzalo Garramuño
// All rights reserved.

#include <tlCore/StringFormat.h>

#include <NDI/Processing.NDI.Lib.h>

#include <FL/fl_utf8.h>

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;
#include <stdexcept>

namespace tl
{
    namespace
    {
        std::string rootpath()
        {
            const char* root = fl_getenv("MRV2_ROOT");
            if (!root)
                root = "..";
            return root;
        }
        
        bool isReadable(const fs::path& p)
        {
            const std::string& filePath = p.generic_string();
            if (filePath.empty())
                return false;

            std::ifstream f(filePath);
            if (f.is_open())
            {
                f.close();
                return true;
            }

            return false;
        }
    }
    
    namespace ndi
    {
        //! Path to NDI (if installed and found)
        std::string NDI_library()
        {
            const std::string library = NDILIB_LIBRARY_NAME;
            std::string libpath = rootpath() + "/lib/";
            std::string fullpath = libpath + library;
            if (!isReadable(fullpath))
            {
                const char* env = fl_getenv(NDILIB_REDIST_FOLDER);
                if (env)
                    libpath = env;
                if (!libpath.empty())
                {
                    fullpath = libpath + "/" + library;
                }
                else
                {
                    libpath = "/usr/local/lib/";
                    fullpath = libpath + library;
                    if (!isReadable(fullpath))
                    {
                        throw std::runtime_error(NDILIB_LIBRARY_NAME
                                                 " was not found.  "
                                                 "Please download it from "
                                                 "http://ndi.link/NDIRedistV6");
                    }
                }
            }
            
            return fullpath;
        }
    }
}
