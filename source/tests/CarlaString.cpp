/*
 * Carla Tests
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaString.hpp"

int main()
{
    CarlaString str;

    // empty
    assert(str.length() == 0);
    assert(str.length() == std::strlen(""));
    assert(str.isEmpty());
    assert(str.contains(""));
    assert(! str.isNotEmpty());
    assert(! str.contains("-"));
    assert(! str.isDigit(0));

    // single number
    str = "5";
    assert(str.length() == 1);
    assert(str.length() == std::strlen("5"));
    assert(str.isNotEmpty());
    assert(str.contains(""));
    assert(str.contains("5"));
    assert(str.isDigit(0));
    assert(! str.isEmpty());
    assert(! str.contains("6"));
    assert(! str.isDigit(1));

    // single number, using constructor
    str = CarlaString(5);
    assert(str.length() == 1);
    assert(str.length() == std::strlen("5"));
    assert(str.isNotEmpty());
    assert(str.contains(""));
    assert(str.contains("5"));
    assert(str.isDigit(0));
    assert(! str.isEmpty());
    assert(! str.contains("6"));
    assert(! str.isDigit(1));

    // decimal number
    str = CarlaString(51);
    assert(str.length() == 2);
    assert(str.length() == std::strlen("51"));
    assert(str.isNotEmpty());
    assert(str.contains(""));
    assert(str.contains("1"));
    assert(str.contains("51"));
    assert(str.isDigit(0));
    assert(str.isDigit(1));
    assert(! str.isEmpty());
    assert(! str.contains("6"));
    assert(! str.isDigit(2));
    assert(! str.isDigit(-1));

    // negative number
    str = CarlaString(-51);
    assert(str.length() == 3);
    assert(str.length() == std::strlen("-51"));
    assert(str.isNotEmpty());
    assert(str.contains(""));
    assert(str.contains("-5"));
    assert(str.contains("51"));
    assert(str.isDigit(1));
    assert(str.isDigit(2));
    assert(! str.isEmpty());
    assert(! str.contains("6"));
    assert(! str.isDigit(0));
    assert(! str.isDigit(-1));

    // small operations
    str += "ah";
    assert(str.length() == 5);
    assert(str.length() == std::strlen("-51ah"));
    assert(str.contains("-51ah"));
    assert(! str.isDigit(3));

    // hexacimal number
    unsigned int n = 0x91;
    str += CarlaString(n, true);
    assert(str.length() == 9);
    assert(str.length() == std::strlen("-51ah0x91"));
    assert(str.contains("-51ah0x91"));
    assert(! str.isDigit(6));

    // float number
    str += CarlaString(0.0102f);
    assert(str.length() == 17);
    assert(str.length() == std::strlen("-51ah0x910.010200"));
    assert(str.contains("-51ah0x91"));
    assert(! str.isDigit(6));

    // double number
    str += CarlaString(7.9642);
    assert(str.length() == 23);
    assert(str.length() == std::strlen("-51ah0x910.0102007.9642"));
    assert(str.contains("7.9642"));

    // replace
    str.replace('0', 'k');
    str.replace('k', 'O');
    str.replace('O', '0');
    str.replace('0', '\0'); // shouldn't do anything

    // truncate
    str.truncate(11);
    assert(str.length() == 11);
    assert(str.length() == std::strlen("-51ah0x910."));

    // basic
    str.toBasic();
    assert(str.length() == 11);
    assert(str == "_51ah0x910_");

    // upper
    str.toUpper();
    assert(str.length() == 11);
    assert(str == "_51AH0X910_");

    // lower
    str.toLower();
    assert(str.length() == 11);
    assert(str == "_51ah0x910_");

    // random stuff
    CarlaString str1(1.23);
    str1 += "_  ?";

    CarlaString str2("test1");
    str2  = "test2";
    str2 += ".0";

    CarlaString str3("1.23_  ?test2.0 final");

    CarlaString str4 = "" + str1 + str2 + " final";

    assert(str3 == "1.23_  ?test2.0 final");
    assert(str3 == str4);
    assert(str3.length() == str4.length());
    assert(str3.length() == std::strlen("1.23_  ?test2.0 final"));

    CarlaString str5 = "ola " + str + " " + CarlaString(6);
    assert(str5 == "ola _51ah0x910_ 6");
    assert(str5.length() == std::strlen("ola _51ah0x910_ 6"));

    printf("FINAL: \"%s\"\n", (const char*)str5);

    // clear
    str.clear();
    assert(str.length() == 0);
    assert(str.length() == std::strlen(""));
    assert(str == "");

    return 0;
}
