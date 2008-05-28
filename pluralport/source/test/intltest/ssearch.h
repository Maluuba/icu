/*
 **********************************************************************
 *   Copyright (C) 2005-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __SSEARCH_H
#define __SSEARCH_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/ucol.h"

#include "intltest.h"

//
//  Test of the function usearch_search()
//
//     See srchtest.h for the tests for the rest of the string search functions.
//
class SSearchTest: public IntlTest {
public:
  
    SSearchTest();
    virtual ~SSearchTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* params = NULL );

    virtual void searchTest();
    virtual void offsetTest();
    virtual void monkeyTest(char *params);

private:
    virtual const char   *getPath(char buffer[2048], const char *filename);
    virtual       int32_t monkeyTestCase(UCollator *coll, const UnicodeString &testCase, const UnicodeString &pattern, const UnicodeString &altPattern,
                                         const char *name, const char *strength, uint32_t seed);
};

#endif
