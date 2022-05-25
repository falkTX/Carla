// Copyright 2021 Jean Pierre Cimalando
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//

#if !defined(_WIN32)

// fts lacks large file support in glibc < 2.23
#if defined(YSFX_FTS_LACKS_LFS_SUPPORT)
#   undef _FILE_OFFSET_BITS
#endif

#include "ysfx_utils.hpp"
#include <fts.h>
#include <string>
#include <cstring>

namespace ysfx {

void visit_directories(const char *rootpath, bool (*visit)(const std::string &, void *), void *data)
{
    char *argv[] = {(char *)rootpath, nullptr};

    auto compar = [](const FTSENT **a, const FTSENT **b) -> int {
        return strcmp((*a)->fts_name, (*b)->fts_name);
    };

    FTS *fts = fts_open(argv, FTS_NOCHDIR|FTS_PHYSICAL, +compar);
    if (!fts)
        return;
    auto fts_cleanup = defer([fts]() { fts_close(fts); });

    std::string pathbuf;
    pathbuf.reserve(1024);

    while (FTSENT *ent = fts_read(fts)) {
        if (ent->fts_info == FTS_D) {
            pathbuf.assign(ent->fts_path);
            pathbuf.push_back('/');
            if (!visit(pathbuf, data))
                return;
        }
    }
}

} // namespace ysfx

#endif // !defined(_WIN32)
