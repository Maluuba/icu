/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2002-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

//
//   regextst.cpp
//
//      ICU Regular Expressions test, part of intltest.
//

#include "intltest.h"

#include "v32test.h"
#include "uvectr32.h"
#include "uvector.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>


//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
UVector32Test::UVector32Test() 
{
}


UVector32Test::~UVector32Test()
{
}



void UVector32Test::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite UVector32Test: ");
    switch (index) {

        case 0: name = "UVector32_API";
            if (exec) UVector32_API(); 
            break;
        default: name = ""; 
            break; //needed to end loop
    }
}



//---------------------------------------------------------------------------
//
//      UVector32_API      Check for basic functionality of UVector32.
//
//---------------------------------------------------------------------------
void UVector32Test::UVector32_API() {

    UErrorCode  status = U_ZERO_ERROR;
    UVector32     *a;
    UVector32     *b;

    a = new UVector32(status);
    ASSERT_SUCCESS(status);
    delete a;

    status = U_ZERO_ERROR;
    a = new UVector32(2000, status);
    ASSERT_SUCCESS(status);
    delete a;

    //
    //  assign()
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    b->assign(*a, status);
    ASSERT_EQUALS(3, b->size());
    ASSERT_EQUALS2((4, b->size(), "and some %s stuff", "xxx"));
    ASSERT_EQUALS(20, b->elementAti(1));
    ASSERT_SUCCESS(status);
    delete a;
    delete b;

    //
    //  operator == and != and equals()
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    ASSERT_TRUE(*b != *a);
    ASSERT_FALSE(*b == *a);
    ASSERT_FALSE(b->equals(*a));
    b->assign(*a, status);
    ASSERT_TRUE(*b == *a);
    ASSERT_FALSE(*b != *a);
    ASSERT_TRUE(b->equals(*a));
    b->addElement(666, status);
    ASSERT_TRUE(*b != *a);
    ASSERT_FALSE(*b == *a);
    ASSERT_FALSE(b->equals(*a));
    ASSERT_SUCCESS(status);
    delete b;
    delete a;

    //
    //  addElement().   Covered by above tests.
    //

    //
    // setElementAt()
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    a->setElementAt(666, 1);
    ASSERT_EQUALS(10, a->elementAti(0));
    ASSERT_EQUALS(666, a->elementAti(1));
    ASSERT_EQUALS(3, a->size());
    ASSERT_SUCCESS(status);
    delete a;

    //
    // insertElementAt()
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    a->insertElementAt(666, 1, status);
    ASSERT_EQUALS(10, a->elementAti(0));
    ASSERT_EQUALS(666, a->elementAti(1));
    ASSERT_EQUALS(20, a->elementAti(2));
    ASSERT_EQUALS(30, a->elementAti(3));
    ASSERT_EQUALS(4, a->size());
    ASSERT_SUCCESS(status);
    delete a;

    //
    //  elementAti()    covered by above tests
    //

    //
    //  lastElementi
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    ASSERT_EQUALS(30, a->lastElementi());
    ASSERT_SUCCESS(status);
    delete a;


    //
    //  indexOf
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    ASSERT_EQUALS(2, a->indexOf(30, 0));
    ASSERT_EQUALS(-1, a->indexOf(40, 0));
    ASSERT_EQUALS(0, a->indexOf(10, 0));
    ASSERT_EQUALS(-1, a->indexOf(10, 1));
    ASSERT_SUCCESS(status);
    delete a;

    
    //
    //  contains
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    ASSERT_TRUE(a->contains(10));
    ASSERT_FALSE(a->contains(11));
    ASSERT_TRUE(a->contains(20));
    ASSERT_FALSE(a->contains(-10));
    ASSERT_SUCCESS(status);
    delete a;


    //
    //  containsAll
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    ASSERT_TRUE(a->containsAll(*b));
    b->addElement(2, status);
    ASSERT_FALSE(a->containsAll(*b));
    b->setElementAt(10, 0);
    ASSERT_TRUE(a->containsAll(*b));
    ASSERT_FALSE(b->containsAll(*a));
    b->addElement(30, status);
    b->addElement(20, status);
    ASSERT_TRUE(a->containsAll(*b));
    ASSERT_TRUE(b->containsAll(*a));
    b->addElement(2, status);
    ASSERT_FALSE(a->containsAll(*b));
    ASSERT_TRUE(b->containsAll(*a));
    ASSERT_SUCCESS(status);
    delete a;
    delete b;

    //
    //  removeAll
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    a->removeAll(*b);
    ASSERT_EQUALS(3, a->size());
    b->addElement(20, status);
    a->removeAll(*b);
    ASSERT_EQUALS(2, a->size());
    ASSERT_TRUE(a->contains(10));
    ASSERT_TRUE(a->contains(30));
    b->addElement(10, status);
    a->removeAll(*b);
    ASSERT_EQUALS(1, a->size());
    ASSERT_TRUE(a->contains(30));
    ASSERT_SUCCESS(status);
    delete a;
    delete b;


    // retainAll
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    b->addElement(10, status);
    b->addElement(20, status);
    b->addElement(30, status);
    b->addElement(15, status);
    a->retainAll(*b);
    ASSERT_EQUALS(3, a->size());
    b->removeElementAt(1);
    a->retainAll(*b);
    ASSERT_FALSE(a->contains(20));
    ASSERT_EQUALS(2, a->size());
    b->removeAllElements();
    ASSERT_EQUALS(0, b->size());
    a->retainAll(*b);
    ASSERT_EQUALS(0, a->size());
    ASSERT_SUCCESS(status);
    delete a;
    delete b;

    //
    //  removeElementAt   Tested above.
    //

    //
    //  removeAllElments   Tested above
    //

    //
    //  size()   tested above
    //

    //
    //  isEmpty
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    ASSERT_TRUE(a->isEmpty());
    a->addElement(10, status);
    ASSERT_FALSE(a->isEmpty());
    a->addElement(20, status);
    a->removeElementAt(0);
    ASSERT_FALSE(a->isEmpty());
    a->removeElementAt(0);
    ASSERT_TRUE(a->isEmpty());
    ASSERT_SUCCESS(status);
    delete a;


    //
    // ensureCapacity, expandCapacity
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    ASSERT_TRUE(a->isEmpty());
    a->addElement(10, status);
    ASSERT_TRUE(a->ensureCapacity(5000, status));
    ASSERT_TRUE(a->expandCapacity(20000, status));
    ASSERT_SUCCESS(status);
    delete a;
    
    //
    // setSize
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    a->setSize(100);
    ASSERT_EQUALS(100, a->size());
    ASSERT_EQUALS(10, a->elementAti(0));
    ASSERT_EQUALS(20, a->elementAti(1));
    ASSERT_EQUALS(30, a->elementAti(2));
    ASSERT_EQUALS(0, a->elementAti(3));
    a->setElementAt(666, 99);
    a->setElementAt(777, 100);
    ASSERT_EQUALS(666, a->elementAti(99));
    ASSERT_EQUALS(0, a->elementAti(100));
    a->setSize(2);
    ASSERT_EQUALS(20, a->elementAti(1));
    ASSERT_EQUALS(0, a->elementAti(2));
    ASSERT_EQUALS(2, a->size());
    a->setSize(0);
    ASSERT_TRUE(a->empty());
    ASSERT_EQUALS(0, a->size());

    ASSERT_SUCCESS(status);
    delete a;

    //
    // containsNone
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    ASSERT_TRUE(a->containsNone(*b));
    b->addElement(5, status);
    ASSERT_TRUE(a->containsNone(*b));
    b->addElement(30, status);
    ASSERT_FALSE(a->containsNone(*b));

    ASSERT_SUCCESS(status);
    delete a;
    delete b;

    //
    // sortedInsert
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->sortedInsert(30, status);
    a->sortedInsert(20, status);
    a->sortedInsert(10, status);
    ASSERT_EQUALS(10, a->elementAti(0));
    ASSERT_EQUALS(20, a->elementAti(1));
    ASSERT_EQUALS(30, a->elementAti(2));

    ASSERT_SUCCESS(status);
    delete a;

    //
    // getBuffer
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    int32_t *buf = a->getBuffer();
    ASSERT_EQUALS(10, buf[0]);
    ASSERT_EQUALS(20, buf[1]);
    a->setSize(20000);
    int32_t *resizedBuf;
    resizedBuf = a->getBuffer();
    //TEST_ASSERT(buf != resizedBuf); // The buffer might have been realloc'd
    ASSERT_EQUALS(10, resizedBuf[0]);
    ASSERT_EQUALS(20, resizedBuf[1]);

    ASSERT_SUCCESS(status);
    delete a;


    //
    //  empty
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    ASSERT_TRUE(a->empty());
    a->addElement(10, status);
    ASSERT_FALSE(a->empty());
    a->addElement(20, status);
    a->removeElementAt(0);
    ASSERT_FALSE(a->empty());
    a->removeElementAt(0);
    ASSERT_TRUE(a->empty());
    ASSERT_SUCCESS(status);
    delete a;


    //
    //  peeki
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    ASSERT_EQUALS(10, a->peeki());
    a->addElement(20, status);
    ASSERT_EQUALS(20, a->peeki());
    a->addElement(30, status);
    ASSERT_EQUALS(30, a->peeki());
    ASSERT_SUCCESS(status);
    delete a;


    //
    // popi
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    ASSERT_EQUALS(30, a->popi());
    ASSERT_EQUALS(20, a->popi());
    ASSERT_EQUALS(10, a->popi());
    ASSERT_EQUALS(0, a->popi());
    ASSERT_TRUE(a->isEmpty());
    ASSERT_SUCCESS(status);
    delete a;

    //
    // push
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    ASSERT_EQUALS(10, a->push(10, status));
    ASSERT_EQUALS(20, a->push(20, status));
    ASSERT_EQUALS(30, a->push(30, status));
    ASSERT_EQUALS(3, a->size());
    ASSERT_EQUALS(30, a->popi());
    ASSERT_EQUALS(20, a->popi());
    ASSERT_EQUALS(10, a->popi());
    ASSERT_TRUE(a->isEmpty());
    ASSERT_SUCCESS(status);
    delete a;


    //
    // reserveBlock
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->ensureCapacity(1000, status);

    // TODO:

    ASSERT_SUCCESS(status);
    delete a;

}

