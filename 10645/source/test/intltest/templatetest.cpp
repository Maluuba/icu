/*
*******************************************************************************
* Copyright (C) 2014, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File TEMPLATETEST.CPP
*
********************************************************************************
*/
#include "cstring.h"
#include "intltest.h"
#include "template.h"

#define LENGTHOF(array) (int32_t)(sizeof(array) / sizeof((array)[0]))

class TemplateTest : public IntlTest {
public:
    TemplateTest() {
    }
    void TestNoPlaceholders();
    void TestOnePlaceholder();
    void TestManyPlaceholders();
    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=0);
private:
};

void TemplateTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* /*par*/) {
  TESTCASE_AUTO_BEGIN;
  TESTCASE_AUTO(TestNoPlaceholders);
  TESTCASE_AUTO(TestOnePlaceholder);
  TESTCASE_AUTO(TestManyPlaceholders);
  TESTCASE_AUTO_END;
}

void TemplateTest::TestNoPlaceholders() {
    UErrorCode status = U_ZERO_ERROR;
    Template t;
    t.compile("This doesn''t have templates '{0}", status);
    assertEquals("PlaceholderCount", 0, t.getPlaceholderCount());
    UnicodeString appendTo;
    assertEquals(
            "Evaluate",
            "This doesn't have templates {0}", 
            t.evaluate(
                    NULL,
                    0,
                    appendTo,
                    status));
    assertSuccess("Status", status);
}

void TemplateTest::TestOnePlaceholder() {
    UErrorCode status = U_ZERO_ERROR;
    Template t;
    t.compile("{0} meter", status);
    assertEquals("PlaceholderCount", 1, t.getPlaceholderCount());
    UnicodeString one("1");
    UnicodeString appendTo;
    assertEquals(
            "Evaluate",
            "1 meter",
            t.evaluate(
                    &one,
                    1,
                    appendTo,
                    status));
    assertSuccess("Status", status);

    // assignment
    Template s;
    s = t;
    assertEquals(
            "Assignment",
            "1 meter",
            s.evaluate(
                    &one,
                    1,
                    appendTo,
                    status));

    // Copy constructor
    Template r(t);
    assertEquals(
            "Copy constructor",
            "1 meter",
            r.evaluate(
                    &one,
                    1,
                    appendTo,
                    status));
    assertSuccess("Status", status);
}

void TemplateTest::TestManyPlaceholders() {
    UErrorCode status = U_ZERO_ERROR;
    Template t;
    t.compile(
            "Templates {2}{1}{5} and {4} are out of order.", status);
    assertEquals("PlaceholderCount", 6, t.getPlaceholderCount());
    UnicodeString values[] = {
            "freddy", "tommy", "frog", "billy", "leg", "{0}"};
    int32_t offsets[6];
    int32_t expectedOffsets[6] = {-1, 14, 10, -1, 27, 19};
    UnicodeString appendTo;
    assertEquals(
            "Evaluate",
            "Templates frogtommy{0} and leg are out of order.",
            t.evaluate(
                    values,
                    LENGTHOF(values),
                    appendTo,
                    offsets,
                    LENGTHOF(offsets),
                    status));
    assertSuccess("Status", status);
    for (int32_t i = 0; i < LENGTHOF(expectedOffsets); ++i) {
        if (expectedOffsets[i] != offsets[i]) {
            errln("Expected %d, got %d", expectedOffsets[i], offsets[i]);
        }
    }
    t.evaluate(
            values,
            LENGTHOF(values) - 1,
            appendTo,
            offsets,
            LENGTHOF(offsets),
            status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        errln("Expected U_ILLEGAL_ARGUMENT_ERROR");
    }
    status = U_ZERO_ERROR;
    t.evaluate(
            values,
            LENGTHOF(values),
            appendTo,
            offsets,
            LENGTHOF(offsets) - 1,
            status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        errln("Expected U_ILLEGAL_ARGUMENT_ERROR");
    }
    status = U_ZERO_ERROR;

    // Test assignment
    Template s;
    s = t;
    assertEquals(
            "Assignment",
            "Templates frogtommy{0} and leg are out of order.",
            s.evaluate(
                    values,
                    LENGTHOF(values),
                    appendTo,
                    status));

    // Copy constructor
    Template r(t);
    assertEquals(
            "Assignment",
            "Templates frogtommy{0} and leg are out of order.",
            r.evaluate(
                    values,
                    LENGTHOF(values),
                    appendTo,
                    status));
    assertSuccess("Status", status);
}

extern IntlTest *createTemplateTest() {
    return new TemplateTest();
}
