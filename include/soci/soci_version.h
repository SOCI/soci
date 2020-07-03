//  SOCI version.hpp configuration header file

//
// Copyright (C) 2011 Mateusz Loskot <mateusz@loskot.net>
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_VERSION_HPP
#define SOCI_VERSION_HPP

#include <string>

std::string getSOCIVersion();
unsigned getSOCIVersionMajor();
unsigned getSOCIVersionMinor();
unsigned getSOCIVersionPatch();
unsigned getSOCIVersionTweak();

#endif // SOCI_VERSION_HPP
