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

#include "ysfx_reader.hpp"

namespace ysfx {

//------------------------------------------------------------------------------
bool text_reader::read_next_line(std::string &line)
{
    line.clear();

    char next = read_next_char();
    if (next == '\0')
        return false;

    while (next != '\0' && next != '\r' && next != '\n') {
        line.push_back(next);
        next = read_next_char();
    }

    if (next == '\r') {
        next = peek_next_char();
        if (next == '\n')
            read_next_char();
    }

    return true;
}

//------------------------------------------------------------------------------
char string_text_reader::read_next_char()
{
    const char *ptr = m_char_ptr;

    if (!ptr || *ptr == '\0')
        return '\0';

    char next = *ptr;
    m_char_ptr = ptr + 1;
    return next;
}

char string_text_reader::peek_next_char()
{
    const char *ptr = m_char_ptr;

    if (!ptr)
        return '\0';

    return *ptr;
}

//------------------------------------------------------------------------------
char stdio_text_reader::read_next_char()
{
    FILE *stream = m_stream;

    if (!stream)
        return '\0';

    int next = fgetc(stream);
    if (next == EOF)
        return '\0';

    return (unsigned char)next;
}

char stdio_text_reader::peek_next_char()
{
    FILE *stream = m_stream;

    if (!stream)
        return '\0';

    int next = fgetc(stream);
    if (next == EOF)
        return '\0';

    ungetc(next, stream);
    return (unsigned char)next;
}

} // namespace ysfx
