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

#pragma once
#include <string>
#include <cstdio>
#include <cstddef>

namespace ysfx {

class text_reader
{
public:
    virtual ~text_reader() = default;
    virtual char read_next_char() = 0;
    virtual char peek_next_char() = 0;
    bool read_next_line(std::string &line);
};

//------------------------------------------------------------------------------
class string_text_reader : public text_reader
{
public:
    explicit string_text_reader(const char *text) : m_char_ptr(text) {}
    char read_next_char() override;
    char peek_next_char() override;
private:
    const char *m_char_ptr = nullptr;
};

//------------------------------------------------------------------------------
class stdio_text_reader : public text_reader
{
public:
    explicit stdio_text_reader(FILE *stream) : m_stream(stream) {}
    char read_next_char() override;
    char peek_next_char() override;
private:
    FILE *m_stream = nullptr;
};

} // namespace ysfx
