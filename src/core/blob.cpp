//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "blob.h"
#include "session.h"

#include <cstddef>
#include <vector>

using namespace soci;

blob::blob(session & s) : session_(s)
{
    backEnd_ = session_.make_blob_backend();
}

blob::~blob()
{
    delete backEnd_;
}

blob::blob(blob const &b) : session_(b.session_)
{
  backEnd_ = session_.make_blob_backend();
  *this = b;
}

blob  & blob::operator = (blob const &b)
{
  blob              & bb = const_cast<blob &>(b);
  
  if (size_t len = bb.get_len())
  {
    std::vector<char>   buf(len);

    bb.read(0, &buf[0], len);
    this->write(0, &buf[0], len);
  }
  else
    trim(0);

  return *this;
}

std::size_t blob::get_len()
{
    return backEnd_->get_len();
}

std::size_t blob::read(std::size_t offset, char *buf, std::size_t toRead)
{
    return backEnd_->read(offset, buf, toRead);
}

std::size_t blob::write(
    std::size_t offset, char const * buf, std::size_t toWrite)
{
    return backEnd_->write(offset, buf, toWrite);
}

std::size_t blob::append(char const * buf, std::size_t toWrite)
{
    return backEnd_->append(buf, toWrite);
}

void blob::trim(std::size_t newLen)
{
    backEnd_->trim(newLen);
}
