//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MNSOCISTRING_H_INCLUDED
#define MNSOCISTRING_H_INCLUDED

#include "soci-config.h"



class SOCI_DECL MNSociString
{
public:
    MNSociString(short iCharLength = 256) { m_iCharLength = iCharLength; m_ptrCharData = new char[m_iCharLength + 1]; m_ptrCharData[0] = '\0'; }
    MNSociString(const char* ptrChar, short iCharLength) { m_iCharLength = iCharLength; m_ptrCharData = new char[m_iCharLength + 1]; strcpy(m_ptrCharData, ptrChar); }
    MNSociString(const MNSociString& obj) { m_iCharLength = obj.m_iCharLength; m_ptrCharData = new char[m_iCharLength + 1]; *this = obj; }
    ~MNSociString() 
    {
        if (m_ptrCharData != NULL)  
        { 
            delete m_ptrCharData; 
            m_ptrCharData = NULL; 
        }
    }

    MNSociString& operator = (const MNSociString& obj)  { strcpy(m_ptrCharData, obj.m_ptrCharData); return *this; }
    MNSociString& operator = (char* ptrChar)            { strcpy(m_ptrCharData, ptrChar); return *this; }
    MNSociString& operator = (const char* ptrChar)      { strcpy(m_ptrCharData, ptrChar); return *this; }

    char* m_ptrCharData;
    short m_iCharLength;
};

#endif // MNSOCISTRING_H_INCLUDED
