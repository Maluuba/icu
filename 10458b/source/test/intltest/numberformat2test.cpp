/*
*******************************************************************************
* Copyright (C) 2015, International Business Machines Corporation and         *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File NUMBERFORMAT2TEST.CPP
*
*******************************************************************************
*/
#include "unicode/utypes.h"

#include "intltest.h"

#if !UCONFIG_NO_FORMATTING

#include "digitformatter.h"
#include "sciformatter.h"
#include "digitinterval.h"
#include "significantdigitinterval.h"
#include "digitlst.h"
#include "digitgrouping.h"
#include "unicode/localpointer.h"
#include "fphdlimp.h"
#include "digitaffixesandpadding.h"
#include "valueformatter.h"
#include "precision.h"
#include "plurrule_impl.h"
#include "unicode/plurrule.h"
#include "decimalformatpattern.h"
#include "affixpatternparser.h"
#include "datadrivennumberformattestsuite.h"
#include "charstr.h"
#include "smallintformatter.h"
#include "decimfmt2.h"
#include "uassert.h"

static const int32_t kIntField = 4938;
static const int32_t kSignField = 5770;

struct NumberFormat2Test_Attributes {
    int32_t id;
    int32_t spos;
    int32_t epos;
};

class NumberFormat2Test_FieldPositionHandler : public FieldPositionHandler {
public:
NumberFormat2Test_Attributes attributes[100];
int32_t count;
UBool bRecording;



NumberFormat2Test_FieldPositionHandler() : count(0), bRecording(TRUE) { attributes[0].spos = -1; }
NumberFormat2Test_FieldPositionHandler(UBool recording) : count(0), bRecording(recording) { attributes[0].spos = -1; }
virtual ~NumberFormat2Test_FieldPositionHandler();
virtual void addAttribute(int32_t id, int32_t start, int32_t limit);
virtual void shiftLast(int32_t delta);
virtual UBool isRecording(void) const;
};
 
NumberFormat2Test_FieldPositionHandler::~NumberFormat2Test_FieldPositionHandler() {
}

void NumberFormat2Test_FieldPositionHandler::addAttribute(
        int32_t id, int32_t start, int32_t limit) {
    if (count == UPRV_LENGTHOF(attributes) - 1) {
        return;
    }
    attributes[count].id = id;
    attributes[count].spos = start;
    attributes[count].epos = limit;
    ++count;
    attributes[count].spos = -1;
}

void NumberFormat2Test_FieldPositionHandler::shiftLast(int32_t /* delta */) {
}

UBool NumberFormat2Test_FieldPositionHandler::isRecording() const {
    return bRecording;
}

class NumberFormat2TestDataDriven : public DataDrivenNumberFormatTestSuite {
protected:
UBool isFormatPass(
        const NumberFormatTestTuple &tuple,
        UObject *somePreviousFormatter,
        UnicodeString &appendErrorMessage,
        UErrorCode &status);
UObject *newFormatter(UErrorCode &status);
UBool isToPatternPass(
        const NumberFormatTestTuple &tuple,
        UnicodeString &appendErrorMessage,
        UErrorCode &status);
UBool isSelectPass(
        const NumberFormatTestTuple &tuple,
        UnicodeString &appendErrorMessage,
        UErrorCode &status);
UBool isParsePass(
        const NumberFormatTestTuple &tuple,
        UnicodeString &appendErrorMessage,
        UErrorCode &status);
UBool isParseCurrencyPass(
        const NumberFormatTestTuple &tuple,
        UnicodeString &appendErrorMessage,
        UErrorCode &status);
private:

};

#define CHECK_EQUALITY(fmt, fmtCopy, fieldName, getter, errors) \
    if ((fmt).getter() != (fmtCopy).getter() && (fmt) == (fmtCopy)) { \
        (errors).append(#fieldName); \
        (errors).append(": set/equality mismatch"); \
    } \

#define SET_AND_CHECK_WITH_STATUS_LONG(fmt, fieldName, setter, getter, expr, cond, errors) { \
    DecimalFormat2 fmtCopy(fmt); \
    if (fmtCopy != (fmt)) { \
        (errors).append("copy constructor equality mismatch"); \
    } else { \
        UErrorCode status = U_ZERO_ERROR; \
        (fmt).setter(expr, status); \
        if (U_FAILURE(status)) { \
            (errors).append(#fieldName); \
            (errors).append(": error setting."); \
        } else if ((cond) && (fmt).getter() != (expr)) { \
            (errors).append(#fieldName); \
            (errors).append(": set/get mismatch"); \
        } \
        CHECK_EQUALITY(fmt, fmtCopy, fieldName, getter, errors); \
    } \
}

#define SET_AND_CHECK_WITH_COND_LONG(fmt, fieldName, setter, getter, expr, cond, errors) { \
    DecimalFormat2 fmtCopy(fmt); \
    if (fmtCopy != (fmt)) { \
        (errors).append("copy constructor equality mismatch"); \
    } else { \
        (fmt).setter(expr); \
        if ((cond) && (fmt).getter() != (expr)) { \
            (errors).append(#fieldName); \
            (errors).append(": set/get mismatch"); \
        } \
        CHECK_EQUALITY(fmt, fmtCopy, fieldName, getter, errors); \
    } \
}

#define SET_AND_CHECK(fmt, fieldName, expr, errors) SET_AND_CHECK_WITH_COND_LONG(fmt, fieldName, set##fieldName, get##fieldName, expr, TRUE, errors)

#define SET_AND_CHECK_WITH_STATUS(fmt, fieldName, expr, errors) SET_AND_CHECK_WITH_STATUS_LONG(fmt, fieldName, set##fieldName, get##fieldName, expr, TRUE, errors)

#define SET_AND_CHECK_WITH_COND(fmt, fieldName, expr, cond, errors) SET_AND_CHECK_WITH_COND_LONG(fmt, fieldName, set##fieldName, get##fieldName, expr, cond, errors)

#define SET_AND_CHECK_BOOL(fmt, fieldName, expr, errors) SET_AND_CHECK_WITH_COND_LONG(fmt, fieldName, set##fieldName, is##fieldName, expr, TRUE, errors)

static DigitList &strToDigitList(
        const UnicodeString &str,
        DigitList &digitList,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return digitList;
    }
    if (str == "NaN") {
        digitList.set(uprv_getNaN());
        return digitList;
    }
    if (str == "-Inf") {
        digitList.set(-1*uprv_getInfinity());
        return digitList;
    }
    if (str == "Inf") {
        digitList.set(uprv_getInfinity());
        return digitList;
    }
    CharString formatValue;
    formatValue.appendInvariantChars(str, status);
    digitList.set(StringPiece(formatValue.data()), status, 0);
    return digitList;
}

static UnicodeString &format(
        const DecimalFormat2 &fmt,
        const DigitList &digitList,
        UnicodeString &appendTo,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return appendTo;
    }
    FieldPosition fpos(FieldPosition::DONT_CARE);
    return fmt.format(digitList, appendTo, fpos, status);
}

static DigitList::ERoundingMode convertRoundingMode(
        DecimalFormat::ERoundingMode mode) {
    switch (mode) {
    case DecimalFormat::kRoundCeiling:
        return DigitList::kRoundCeiling;
    case DecimalFormat::kRoundFloor:
        return DigitList::kRoundFloor;
    case DecimalFormat::kRoundDown:
        return DigitList::kRoundDown;
    case DecimalFormat::kRoundUp:
        return DigitList::kRoundUp;
    case DecimalFormat::kRoundHalfEven:
        return DigitList::kRoundHalfEven;
    case DecimalFormat::kRoundHalfDown:
        return DigitList::kRoundHalfDown;
    case DecimalFormat::kRoundHalfUp:
        return DigitList::kRoundHalfUp;
    default:
        U_ASSERT(FALSE);
        break;
    }
    return DigitList::kRoundHalfUp;
}

static DigitAffixesAndPadding::EPadPosition convertPadPosition(
        DecimalFormat::EPadPosition position) {
    switch (position) {
    case DecimalFormat::kPadBeforePrefix:
        return DigitAffixesAndPadding::kPadBeforePrefix;
    case DecimalFormat::kPadAfterPrefix:
        return DigitAffixesAndPadding::kPadAfterPrefix;
    case DecimalFormat::kPadBeforeSuffix:
        return DigitAffixesAndPadding::kPadBeforeSuffix;
    case DecimalFormat::kPadAfterSuffix:
        return DigitAffixesAndPadding::kPadAfterSuffix;
    default:
        U_ASSERT(FALSE);
        break;
    }
    return DigitAffixesAndPadding::kPadBeforePrefix;
}

static void adjustDecimalFormat(
        const NumberFormatTestTuple &tuple,
        DecimalFormat2 &fmt,
        UnicodeString &appendErrorMessage) {
    if (tuple.minIntegerDigitsFlag) {
        SET_AND_CHECK(
                fmt,
                MinimumIntegerDigits,
                tuple.minIntegerDigits,
                appendErrorMessage);
    }
    if (tuple.maxIntegerDigitsFlag) {
        SET_AND_CHECK(
                fmt,
                MaximumIntegerDigits,
                tuple.maxIntegerDigits,
                appendErrorMessage);
    }
    if (tuple.minFractionDigitsFlag) {
        SET_AND_CHECK(
                fmt,
                MinimumFractionDigits,
                tuple.minFractionDigits,
                appendErrorMessage);
    }
    if (tuple.maxFractionDigitsFlag) {
        SET_AND_CHECK(
                fmt,
                MaximumFractionDigits,
                tuple.maxFractionDigits,
                appendErrorMessage);
    }
    if (tuple.currencyFlag) {
        DecimalFormat2 fmtCopy(fmt);
        if (fmtCopy != fmt) {
            appendErrorMessage.append("Copy constructor equality mismatch.");
        } else {
            UErrorCode status = U_ZERO_ERROR;
            UnicodeString currency(tuple.currency);
            const UChar *terminatedCurrency = currency.getTerminatedBuffer();
            fmt.setCurrency(terminatedCurrency, status);
            if (U_FAILURE(status)) {
                appendErrorMessage.append("Error setting currency.");
            } else if (u_strcmp(fmt.getCurrency(), terminatedCurrency) != 0) {
                appendErrorMessage.append("Currency: get/set mismatch.");
            }
            if (u_strcmp(fmt.getCurrency(), fmtCopy.getCurrency()) != 0 && fmt == fmtCopy) {
                appendErrorMessage.append("Currency: set/equality mismatch.");
            }
        }
    }
    if (tuple.minGroupingDigitsFlag) {
        SET_AND_CHECK(
                fmt,
                MinimumGroupingDigits,
                tuple.minGroupingDigits,
                appendErrorMessage);
    }
    if (tuple.useSigDigitsFlag) {
        UBool newValue = tuple.useSigDigits != 0;
        SET_AND_CHECK_WITH_COND_LONG(
                fmt,
                SignificantDigitsUsed,
                setSignificantDigitsUsed,
                areSignificantDigitsUsed,
                newValue,
                TRUE,
                appendErrorMessage);
    }
    if (tuple.minSigDigitsFlag) {
        SET_AND_CHECK(
                fmt,
                MinimumSignificantDigits,
                tuple.minSigDigits,
                appendErrorMessage);
    }
    if (tuple.maxSigDigitsFlag) {
        SET_AND_CHECK(
                fmt,
                MaximumSignificantDigits,
                tuple.maxSigDigits,
                appendErrorMessage);
    }
    if (tuple.useGroupingFlag) {
        SET_AND_CHECK_BOOL(
                fmt,
                GroupingUsed,
                tuple.useGrouping != 0,
                appendErrorMessage);
    }
    if (tuple.multiplierFlag) {
        SET_AND_CHECK_WITH_COND(
                fmt,
                Multiplier,
                tuple.multiplier,
                tuple.multiplier != 0,
                appendErrorMessage);
    }
    if (tuple.roundingIncrementFlag) {
        SET_AND_CHECK(
                fmt,
                RoundingIncrement,
                tuple.roundingIncrement,
                appendErrorMessage);
    }
    if (tuple.formatWidthFlag) {
        SET_AND_CHECK(
                fmt,
                FormatWidth,
                tuple.formatWidth,
                appendErrorMessage);
    }
    if (tuple.padCharacterFlag && tuple.padCharacter.length() > 0) {
        SET_AND_CHECK(
                fmt,
                PadCharacter,
                tuple.padCharacter.char32At(0),
                appendErrorMessage);
    }
    if (tuple.useScientificFlag) {
        SET_AND_CHECK_BOOL(
                fmt,
                ScientificNotation,
                tuple.useScientific != 0,
                appendErrorMessage);
    }
    if (tuple.groupingFlag) {
        SET_AND_CHECK(
                fmt,
                GroupingSize,
                tuple.grouping,
                appendErrorMessage);
    }
    if (tuple.grouping2Flag) {
        SET_AND_CHECK(
                fmt,
                SecondaryGroupingSize,
                tuple.grouping2,
                appendErrorMessage);
    }
    if (tuple.roundingModeFlag) {
        SET_AND_CHECK(
                fmt,
                RoundingMode,
                convertRoundingMode(tuple.roundingMode),
                appendErrorMessage);
    }
    if (tuple.currencyUsageFlag) {
        SET_AND_CHECK_WITH_STATUS(
                fmt,
                CurrencyUsage,
                tuple.currencyUsage,
                appendErrorMessage);
    }
    if (tuple.minimumExponentDigitsFlag) {
        SET_AND_CHECK(
                fmt,
                MinimumExponentDigits,
                tuple.minimumExponentDigits,
                appendErrorMessage);
    }
    if (tuple.exponentSignAlwaysShownFlag) {
        SET_AND_CHECK_BOOL(
                fmt,
                ExponentSignAlwaysShown,
                tuple.exponentSignAlwaysShown != 0,
                appendErrorMessage);
    }
    if (tuple.decimalSeparatorAlwaysShownFlag) {
        SET_AND_CHECK_BOOL(
                fmt,
                DecimalSeparatorAlwaysShown,
                tuple.decimalSeparatorAlwaysShown != 0,
                appendErrorMessage);
    }
    if (tuple.padPositionFlag) {
        SET_AND_CHECK(
                fmt,
                PadPosition,
                convertPadPosition(tuple.padPosition),
                appendErrorMessage);
    }
    if (tuple.positivePrefixFlag) {
        fmt.setPositivePrefix(tuple.positivePrefix);
    }
    if (tuple.positiveSuffixFlag) {
        fmt.setPositiveSuffix(tuple.positiveSuffix);
    }
    if (tuple.negativePrefixFlag) {
        fmt.setNegativePrefix(tuple.negativePrefix);
    }
    if (tuple.negativeSuffixFlag) {
        fmt.setNegativeSuffix(tuple.negativeSuffix);
    }
    if (tuple.localizedPatternFlag) {
        UErrorCode status = U_ZERO_ERROR;
        fmt.applyLocalizedPattern(tuple.localizedPattern, status);
        if (U_FAILURE(status)) {
            appendErrorMessage.append("Error setting localized pattern.");
        }
    }
    SET_AND_CHECK_BOOL(
            fmt,
            Lenient,
            NFTT_GET_FIELD(tuple, lenient, 1) != 0,
            appendErrorMessage);
    if (tuple.parseIntegerOnlyFlag) {
        SET_AND_CHECK_BOOL(
                fmt,
                ParseIntegerOnly,
                tuple.parseIntegerOnly != 0,
                appendErrorMessage);
    }
    if (tuple.decimalPatternMatchRequiredFlag) {
        SET_AND_CHECK_BOOL(
                fmt,
                DecimalPatternMatchRequired,
                tuple.decimalPatternMatchRequired != 0,
                appendErrorMessage);
    }
    if (tuple.parseNoExponentFlag) {
        SET_AND_CHECK_BOOL(
                fmt,
                ParseNoExponent,
                tuple.parseNoExponent != 0,
                appendErrorMessage);
    }
}

static DecimalFormat2 *newDecimalFormat(
        const Locale &locale,
        const UnicodeString &pattern,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    LocalPointer<DecimalFormatSymbols> symbols(
            new DecimalFormatSymbols(locale, status), status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    UParseError perror;
    LocalPointer<DecimalFormat2> result(new DecimalFormat2(
            pattern, symbols.getAlias(), perror, status), status);
    if (!result.isNull()) {
        symbols.orphan();
    }
    if (U_FAILURE(status)) {
        return NULL;
    }
    return result.orphan();
}

static DecimalFormat2 *newDecimalFormat(
        const NumberFormatTestTuple &tuple,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    Locale en("en");
    return newDecimalFormat(
            NFTT_GET_FIELD(tuple, locale, en),
            NFTT_GET_FIELD(tuple, pattern, "0"),
            status);
}

UObject *NumberFormat2TestDataDriven::newFormatter(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    UnicodeString pattern("0");
    Locale en("en");
    return newDecimalFormat(en, pattern, status);
}

UBool NumberFormat2TestDataDriven::isFormatPass(
        const NumberFormatTestTuple &tuple,
        UObject *somePreviousFormatter, 
        UnicodeString &appendErrorMessage,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    LocalPointer<DecimalFormat2> fmtPtr(newDecimalFormat(tuple, status));
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error creating DecimalFormat.");
        return FALSE;
    }
    adjustDecimalFormat(tuple, *fmtPtr, appendErrorMessage);
    if (appendErrorMessage.length() > 0) {
        return FALSE;
    }
    DigitList digitList;
    strToDigitList(tuple.format, digitList, status);
    UnicodeString baseLine;
    format(*fmtPtr, digitList, baseLine, status);

    // Replace with copy from copy constructor
    DecimalFormat2 *temp = new DecimalFormat2(*fmtPtr);
    if (temp == NULL) {
        appendErrorMessage.append("Error creating copy constructor formatter.");
        return FALSE;
    }
    if (*fmtPtr != *temp) {
        appendErrorMessage.append("Copy constructor equality mismatch.");
        delete temp;
        return FALSE;
    }
    fmtPtr.adoptInstead(temp);
    UnicodeString copyFormatted;
    format(*fmtPtr, digitList, copyFormatted, status);
    DecimalFormat2 *assignFormatter = (DecimalFormat2 *) somePreviousFormatter;
    *assignFormatter = *fmtPtr;
    if (*fmtPtr != *assignFormatter) {
        appendErrorMessage.append("assignment equality mismatch.");
        return FALSE;
    }
    fmtPtr.adoptInstead(NULL);
    UnicodeString assignFormatted;
    format(*assignFormatter, digitList, assignFormatted, status);
    
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error formatting.");
        return FALSE;
    }
    if (baseLine != tuple.output) {
        appendErrorMessage.append(
                UnicodeString("Expected: ") + tuple.output + ", got: " + baseLine);
        return FALSE;
    }
    if (baseLine != copyFormatted) {
        appendErrorMessage.append(
                UnicodeString("Copy ctor failed had before: ") + baseLine + ", but got: " + copyFormatted);
        return FALSE;
    }
    if (baseLine != assignFormatted) {
        appendErrorMessage.append(
                UnicodeString("assignment failed had before: ") + baseLine + ", but got: " + assignFormatted);
        return FALSE;
    }
    return TRUE;
}

UBool NumberFormat2TestDataDriven::isToPatternPass(
        const NumberFormatTestTuple &tuple,
        UnicodeString &appendErrorMessage,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    LocalPointer<DecimalFormat2> fmtPtr(newDecimalFormat(tuple, status));
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error creating DecimalFormat.");
        return FALSE;
    }
    adjustDecimalFormat(tuple, *fmtPtr, appendErrorMessage);
    if (appendErrorMessage.length() > 0) {
        return FALSE;
    }
    if (tuple.toPatternFlag) {
        UnicodeString actual;
        fmtPtr->toPattern(actual);
        if (actual != tuple.toPattern) {
            appendErrorMessage.append(
                    UnicodeString("Expected: ") + tuple.toPattern + ", got: " + actual + ". ");
        }
    }
    return appendErrorMessage.length() == 0;
}

UBool NumberFormat2TestDataDriven::isSelectPass(
        const NumberFormatTestTuple &tuple,
        UnicodeString &appendErrorMessage,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    LocalPointer<DecimalFormat2> fmtPtr(newDecimalFormat(tuple, status));
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error creating DecimalFormat.");
        return FALSE;
    }
    adjustDecimalFormat(tuple, *fmtPtr, appendErrorMessage);
    if (appendErrorMessage.length() > 0) {
        return FALSE;
    }
    DigitList digitList;
    strToDigitList(tuple.format, digitList, status);
    Locale en("en");
    LocalPointer<PluralRules> rules(PluralRules::forLocale(
            NFTT_GET_FIELD(tuple, locale, en), status));
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error creating plural rules.");
        return FALSE;
    }
    UnicodeString actual = fmtPtr->select(digitList, *rules);
    if (actual != tuple.plural) {
        appendErrorMessage.append(
                UnicodeString("Expected: ") + tuple.plural + ", got: " + actual + ". ");
    }
    return appendErrorMessage.length() == 0;
}

UBool NumberFormat2TestDataDriven::isParseCurrencyPass(
        const NumberFormatTestTuple &tuple,
        UnicodeString &appendErrorMessage,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    LocalPointer<DecimalFormat2> fmtPtr(newDecimalFormat(tuple, status));
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error creating DecimalFormat.");
        return FALSE;
    }
    adjustDecimalFormat(tuple, *fmtPtr, appendErrorMessage);
    if (appendErrorMessage.length() > 0) {
        return FALSE;
    }
    ParsePosition ppos;
    LocalPointer<CurrencyAmount> currAmt(
            fmtPtr->parseCurrency(tuple.parse, ppos));
    if (ppos.getIndex() == 0) {
        if (tuple.output != "fail") {
            appendErrorMessage.append("Parse failed but was expected to succeed.");
            return FALSE;
        }
        return TRUE;
    }
    UnicodeString currStr(currAmt->getISOCurrency());
    Formattable resultFormattable(currAmt->getNumber());
    UnicodeString resultStr(UnicodeString::fromUTF8(resultFormattable.getDecimalNumber(status)));
    if (tuple.output == "fail") {
        appendErrorMessage.append(UnicodeString("Parse succeeded: ") + resultStr + ", but was expected to fail.");
        return FALSE;
    }
    DigitList expected;
    strToDigitList(tuple.output, expected, status);
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error parsing.");
        return FALSE;
    }
    if (expected != *currAmt->getNumber().getDigitList()) {
        appendErrorMessage.append(
                    UnicodeString("Expected: ") + tuple.output + ", got: " + resultStr + ". ");
        return FALSE;
    }
    if (currStr != tuple.outputCurrency) {
        appendErrorMessage.append(UnicodeString(
                "Expected currency: ") + tuple.outputCurrency + ", got: " + currStr + ". ");
        return FALSE;
    }
    return TRUE;
}

UBool NumberFormat2TestDataDriven::isParsePass(
        const NumberFormatTestTuple &tuple,
        UnicodeString &appendErrorMessage,
        UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    LocalPointer<DecimalFormat2> fmtPtr(newDecimalFormat(tuple, status));
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error creating DecimalFormat.");
        return FALSE;
    }
    adjustDecimalFormat(tuple, *fmtPtr, appendErrorMessage);
    if (appendErrorMessage.length() > 0) {
        return FALSE;
    }
    Formattable result;
    ParsePosition ppos;
    fmtPtr->parse(tuple.parse, result, ppos);
    if (ppos.getIndex() == 0) {
        if (tuple.output != "fail") {
            appendErrorMessage.append("Parse failed but was expected to succeed.");
            return FALSE;
        }
        return TRUE;
    }
    UnicodeString resultStr(UnicodeString::fromUTF8(result.getDecimalNumber(status)));
    if (tuple.output == "fail") {
        appendErrorMessage.append(UnicodeString("Parse succeeded: ") + resultStr + ", but was expected to fail.");
        return FALSE;
    }
    DigitList expected;
    strToDigitList(tuple.output, expected, status);
    if (U_FAILURE(status)) {
        appendErrorMessage.append("Error parsing.");
        return FALSE;
    }
    if (expected != *result.getDigitList()) {
        appendErrorMessage.append(
                    UnicodeString("Expected: ") + tuple.output + ", got: " + resultStr + ". ");
        return FALSE;
    }
    return TRUE;
}


class NumberFormat2Test : public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=0);
private:
    void TestQuantize();
    void TestConvertScientificNotation();
    void TestLowerUpperExponent();
    void TestRounding();
    void TestRoundingIncrement();
    void TestDigitInterval();
    void TestGroupingUsed();
    void TestBenchmark();
    void TestBenchmark2();
    void TestSmallIntFormatter();
    void TestPositiveIntDigitFormatter();
    void TestDigitListInterval();
    void TestDigitAffixesAndPadding();
    void TestPluralsAndRounding();
    void TestPluralsAndRoundingScientific();
    void TestValueFormatter();
    void TestValueFormatterIsFastFormattable();
    void TestValueFormatterScientific();
    void TestCurrencyAffixInfo();
    void TestAffixPattern();
    void TestAffixPatternAppend();
    void TestAffixPatternAppendAjoiningLiterals();
    void TestAffixPatternDoubleQuote();
    void TestAffixPatternParser();
    void TestPluralAffix();
    void TestDigitAffix();
    void TestDigitFormatterDefaultCtor();
    void TestDigitIntFormatter();
    void TestDigitFormatterMonetary();
    void TestDigitFormatter();
    void TestSciFormatterDefaultCtor();
    void TestSciFormatter();
    void TestDigitListToFixedDecimal();
    void TestGetAffixes();
    void TestApplyPatternResets();
    void TestDataDriven();
    void TestToPatternScientific11648();
    void verifyInterval(const DigitInterval &, int32_t minInclusive, int32_t maxExclusive);
    void verifyFixedDecimal(
            const FixedDecimal &result,
            int64_t numerator,
            int64_t denominator,
            UBool bNegative,
            int32_t v,
            int64_t f);
    void verifyAffix(
            const UnicodeString &expected,
            const DigitAffix &affix,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifyValueFormatter(
            const UnicodeString &expected,
            const ValueFormatter &formatter,
            DigitList &digits,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifyAffixesAndPadding(
            const UnicodeString &expected,
            const DigitAffixesAndPadding &aaf,
            DigitList &digits,
            const ValueFormatter &vf,
            const PluralRules *optPluralRules,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifyAffixesAndPaddingInt32(
            const UnicodeString &expected,
            const DigitAffixesAndPadding &aaf,
            int32_t value,
            const ValueFormatter &vf,
            const PluralRules *optPluralRules,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifyDigitList(
        const UnicodeString &expected,
        const DigitList &digits);
    void verifyDigitIntFormatter(
            const UnicodeString &expected,
            const DigitFormatter &formatter,
            int32_t value,
            int32_t minDigits,
            UBool alwaysShowSign,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifyDigitFormatter(
            const UnicodeString &expected,
            const DigitFormatter &formatter,
            const DigitList &digits,
            const DigitGrouping &grouping,
            const DigitInterval &interval,
            UBool alwaysShowDecimal,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifyDigitFormatter(
            const UnicodeString &expected,
            const DigitFormatter &formatter,
            const DigitList &digits,
            const DigitGrouping &grouping,
            const DigitInterval &interval,
            const DigitFormatterOptions &options,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifySciFormatter(
            const UnicodeString &expected,
            const SciFormatter &sciformatter,
            const DigitList &mantissa,
            int32_t exponent,
            const DigitFormatter &formatter,
            const DigitInterval &interval,
            const SciFormatterOptions &options,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifySmallIntFormatter(
            const UnicodeString &expected,
            int32_t positiveValue,
            int32_t minDigits,
            int32_t maxDigits);
    void verifyPositiveIntDigitFormatter(
            const UnicodeString &expected,
            const DigitFormatter &formatter,
            int32_t value,
            int32_t minDigits,
            int32_t maxDigits,
            const NumberFormat2Test_Attributes *expectedAttributes);
    void verifyAttributes(
            const NumberFormat2Test_Attributes *expected,
            const NumberFormat2Test_Attributes *actual);
};

void NumberFormat2Test::runIndexedTest(
        int32_t index, UBool exec, const char *&name, char *) {
    if (exec) {
        logln("TestSuite ScientificNumberFormatterTest: ");
    }
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(TestQuantize);
    TESTCASE_AUTO(TestConvertScientificNotation);
    TESTCASE_AUTO(TestLowerUpperExponent);
    TESTCASE_AUTO(TestRounding);
    TESTCASE_AUTO(TestRoundingIncrement);
    TESTCASE_AUTO(TestDigitInterval);
    TESTCASE_AUTO(TestGroupingUsed);
    TESTCASE_AUTO(TestDigitListInterval);
    TESTCASE_AUTO(TestDigitFormatterDefaultCtor);
    TESTCASE_AUTO(TestDigitIntFormatter);
    TESTCASE_AUTO(TestDigitFormatterMonetary);
    TESTCASE_AUTO(TestDigitFormatter);
    TESTCASE_AUTO(TestSciFormatterDefaultCtor);
    TESTCASE_AUTO(TestSciFormatter);
    TESTCASE_AUTO(TestBenchmark);
    TESTCASE_AUTO(TestBenchmark2);
    TESTCASE_AUTO(TestSmallIntFormatter);
    TESTCASE_AUTO(TestPositiveIntDigitFormatter);
    TESTCASE_AUTO(TestCurrencyAffixInfo);
    TESTCASE_AUTO(TestAffixPattern);
    TESTCASE_AUTO(TestAffixPatternAppend);
    TESTCASE_AUTO(TestAffixPatternAppendAjoiningLiterals);
    TESTCASE_AUTO(TestAffixPatternDoubleQuote);
    TESTCASE_AUTO(TestAffixPatternParser);
    TESTCASE_AUTO(TestPluralAffix);
    TESTCASE_AUTO(TestDigitAffix);
    TESTCASE_AUTO(TestValueFormatter);
    TESTCASE_AUTO(TestValueFormatterIsFastFormattable);
    TESTCASE_AUTO(TestValueFormatterScientific);
    TESTCASE_AUTO(TestDigitAffixesAndPadding);
    TESTCASE_AUTO(TestPluralsAndRounding);
    TESTCASE_AUTO(TestPluralsAndRoundingScientific);
    TESTCASE_AUTO(TestDigitListToFixedDecimal);
    TESTCASE_AUTO(TestGetAffixes);
    TESTCASE_AUTO(TestApplyPatternResets);
    TESTCASE_AUTO(TestDataDriven);
    TESTCASE_AUTO(TestToPatternScientific11648);
 
    TESTCASE_AUTO_END;
}

void NumberFormat2Test::TestDigitInterval() {
    DigitInterval all;
    DigitInterval threeInts;
    DigitInterval fourFrac;
    threeInts.setIntDigitCount(3);
    fourFrac.setFracDigitCount(4);
    verifyInterval(all, INT32_MIN, INT32_MAX);
    verifyInterval(threeInts, INT32_MIN, 3);
    verifyInterval(fourFrac, -4, INT32_MAX);
    {
        DigitInterval result(threeInts);
        result.shrinkToFitWithin(fourFrac);
        verifyInterval(result, -4, 3);
        assertEquals("", 7, result.length());
    }
    {
        DigitInterval result(threeInts);
        result.expandToContain(fourFrac);
        verifyInterval(result, INT32_MIN, INT32_MAX);
    }
    {
        DigitInterval result(threeInts);
        result.setIntDigitCount(0);
        verifyInterval(result, INT32_MIN, 0);
        result.setIntDigitCount(-1);
        verifyInterval(result, INT32_MIN, INT32_MAX);
    }
    {
        DigitInterval result(fourFrac);
        result.setFracDigitCount(0);
        verifyInterval(result, 0, INT32_MAX);
        result.setFracDigitCount(-1);
        verifyInterval(result, INT32_MIN, INT32_MAX);
    }
    {
        DigitInterval result;
        result.setIntDigitCount(3);
        result.setFracDigitCount(1);
        result.expandToContainDigit(0);
        result.expandToContainDigit(-1);
        result.expandToContainDigit(2);
        verifyInterval(result, -1, 3);
        result.expandToContainDigit(3);
        verifyInterval(result, -1, 4);
        result.expandToContainDigit(-2);
        verifyInterval(result, -2, 4);
        result.expandToContainDigit(15);
        result.expandToContainDigit(-15);
        verifyInterval(result, -15, 16);
    }
    {
        DigitInterval result;
        result.setIntDigitCount(3);
        result.setFracDigitCount(1);
        assertTrue("", result.contains(2));
        assertTrue("", result.contains(-1));
        assertFalse("", result.contains(3));
        assertFalse("", result.contains(-2));
    }
}

void NumberFormat2Test::verifyInterval(
        const DigitInterval &interval,
        int32_t minInclusive, int32_t maxExclusive) {
    assertEquals("", minInclusive, interval.getLeastSignificantInclusive());
    assertEquals("", maxExclusive, interval.getMostSignificantExclusive());
    assertEquals("", maxExclusive, interval.getIntDigitCount());
}

void NumberFormat2Test::TestGroupingUsed() {
    {
        DigitGrouping grouping;
        assertFalse("", grouping.isGroupingUsed());
    }
    {
        DigitGrouping grouping;
        grouping.fGrouping = 2;
        assertTrue("", grouping.isGroupingUsed());
    }
}

void NumberFormat2Test::TestDigitListInterval() {
    DigitInterval result;
    DigitList digitList;
    {
        digitList.set(12345);
        verifyInterval(digitList.getSmallestInterval(result), 0, 5);
    }
    {
        digitList.set(1000.00);
        verifyInterval(digitList.getSmallestInterval(result), 0, 4);
    }
    {
        digitList.set(43.125);
        verifyInterval(digitList.getSmallestInterval(result), -3, 2);
    }
    {
        digitList.set(.0078125);
        verifyInterval(digitList.getSmallestInterval(result), -7, 0);
    }
    {
        digitList.set(1000.00);
        digitList.getSmallestInterval(result);
        result.expandToContainDigit(3);
        verifyInterval(result, 0, 4);
    }
    {
        digitList.set(1000.00);
        digitList.getSmallestInterval(result);
        result.expandToContainDigit(4);
        verifyInterval(result, 0, 5);
    }
    {
        digitList.set(1000.00);
        digitList.getSmallestInterval(result);
        result.expandToContainDigit(0);
        verifyInterval(result, 0, 4);
    }
    {
        digitList.set(1000.00);
        digitList.getSmallestInterval(result);
        result.expandToContainDigit(-1);
        verifyInterval(result, -1, 4);
    }
    {
        digitList.set(43.125);
        digitList.getSmallestInterval(result);
        result.expandToContainDigit(1);
        verifyInterval(result, -3, 2);
    }
    {
        digitList.set(43.125);
        digitList.getSmallestInterval(result);
        result.expandToContainDigit(2);
        verifyInterval(result, -3, 3);
    }
    {
        digitList.set(43.125);
        digitList.getSmallestInterval(result);
        result.expandToContainDigit(-3);
        verifyInterval(result, -3, 2);
    }
    {
        digitList.set(43.125);
        digitList.getSmallestInterval(result);
        result.expandToContainDigit(-4);
        verifyInterval(result, -4, 2);
    }
}

void NumberFormat2Test::TestQuantize() {
    DigitList quantity;
    quantity.set(0.00168);
    quantity.roundAtExponent(-5);
    DigitList digits;
    UErrorCode status = U_ZERO_ERROR;
    {
        digits.set(1);
        digits.quantize(quantity, status);
        verifyDigitList(".9996", digits);
    }
    {
        // round half even up
        digits.set(1.00044);
        digits.roundAtExponent(-5);
        digits.quantize(quantity, status);
        verifyDigitList("1.00128", digits);
    }
    {
        // round half down
        digits.set(0.99876);
        digits.roundAtExponent(-5);
        digits.quantize(quantity, status);
        verifyDigitList(".99792", digits);
    }
}

void NumberFormat2Test::TestConvertScientificNotation() {
    DigitList digits;
    {
        digits.set(186283);
        assertEquals("", 5, digits.toScientific(1, 1));
        verifyDigitList(
                "1.86283",
                digits);
    }
    {
        digits.set(186283);
        assertEquals("", 0, digits.toScientific(6, 1));
        verifyDigitList(
                "186283",
                digits);
    }
    {
        digits.set(186283);
        assertEquals("", -2, digits.toScientific(8, 1));
        verifyDigitList(
                "18628300",
                digits);
    }
    {
        digits.set(43561);
        assertEquals("", 6, digits.toScientific(-1, 3));
        verifyDigitList(
                ".043561",
                digits);
    }
    {
        digits.set(43561);
        assertEquals("", 3, digits.toScientific(0, 3));
        verifyDigitList(
                "43.561",
                digits);
    }
    {
        digits.set(43561);
        assertEquals("", 3, digits.toScientific(2, 3));
        verifyDigitList(
                "43.561",
                digits);
    }
    {
        digits.set(43561);
        assertEquals("", 0, digits.toScientific(3, 3));
        verifyDigitList(
                "43561",
                digits);
    }
    {
        digits.set(43561);
        assertEquals("", 0, digits.toScientific(5, 3));
        verifyDigitList(
                "43561",
                digits);
    }
    {
        digits.set(43561);
        assertEquals("", -3, digits.toScientific(6, 3));
        verifyDigitList(
                "43561000",
                digits);
    }
    {
        digits.set(43561);
        assertEquals("", -3, digits.toScientific(8, 3));
        verifyDigitList(
                "43561000",
                digits);
    }
    {
        digits.set(43561);
        assertEquals("", -6, digits.toScientific(9, 3));
        verifyDigitList(
                "43561000000",
                digits);
    }
}

void NumberFormat2Test::TestLowerUpperExponent() {
    DigitList digits;

    digits.set(98.7);
    assertEquals("", -1, digits.getLowerExponent());
    assertEquals("", 2, digits.getUpperExponent());
}

void NumberFormat2Test::TestRounding() {
    DigitList digits;
    uprv_decContextSetRounding(&digits.fContext, DEC_ROUND_CEILING);
    {
        // Round at very large exponent
        digits.set(789.123);
        digits.roundAtExponent(100);
        verifyDigitList(
                "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", // 100 0's after 1
                digits);
    }
    {
        // Round at very large exponent
        digits.set(789.123);
        digits.roundAtExponent(1);
        verifyDigitList(
                "790", // 100 0's after 1
                digits);
    }
    {
        // Round at positive exponent
        digits.set(789.123);
        digits.roundAtExponent(1);
        verifyDigitList("790", digits);
    }
    {
        // Round at zero exponent
        digits.set(788.123);
        digits.roundAtExponent(0);
        verifyDigitList("789", digits);
    }
    {
        // Round at negative exponent
        digits.set(789.123);
        digits.roundAtExponent(-2);
        verifyDigitList("789.13", digits);
    }
    {
        // Round to exponent of digits.
        digits.set(789.123);
        digits.roundAtExponent(-3);
        verifyDigitList("789.123", digits);
    }
    {
        // Round at large negative exponent
        digits.set(789.123);
        digits.roundAtExponent(-100);
        verifyDigitList("789.123", digits);
    }
    {
        // Round negative
        digits.set(-789.123);
        digits.roundAtExponent(-2);
        digits.setPositive(TRUE);
        verifyDigitList("789.12", digits);
    }
    {
        // Round to 1 significant digit
        digits.set(789.123);
        digits.roundAtExponent(INT32_MIN, 1);
        verifyDigitList("800", digits);
    }
    {
        // Round to 5 significant digit
        digits.set(789.123);
        digits.roundAtExponent(INT32_MIN, 5);
        verifyDigitList("789.13", digits);
    }
    {
        // Round to 6 significant digit
        digits.set(789.123);
        digits.roundAtExponent(INT32_MIN, 6);
        verifyDigitList("789.123", digits);
    }
    {
        // no-op
        digits.set(789.123);
        digits.roundAtExponent(INT32_MIN, INT32_MAX);
        verifyDigitList("789.123", digits);
    }
    {
        // Rounding at -1 produces fewer than 5 significant digits
        digits.set(789.123);
        digits.roundAtExponent(-1, 5);
        verifyDigitList("789.2", digits);
    }
    {
        // Rounding at -1 produces exactly 4 significant digits
        digits.set(789.123);
        digits.roundAtExponent(-1, 4);
        verifyDigitList("789.2", digits);
    }
    {
        // Rounding at -1 produces more than 3 significant digits
        digits.set(788.123);
        digits.roundAtExponent(-1, 3);
        verifyDigitList("789", digits);
    }
    {
        digits.set(123.456);
        digits.round(INT32_MAX);
        verifyDigitList("123.456", digits);
    }
    {
        digits.set(123.456);
        digits.round(1);
        verifyDigitList("200", digits);
    }
}
void NumberFormat2Test::TestBenchmark() {
/*
    UErrorCode status = U_ZERO_ERROR;
    Locale en("en");
    DecimalFormatSymbols *sym = new DecimalFormatSymbols(en, status);
    DecimalFormat2 fmt(en, "0.0000000", status);
    FieldPosition fpos(0);
    clock_t start = clock();
    for (int32_t i = 0; i < 100000; ++i) {
       UParseError perror;
       DecimalFormat2 fmt2("0.0000000", new DecimalFormatSymbols(*sym), perror, status);
//       UnicodeString append;
//       fmt.format(4.6692016, append, fpos, status);
    }
    errln("Took %f", (double) (clock() - start) / CLOCKS_PER_SEC);
    assertSuccess("", status);
*/
}

void NumberFormat2Test::TestBenchmark2() {
/*
    UErrorCode status = U_ZERO_ERROR;
    Locale en("en");
    DecimalFormatSymbols *sym = new DecimalFormatSymbols(en, status);
    DecimalFormat fmt("0.0000000", sym, status);
    FieldPosition fpos(0);
    clock_t start = clock();
    for (int32_t i = 0; i < 100000; ++i) {
      UParseError perror;
      DecimalFormat fmt("0.0000000", new DecimalFormatSymbols(*sym), perror, status);
//        UnicodeString append;
//        fmt.format(4.6692016, append, fpos, status);
    }
    errln("Took %f", (double) (clock() - start) / CLOCKS_PER_SEC);
    assertSuccess("", status);
*/
}

void NumberFormat2Test::TestSmallIntFormatter() {
    verifySmallIntFormatter("0", 7, 0, -2);
    verifySmallIntFormatter("7", 7, 1, -2);
    verifySmallIntFormatter("07", 7, 2, -2);
    verifySmallIntFormatter("07", 7, 2, 2);
    verifySmallIntFormatter("007", 7, 3, 4);
    verifySmallIntFormatter("7", 7, -1, 3);
    verifySmallIntFormatter("0", 0, -1, 3);
    verifySmallIntFormatter("057", 57, 3, 7);
    verifySmallIntFormatter("0057", 57, 4, 7);
    // too many digits for small int
    verifySmallIntFormatter("", 57, 5, 7);
    // too many digits for small int
    verifySmallIntFormatter("", 57, 5, 4);
    verifySmallIntFormatter("03", 3, 2, 3);
    verifySmallIntFormatter("32", 32, 2, 3);
    verifySmallIntFormatter("321", 321, 2, 3);
    verifySmallIntFormatter("219", 3219, 2, 3);
    verifySmallIntFormatter("4095", 4095, 2, 4);
    verifySmallIntFormatter("4095", 4095, 2, 5);
    verifySmallIntFormatter("", 4096, 2, 5);
}

void NumberFormat2Test::TestPositiveIntDigitFormatter() {
    DigitFormatter formatter;
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 4},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "0057",
                formatter,
                57,
                4,
                INT32_MAX,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 5},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "00057",
                formatter,
                57,
                5,
                INT32_MAX,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 5},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "01000",
                formatter,
                1000,
                5,
                INT32_MAX,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 3},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "100",
                formatter,
                100,
                0,
                INT32_MAX,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 10},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "2147483647",
                formatter,
                2147483647,
                5,
                INT32_MAX,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 12},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "002147483647",
                formatter,
                2147483647,
                12,
                INT32_MAX,
                expectedAttributes);
    }
    {
        // Test long digit string where we have to append one
        // character at a time.
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 40},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "0000000000000000000000000000002147483647",
                formatter,
                2147483647,
                40,
                INT32_MAX,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 4},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "6283",
                formatter,
                186283,
                2,
                4,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "0",
                formatter,
                186283,
                0,
                0,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {0, -1, 0}};
        verifyPositiveIntDigitFormatter(
                "3",
                formatter,
                186283,
                1,
                1,
                expectedAttributes);
    }
}


void NumberFormat2Test::TestDigitFormatterDefaultCtor() {
    DigitFormatter formatter;
    DigitList digits;
    DigitInterval interval;
    DigitGrouping grouping;
    digits.set(246.801);
    verifyDigitFormatter(
            "246.801",
            formatter,
            digits,
            grouping,
            digits.getSmallestInterval(interval),
            FALSE,
            NULL);
    verifyDigitIntFormatter(
            "+023",
            formatter,
            23,
            3,
            TRUE,
            NULL);
}

void NumberFormat2Test::TestDigitIntFormatter() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    DigitFormatter formatter(symbols);
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kIntField, 0, 1},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "0",
                formatter,
                0,
                0,
                FALSE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kSignField, 0, 1},
            {kIntField, 1, 2},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "+0",
                formatter,
                0,
                0,
                TRUE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kIntField, 0, 1},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "0",
                formatter,
                0,
                1,
                FALSE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kSignField, 0, 1},
            {kIntField, 1, 2},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "+0",
                formatter,
                0,
                1,
                TRUE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kSignField, 0, 1},
            {kIntField, 1, 2},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "-2",
                formatter,
                -2,
                1,
                FALSE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kSignField, 0, 1},
            {kIntField, 1, 3},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "-02",
                formatter,
                -2,
                2,
                FALSE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kSignField, 0, 1},
            {kIntField, 1, 3},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "-02",
                formatter,
                -2,
                2,
                TRUE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kSignField, 0, 1},
            {kIntField, 1, 11},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "-2147483648",
                formatter,
                INT32_MIN,
                1,
                FALSE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kSignField, 0, 1},
            {kIntField, 1, 11},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "+2147483647",
                formatter,
                INT32_MAX,
                1,
                TRUE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kIntField, 0, 12},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "002147483647",
                formatter,
                INT32_MAX,
                12,
                FALSE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kIntField, 0, 9},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "007654321",
                formatter,
                7654321,
                9,
                FALSE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kSignField, 0, 1},
            {kIntField, 1, 10},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "-007654321",
                formatter,
                -7654321,
                9,
                FALSE,
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {kIntField, 0, 3},
            {0, -1, 0}};
        verifyDigitIntFormatter(
                "100",
                formatter,
                100,
                0,
                FALSE,
                expectedAttributes);
    }
}

void NumberFormat2Test::TestDigitFormatterMonetary() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    symbols.setSymbol(
            DecimalFormatSymbols::kMonetarySeparatorSymbol,
            "decimal separator");
    symbols.setSymbol(
            DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol,
            "grouping separator");
    DigitFormatter formatter(symbols);
    DigitList digits;
    DigitInterval interval;
    DigitGrouping grouping;
    DigitFormatterOptions options;
    grouping.fGrouping = 3;
    digits.set(43560.02);
    {
        verifyDigitFormatter(
                "43,560.02",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                options,
                NULL);
        formatter.setDecimalFormatSymbolsForMonetary(symbols);
        verifyDigitFormatter(
                "43grouping separator560decimal separator02",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                options,
                NULL);
    }
}

void NumberFormat2Test::TestDigitFormatter() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    DigitFormatter formatter(symbols);
    DigitList digits;
    DigitInterval interval;
    {
        DigitGrouping grouping;
        digits.set(8192);
        verifyDigitFormatter(
                "8192",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                FALSE,
                NULL);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 4},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 4, 5},
            {0, -1, 0}};
        verifyDigitFormatter(
                "8192.",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                TRUE,
                expectedAttributes);

        // Turn on grouping
        grouping.fGrouping = 3;
        verifyDigitFormatter(
                "8,192",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                FALSE,
                NULL);

        // turn on min grouping which will suppress grouping
        grouping.fMinGrouping = 2;
        verifyDigitFormatter(
                "8192",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                FALSE,
                NULL);

        // adding one more digit will enable grouping once again.
        digits.set(43560);
        verifyDigitFormatter(
                "43,560",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                FALSE,
                NULL);
    }
    {
        DigitGrouping grouping;
        digits.set(31415926.0078125);
        verifyDigitFormatter(
                "31415926.0078125",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                FALSE,
                NULL);

        // Turn on grouping with secondary.
        grouping.fGrouping = 2;
        grouping.fGrouping2 = 3;
        verifyDigitFormatter(
                "314,159,26.0078125",
                formatter,
                digits,
                grouping,
                digits.getSmallestInterval(interval),
                FALSE,
                NULL);

        // Pad with zeros by widening interval.
        interval.setIntDigitCount(9);
        interval.setFracDigitCount(10);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_GROUPING_SEPARATOR_FIELD, 1, 2},
            {UNUM_GROUPING_SEPARATOR_FIELD, 5, 6},
            {UNUM_GROUPING_SEPARATOR_FIELD, 9, 10},
            {UNUM_INTEGER_FIELD, 0, 12},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 12, 13},
            {UNUM_FRACTION_FIELD, 13, 23},
            {0, -1, 0}};
        verifyDigitFormatter(
                "0,314,159,26.0078125000",
                formatter,
                digits,
                grouping,
                interval,
                FALSE,
                expectedAttributes);
    }
    {
        DigitGrouping grouping;
        digits.set(3125);
        interval.setIntDigitCount(0);
        interval.setFracDigitCount(0);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {0, -1, 0}};
        verifyDigitFormatter(
                "0",
                formatter,
                digits,
                grouping,
                interval,
                FALSE,
                expectedAttributes);
    }
    {
        DigitGrouping grouping;
        digits.set(3125);
        interval.setIntDigitCount(0);
        interval.setFracDigitCount(0);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 1, 2},
            {0, -1, 0}};
        verifyDigitFormatter(
                "0.",
                formatter,
                digits,
                grouping,
                interval,
                TRUE,
                expectedAttributes);
    }
    {
        DigitGrouping grouping;
        digits.set(3125);
        interval.setIntDigitCount(1);
        interval.setFracDigitCount(1);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 1, 2},
            {UNUM_FRACTION_FIELD, 2, 3},
            {0, -1, 0}};
        verifyDigitFormatter(
                "5.0",
                formatter,
                digits,
                grouping,
                interval,
                TRUE,
                expectedAttributes);
    }
}

void NumberFormat2Test::TestSciFormatterDefaultCtor() {
    SciFormatter sciformatter;
    DigitFormatter formatter;
    DigitList mantissa;
    DigitInterval interval;
    SciFormatterOptions options;
    {
        mantissa.set(6.02);
        verifySciFormatter(
                "6.02E23",
                sciformatter,
                mantissa,
                23,
                formatter,
                mantissa.getSmallestInterval(interval),
                options,
                NULL);
    }
    {
        mantissa.set(6.62);
        verifySciFormatter(
                "6.62E-34",
                sciformatter,
                mantissa,
                -34,
                formatter,
                mantissa.getSmallestInterval(interval),
                options,
                NULL);
    }
}

void NumberFormat2Test::TestSciFormatter() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    SciFormatter sciformatter(symbols);
    DigitFormatter formatter(symbols);
    DigitList mantissa;
    DigitInterval interval;
    SciFormatterOptions options;
    options.fExponent.fMinDigits = 3;
    {
        options.fExponent.fAlwaysShowSign = TRUE;
        mantissa.set(1248);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 4},
            {UNUM_EXPONENT_SYMBOL_FIELD, 4, 5},
            {UNUM_EXPONENT_SIGN_FIELD, 5, 6},
            {UNUM_EXPONENT_FIELD, 6, 9},
            {0, -1, 0}};
        verifySciFormatter(
                "1248E+023",
                sciformatter,
                mantissa,
                23,
                formatter,
                mantissa.getSmallestInterval(interval),
                options,
                expectedAttributes);
    }
    {
        options.fMantissa.fAlwaysShowDecimal = TRUE;
        options.fExponent.fAlwaysShowSign = FALSE;
        mantissa.set(1248);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 4},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 4, 5},
            {UNUM_EXPONENT_SYMBOL_FIELD, 5, 6},
            {UNUM_EXPONENT_FIELD, 6, 9},
            {0, -1, 0}};
        verifySciFormatter(
                "1248.E023",
                sciformatter,
                mantissa,
                23,
                formatter,
                mantissa.getSmallestInterval(interval),
                options,
                expectedAttributes);
    }
}

void NumberFormat2Test::TestValueFormatter() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    DigitFormatter formatter(symbols);
    DigitGrouping grouping;
    grouping.fGrouping = 3;
    FixedPrecision precision;
    precision.fMin.setIntDigitCount(4);
    precision.fMin.setFracDigitCount(2);
    precision.fMax.setIntDigitCount(6);
    precision.fMax.setFracDigitCount(4);
    DigitFormatterOptions options;
    ValueFormatter vf;
    vf.prepareFixedDecimalFormatting(
            formatter,
            grouping,
            precision,
            options);
    DigitList digits;
    {
        digits.set(3.49951);
        verifyValueFormatter(
                "0,003.4995",
                vf,
                digits,
                NULL);
    }
    {
        digits.set(3.499951);
        verifyValueFormatter(
                "0,003.50",
                vf,
                digits,
                NULL);
    }
    {
        digits.set(1234567.89008);
        verifyValueFormatter(
                "234,567.8901",
                vf,
                digits,
                NULL);
    }
    // significant digits too
    precision.fSignificant.setMin(3);
    precision.fSignificant.setMax(4);
    {
        digits.set(342.562);
        verifyValueFormatter(
                "0,342.60",
                vf,
                digits,
                NULL);
    }
    {
        digits.set(0.57);
        verifyValueFormatter(
                "0,000.570",
                vf,
                digits,
                NULL);
    }
}

void NumberFormat2Test::TestValueFormatterIsFastFormattable() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    DigitFormatter formatter(symbols);
    DigitGrouping grouping;
    FixedPrecision precision;
    DigitFormatterOptions options;
    ValueFormatter vf;
    vf.prepareFixedDecimalFormatting(
            formatter, grouping, precision, options);
    assertTrue("", vf.isFastFormattable(0));
    assertTrue("", vf.isFastFormattable(35));
    assertTrue("", vf.isFastFormattable(-48));
    assertTrue("", vf.isFastFormattable(2147483647));
    assertTrue("", vf.isFastFormattable(-2147483647));
    assertFalse("", vf.isFastFormattable(-2147483648));
    {
        DigitGrouping grouping;
        grouping.fGrouping = 3;
        ValueFormatter vf;
        vf.prepareFixedDecimalFormatting(
                formatter, grouping, precision, options);
        assertTrue("0", vf.isFastFormattable(0));
        assertTrue("62", vf.isFastFormattable(62));
        assertTrue("999", vf.isFastFormattable(999));
        assertFalse("1000", vf.isFastFormattable(1000));
        assertTrue("-1", vf.isFastFormattable(-1));
        assertTrue("-38", vf.isFastFormattable(-38));
        assertTrue("-999", vf.isFastFormattable(-999));
        assertFalse("-1000", vf.isFastFormattable(-1000));
        grouping.fMinGrouping = 2;
        assertTrue("-1000", vf.isFastFormattable(-1000));
        assertTrue("-4095", vf.isFastFormattable(-4095));
        assertTrue("4095", vf.isFastFormattable(4095));
        // We give up on acounting digits at 4096
        assertFalse("-4096", vf.isFastFormattable(-4096));
        assertFalse("4096", vf.isFastFormattable(4096));
    }
    {
        // grouping on but with max integer digits set.
        DigitGrouping grouping;
        grouping.fGrouping = 4;
        FixedPrecision precision;
        precision.fMax.setIntDigitCount(4);
        ValueFormatter vf;
        vf.prepareFixedDecimalFormatting(
                formatter, grouping, precision, options);
        assertTrue("-4096", vf.isFastFormattable(-4096));
        assertTrue("4096", vf.isFastFormattable(4096));
        assertTrue("-10000", vf.isFastFormattable(-10000));
        assertTrue("10000", vf.isFastFormattable(10000));
        assertTrue("-2147483647", vf.isFastFormattable(-2147483647));
        assertTrue("2147483647", vf.isFastFormattable(2147483647));

        precision.fMax.setIntDigitCount(5);
        assertFalse("-4096", vf.isFastFormattable(-4096));
        assertFalse("4096", vf.isFastFormattable(4096));

    }
    {
        // grouping on but with min integer digits set.
        DigitGrouping grouping;
        grouping.fGrouping = 3;
        FixedPrecision precision;
        precision.fMin.setIntDigitCount(3);
        ValueFormatter vf;
        vf.prepareFixedDecimalFormatting(
                formatter, grouping, precision, options);
        assertTrue("-999", vf.isFastFormattable(-999));
        assertTrue("999", vf.isFastFormattable(999));
        assertFalse("-1000", vf.isFastFormattable(-1000));
        assertFalse("1000", vf.isFastFormattable(1000));

        precision.fMin.setIntDigitCount(4);
        assertFalse("-999", vf.isFastFormattable(-999));
        assertFalse("999", vf.isFastFormattable(999));
        assertFalse("-2147483647", vf.isFastFormattable(-2147483647));
        assertFalse("2147483647", vf.isFastFormattable(2147483647));
    }
    {
        // options set.
        DigitFormatterOptions options;
        ValueFormatter vf;
        vf.prepareFixedDecimalFormatting(
                formatter, grouping, precision, options);
        assertTrue("5125", vf.isFastFormattable(5125));
        options.fAlwaysShowDecimal = TRUE;
        assertFalse("5125", vf.isFastFormattable(5125));
        options.fAlwaysShowDecimal = FALSE;
        assertTrue("5125", vf.isFastFormattable(5125));
    }
    {
        // test fraction digits
        FixedPrecision precision;
        ValueFormatter vf;
        vf.prepareFixedDecimalFormatting(
                formatter, grouping, precision, options);
        assertTrue("7127", vf.isFastFormattable(7127));
        precision.fMin.setFracDigitCount(1);
        assertFalse("7127", vf.isFastFormattable(7127));
    }
    {
        // test presence of significant digits
        FixedPrecision precision;
        ValueFormatter vf;
        vf.prepareFixedDecimalFormatting(
                formatter, grouping, precision, options);
        assertTrue("1049", vf.isFastFormattable(1049));
        precision.fSignificant.setMin(1);
        assertFalse("1049", vf.isFastFormattable(1049));
    }
    {
        // test presence of rounding increment
        FixedPrecision precision;
        ValueFormatter vf;
        vf.prepareFixedDecimalFormatting(
                formatter, grouping, precision, options);
        assertTrue("1099", vf.isFastFormattable(1099));
        precision.fRoundingIncrement.set(2.3);
        assertFalse("1099", vf.isFastFormattable(1099));
    }
    {
        // test scientific notation
        ScientificPrecision precision;
        SciFormatter sciformatter;
        SciFormatterOptions options;
        ValueFormatter vf;
        vf.prepareScientificFormatting(
                sciformatter, formatter, precision, options);
        assertFalse("1081", vf.isFastFormattable(1081));
    }
}

void NumberFormat2Test::TestValueFormatterScientific() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    SciFormatter sciformatter(symbols);
    DigitFormatter formatter(symbols);
    ScientificPrecision precision;
    precision.fMantissa.fSignificant.setMax(3);
    SciFormatterOptions options;
    ValueFormatter vf;
    vf.prepareScientificFormatting(
            sciformatter,
            formatter,
            precision,
            options);
    DigitList digits;
    {
        digits.set(43560);
        verifyValueFormatter(
                "4.36E4",
                vf,
                digits,
                NULL);
    }
    {
        digits.set(43560);
        precision.fMantissa.fMax.setIntDigitCount(3);
        verifyValueFormatter(
                "43.6E3",
                vf,
                digits,
                NULL);
    }
    {
        digits.set(43560);
        precision.fMantissa.fMin.setIntDigitCount(3);
        verifyValueFormatter(
                "436E2",
                vf,
                digits,
                NULL);
    }
    {
        digits.set(43560);
        options.fExponent.fAlwaysShowSign = TRUE;
        options.fExponent.fMinDigits = 2;
        verifyValueFormatter(
                "436E+02",
                vf,
                digits,
                NULL);
    }

}

void NumberFormat2Test::TestDigitAffix() {
    DigitAffix affix;
    {
        affix.append("foo");
        affix.append("--", UNUM_SIGN_FIELD);
        affix.append("%", UNUM_PERCENT_FIELD);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 3, 5},
            {UNUM_PERCENT_FIELD, 5, 6},
            {0, -1, 0}};
        verifyAffix("foo--%", affix, expectedAttributes);
    }
    {
        affix.remove();
        affix.append("USD", UNUM_CURRENCY_FIELD);
        affix.append(" ");
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_CURRENCY_FIELD, 0, 3},
            {0, -1, 0}};
        verifyAffix("USD ", affix, expectedAttributes);
    }
}

void NumberFormat2Test::TestPluralAffix() {
    UErrorCode status = U_ZERO_ERROR;
    PluralAffix part;
    part.setVariant("one", "Dollar", status);
    part.setVariant("few", "DollarFew", status);
    part.setVariant("other", "Dollars", status);
    PluralAffix dollar(part);
    PluralAffix percent(part);
    part.remove();
    part.setVariant("one", "Percent", status);
    part.setVariant("many", "PercentMany", status);
    part.setVariant("other", "Percents", status);
    percent = part;
    part.remove();
    part.setVariant("one", "foo", status);

    PluralAffix pa;
    assertEquals("", "", pa.getOtherVariant().toString());
    pa.append(dollar, UNUM_CURRENCY_FIELD, status);
    pa.append(" and ");
    pa.append(percent, UNUM_PERCENT_FIELD, status);
    pa.append("-", UNUM_SIGN_FIELD);

    {
        // other
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_CURRENCY_FIELD, 0, 7},
            {UNUM_PERCENT_FIELD, 12, 20},
            {UNUM_SIGN_FIELD, 20, 21},
            {0, -1, 0}};
        verifyAffix(
                "Dollars and Percents-",
                pa.getByVariant("other"),
                expectedAttributes);
    }
    {
        // two which is same as other
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_CURRENCY_FIELD, 0, 7},
            {UNUM_PERCENT_FIELD, 12, 20},
            {UNUM_SIGN_FIELD, 20, 21},
            {0, -1, 0}};
        verifyAffix(
                "Dollars and Percents-",
                pa.getByVariant("two"),
                expectedAttributes);
    }
    {
        // bad which is same as other
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_CURRENCY_FIELD, 0, 7},
            {UNUM_PERCENT_FIELD, 12, 20},
            {UNUM_SIGN_FIELD, 20, 21},
            {0, -1, 0}};
        verifyAffix(
                "Dollars and Percents-",
                pa.getByVariant("bad"),
                expectedAttributes);
    }
    {
        // one
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_CURRENCY_FIELD, 0, 6},
            {UNUM_PERCENT_FIELD, 11, 18},
            {UNUM_SIGN_FIELD, 18, 19},
            {0, -1, 0}};
        verifyAffix(
                "Dollar and Percent-",
                pa.getByVariant("one"),
                expectedAttributes);
    }
    {
        // few
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_CURRENCY_FIELD, 0, 9},
            {UNUM_PERCENT_FIELD, 14, 22},
            {UNUM_SIGN_FIELD, 22, 23},
            {0, -1, 0}};
        verifyAffix(
                "DollarFew and Percents-",
                pa.getByVariant("few"),
                expectedAttributes);
    }
    {
        // many
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_CURRENCY_FIELD, 0, 7},
            {UNUM_PERCENT_FIELD, 12, 23},
            {UNUM_SIGN_FIELD, 23, 24},
            {0, -1, 0}};
        verifyAffix(
                "Dollars and PercentMany-",
                pa.getByVariant("many"),
                expectedAttributes);
    }
    assertTrue("", pa.hasMultipleVariants());
    pa.remove();
    pa.append("$$$", UNUM_CURRENCY_FIELD);
    assertFalse("", pa.hasMultipleVariants());
    
}

void NumberFormat2Test::TestCurrencyAffixInfo() {
    CurrencyAffixInfo info;
    assertTrue("", info.isDefault());
    UnicodeString expectedSymbol("\u00a4");
    UnicodeString expectedSymbolIso("\u00a4\u00a4");
    UnicodeString expectedSymbols("\u00a4\u00a4\u00a4");
    assertEquals("", expectedSymbol.unescape(), info.fSymbol);
    assertEquals("", expectedSymbolIso.unescape(), info.fISO);
    assertEquals("", expectedSymbols.unescape(), info.fLong.getByVariant("one").toString());
    assertEquals("", expectedSymbols.unescape(), info.fLong.getByVariant("other").toString());
    assertEquals("", expectedSymbols.unescape(), info.fLong.getByVariant("two").toString());
    UErrorCode status = U_ZERO_ERROR;
    static UChar USD[] = {0x55, 0x53, 0x44, 0x0};
    LocalPointer<PluralRules> rules(PluralRules::forLocale("en", status));
    info.set("en", rules.getAlias(), USD, status);
    assertEquals("", "$", info.fSymbol);
    assertEquals("", "USD", info.fISO);
    assertEquals("", "US dollar", info.fLong.getByVariant("one").toString());
    assertEquals("", "US dollars", info.fLong.getByVariant("other").toString());
    assertEquals("", "US dollars", info.fLong.getByVariant("two").toString());
    assertFalse("", info.isDefault());
    info.set(NULL, NULL, NULL, status);
    assertTrue("", info.isDefault());
    assertEquals("", expectedSymbol.unescape(), info.fSymbol);
    assertEquals("", expectedSymbolIso.unescape(), info.fISO);
    assertEquals("", expectedSymbols.unescape(), info.fLong.getByVariant("one").toString());
    assertEquals("", expectedSymbols.unescape(), info.fLong.getByVariant("other").toString());
    assertEquals("", expectedSymbols.unescape(), info.fLong.getByVariant("two").toString());
}

void NumberFormat2Test::TestAffixPattern() {
    static UChar chars[500];
    for (int32_t i = 0; i < UPRV_LENGTHOF(chars); ++i) {
        chars[i] = (UChar) (i + 1);
    }
    AffixPattern first;
    first.add(AffixPattern::kPercent);
    first.addLiteral(chars, 0, 200);
    first.addLiteral(chars, 200, 300);
    first.addCurrency(2);
    first.addLiteral(chars, 0, 256);
    AffixPattern second;
    second.add(AffixPattern::kPercent);
    second.addLiteral(chars, 0, 300);
    second.addLiteral(chars, 300, 200);
    second.addCurrency(2);
    second.addLiteral(chars, 0, 150);
    second.addLiteral(chars, 150, 106);
    assertTrue("", first.equals(second));
    AffixPatternIterator iter;
    second.remove();
    assertFalse("", second.iterator(iter).nextToken());
    assertTrue("", first.iterator(iter).nextToken());
    assertEquals("", AffixPattern::kPercent, iter.getTokenType());
    assertEquals("", 1, iter.getTokenLength());
    assertTrue("", iter.nextToken());
    UnicodeString str;
    assertEquals("", 500, iter.getLiteral(str).length());
    assertEquals("", AffixPattern::kLiteral, iter.getTokenType());
    assertEquals("", 500, iter.getTokenLength());
    assertTrue("", iter.nextToken());
    assertEquals("", AffixPattern::kCurrency, iter.getTokenType());
    assertEquals("", 2, iter.getTokenLength());
    assertTrue("", iter.nextToken());
    assertEquals("", 256, iter.getLiteral(str).length());
    assertEquals("", AffixPattern::kLiteral, iter.getTokenType());
    assertEquals("", 256, iter.getTokenLength());
    assertFalse("", iter.nextToken());
}

void NumberFormat2Test::TestAffixPatternDoubleQuote() {
    UnicodeString str("'Don''t'");
    AffixPattern expected;
    // Don't
    static UChar chars[] = {0x44, 0x6F, 0x6E, 0x27, 0x74};
    expected.addLiteral(chars, 0, UPRV_LENGTHOF(chars));
    AffixPattern actual;
    UErrorCode status = U_ZERO_ERROR;
    AffixPattern::parseUserAffixString(str, actual, status);
    assertTrue("", expected.equals(actual));
    UnicodeString formattedString;
    assertEquals("", "Don''t", actual.toUserString(formattedString));
    assertSuccess("", status);
}

void NumberFormat2Test::TestAffixPatternParser() {
    UErrorCode status = U_ZERO_ERROR;
    static UChar USD[] = {0x55, 0x53, 0x44, 0x0};
    LocalPointer<PluralRules> rules(PluralRules::forLocale("en", status));
    DecimalFormatSymbols symbols("en", status);
    AffixPatternParser parser(symbols);
    CurrencyAffixInfo currencyAffixInfo;
    currencyAffixInfo.set("en", rules.getAlias(), USD, status);
    PluralAffix affix;
    UnicodeString str("'--y'''dz'%'\u00a4\u00a4\u00a4\u00a4 y '\u00a4\u00a4\u00a4 or '\u00a4\u00a4 but '\u00a4");
    str = str.unescape();
    assertSuccess("", status);
    AffixPattern affixPattern;
    parser.parse(
            AffixPattern::parseAffixString(str, affixPattern, status),
            currencyAffixInfo,
            affix,
            status);
    UnicodeString formattedStr;
    affixPattern.toString(formattedStr);
    UnicodeString expectedFormattedStr("'--y''dz'%'\u00a4\u00a4\u00a4\u00a4 y '\u00a4\u00a4\u00a4 or '\u00a4\u00a4 but '\u00a4");
    expectedFormattedStr = expectedFormattedStr.unescape();
    assertEquals("", expectedFormattedStr, formattedStr);
    AffixPattern userAffixPattern;
    UnicodeString userStr("-'-'y'''d'z%\u00a4\u00a4\u00a4'\u00a4' y \u00a4\u00a4\u00a4 or \u00a4\u00a4 but \u00a4");
    AffixPattern::parseUserAffixString(userStr, userAffixPattern, status),
    assertTrue("", affixPattern.equals(userAffixPattern));
    AffixPattern userAffixPattern2;
    UnicodeString formattedUserStr;
    AffixPattern::parseUserAffixString(
            userAffixPattern.toUserString(formattedUserStr),
            userAffixPattern2,
            status);
    UnicodeString expectedFormattedUserStr(
            "-'-'y''dz%\u00a4\u00a4\u00a4'\u00a4' y \u00a4\u00a4\u00a4 or \u00a4\u00a4 but \u00a4");
    assertEquals("", expectedFormattedUserStr.unescape(), formattedUserStr);
    assertTrue("", userAffixPattern2.equals(userAffixPattern));
    assertSuccess("", status);
    assertTrue("", affixPattern.usesCurrency());
    assertTrue("", affixPattern.usesPercent());
    assertFalse("", affixPattern.usesPermill());
    assertTrue("", affix.hasMultipleVariants());
    {
        // other
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 1},
            {UNUM_PERCENT_FIELD, 6, 7},
            {UNUM_CURRENCY_FIELD, 7, 17},
            {UNUM_CURRENCY_FIELD, 21, 31},
            {UNUM_CURRENCY_FIELD, 35, 38},
            {UNUM_CURRENCY_FIELD, 43, 44},
            {0, -1, 0}};
        verifyAffix(
                "--y'dz%US dollars\u00a4 y US dollars or USD but $",
                affix.getByVariant("other"),
                expectedAttributes);
    }
    {
        // one
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 1},
            {UNUM_PERCENT_FIELD, 6, 7},
            {UNUM_CURRENCY_FIELD, 7, 16},
            {UNUM_CURRENCY_FIELD, 20, 29},
            {UNUM_CURRENCY_FIELD, 33, 36},
            {UNUM_CURRENCY_FIELD, 41, 42},
            {0, -1, 0}};
        verifyAffix(
                "--y'dz%US dollar\u00a4 y US dollar or USD but $",
                affix.getByVariant("one"),
                expectedAttributes);
    }
    affix.remove();
    str = "%'-";
    affixPattern.remove();
    parser.parse(
            AffixPattern::parseAffixString(str, affixPattern, status),
            currencyAffixInfo,
            affix,
            status);
    assertSuccess("", status);
    assertFalse("", affixPattern.usesCurrency());
    assertFalse("", affixPattern.usesPercent());
    assertFalse("", affixPattern.usesPermill());
    assertFalse("", affix.hasMultipleVariants());
    {
        // other
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 1, 2},
            {0, -1, 0}};
        verifyAffix(
                "%-",
                affix.getByVariant("other"),
                expectedAttributes);
    }
    UnicodeString a4("\u00a4");
    AffixPattern scratchPattern;
    AffixPattern::parseAffixString(a4.unescape(), scratchPattern, status);
    assertFalse("", scratchPattern.usesCurrency());

    // Test really long string > 256 chars.
    str = "'\u2030012345678901234567890123456789012345678901234567890123456789"
          "012345678901234567890123456789012345678901234567890123456789"
          "012345678901234567890123456789012345678901234567890123456789"
          "012345678901234567890123456789012345678901234567890123456789"
          "012345678901234567890123456789012345678901234567890123456789";
    str = str.unescape();
    affixPattern.remove();
    affix.remove();
    parser.parse(
            AffixPattern::parseAffixString(str, affixPattern, status),
            currencyAffixInfo,
            affix,
            status);
    assertSuccess("", status);
    assertFalse("", affixPattern.usesCurrency());
    assertFalse("", affixPattern.usesPercent());
    assertTrue("", affixPattern.usesPermill());
    assertFalse("", affix.hasMultipleVariants());
    {
       UnicodeString expected =
           "\u2030012345678901234567890123456789012345678901234567890123456789"
           "012345678901234567890123456789012345678901234567890123456789"
           "012345678901234567890123456789012345678901234567890123456789"
           "012345678901234567890123456789012345678901234567890123456789"
           "012345678901234567890123456789012345678901234567890123456789";
        expected = expected.unescape();
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_PERMILL_FIELD, 0, 1},
            {0, -1, 0}};
        verifyAffix(
                expected,
                affix.getOtherVariant(),
                expectedAttributes);
    }
}

void NumberFormat2Test::TestAffixPatternAppend() {
  AffixPattern pattern;
  UErrorCode status = U_ZERO_ERROR;
  UnicodeString patternStr("%\u2030");
  AffixPattern::parseUserAffixString(
          patternStr.unescape(), pattern, status);

  AffixPattern appendPattern;
  UnicodeString appendPatternStr("-\u00a4\u00a4*");
  AffixPattern::parseUserAffixString(
          appendPatternStr.unescape(), appendPattern, status);

  AffixPattern expectedPattern;
  UnicodeString expectedPatternStr("%\u2030-\u00a4\u00a4*");
  AffixPattern::parseUserAffixString(
          expectedPatternStr, expectedPattern, status);
  
  assertTrue("", pattern.append(appendPattern).equals(expectedPattern));
  assertSuccess("", status);
}

void NumberFormat2Test::TestAffixPatternAppendAjoiningLiterals() {
  AffixPattern pattern;
  UErrorCode status = U_ZERO_ERROR;
  UnicodeString patternStr("%baaa");
  AffixPattern::parseUserAffixString(
          patternStr, pattern, status);

  AffixPattern appendPattern;
  UnicodeString appendPatternStr("caa%");
  AffixPattern::parseUserAffixString(
          appendPatternStr, appendPattern, status);

  AffixPattern expectedPattern;
  UnicodeString expectedPatternStr("%baaacaa%");
  AffixPattern::parseUserAffixString(
          expectedPatternStr, expectedPattern, status);
  
  assertTrue("", pattern.append(appendPattern).equals(expectedPattern));
  assertSuccess("", status);
}

void NumberFormat2Test::TestDigitAffixesAndPadding() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    DigitFormatter formatter(symbols);
    DigitGrouping grouping;
    grouping.fGrouping = 3;
    FixedPrecision precision;
    DigitFormatterOptions options;
    options.fAlwaysShowDecimal = TRUE;
    ValueFormatter vf;
    vf.prepareFixedDecimalFormatting(
            formatter,
            grouping,
            precision,
            options);
    DigitAffixesAndPadding aap;
    aap.fPositivePrefix.append("(+", UNUM_SIGN_FIELD);
    aap.fPositiveSuffix.append("+)", UNUM_SIGN_FIELD);
    aap.fNegativePrefix.append("(-", UNUM_SIGN_FIELD);
    aap.fNegativeSuffix.append("-)", UNUM_SIGN_FIELD);
    aap.fWidth = 10;
    aap.fPadPosition = DigitAffixesAndPadding::kPadBeforePrefix;
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 4, 6},
            {UNUM_INTEGER_FIELD, 6, 7},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 7, 8},
            {UNUM_SIGN_FIELD, 8, 10},
            {0, -1, 0}};
        verifyAffixesAndPaddingInt32(
                "****(+3.+)",
                aap,
                3,
                vf,
                NULL,
                expectedAttributes);
    }
    aap.fPadPosition = DigitAffixesAndPadding::kPadAfterPrefix;
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 2},
            {UNUM_INTEGER_FIELD, 6, 7},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 7, 8},
            {UNUM_SIGN_FIELD, 8, 10},
            {0, -1, 0}};
        verifyAffixesAndPaddingInt32(
                "(+****3.+)",
                aap,
                3,
                vf,
                NULL,
                expectedAttributes);
    }
    aap.fPadPosition = DigitAffixesAndPadding::kPadBeforeSuffix;
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 2},
            {UNUM_INTEGER_FIELD, 2, 3},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 3, 4},
            {UNUM_SIGN_FIELD, 8, 10},
            {0, -1, 0}};
        verifyAffixesAndPaddingInt32(
                "(+3.****+)",
                aap,
                3,
                vf,
                NULL,
                expectedAttributes);
    }
    aap.fPadPosition = DigitAffixesAndPadding::kPadAfterSuffix;
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 2},
            {UNUM_INTEGER_FIELD, 2, 3},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 3, 4},
            {UNUM_SIGN_FIELD, 4, 6},
            {0, -1, 0}};
        verifyAffixesAndPaddingInt32(
                "(+3.+)****",
                aap,
                3,
                vf,
                NULL,
                expectedAttributes);
    }
    aap.fPadPosition = DigitAffixesAndPadding::kPadAfterSuffix;
    {
        DigitList digits;
        digits.set(-1234.5);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 2},
            {UNUM_GROUPING_SEPARATOR_FIELD, 3, 4},
            {UNUM_INTEGER_FIELD, 2, 7},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 7, 8},
            {UNUM_FRACTION_FIELD, 8, 9},
            {UNUM_SIGN_FIELD, 9, 11},
            {0, -1, 0}};
        verifyAffixesAndPadding(
                "(-1,234.5-)",
                aap,
                digits,
                vf,
                NULL,
                expectedAttributes);
    }
    assertFalse("", aap.needsPluralRules());

    aap.fWidth = 0;
    aap.fPositivePrefix.remove();
    aap.fPositiveSuffix.remove();
    aap.fNegativePrefix.remove();
    aap.fNegativeSuffix.remove();
    
    // Set up for plural currencies.
    aap.fNegativePrefix.append("-", UNUM_SIGN_FIELD);
    {
        PluralAffix part;
        part.setVariant("one", " Dollar", status);
        part.setVariant("other", " Dollars", status);
        aap.fPositiveSuffix.append(part, UNUM_CURRENCY_FIELD, status);
    }
    aap.fNegativeSuffix = aap.fPositiveSuffix;
    
    LocalPointer<PluralRules> rules(PluralRules::forLocale("en", status));

    // Exercise the fastrack path
    {
        options.fAlwaysShowDecimal = FALSE;
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 1},
            {UNUM_INTEGER_FIELD, 1, 3},
            {UNUM_CURRENCY_FIELD, 3, 11},
            {0, -1, 0}};
        verifyAffixesAndPaddingInt32(
                "-45 Dollars",
                aap,
                -45,
                vf,
                NULL,
                expectedAttributes);
        options.fAlwaysShowDecimal = TRUE;
    }
    
    // Now test plurals
    assertTrue("", aap.needsPluralRules());
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 1, 2},
            {UNUM_CURRENCY_FIELD, 2, 9},
            {0, -1, 0}};
        verifyAffixesAndPaddingInt32(
                "1. Dollar",
                aap,
                1,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 1},
            {UNUM_INTEGER_FIELD, 1, 2},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 2, 3},
            {UNUM_CURRENCY_FIELD, 3, 10},
            {0, -1, 0}};
        verifyAffixesAndPaddingInt32(
                "-1. Dollar",
                aap,
                -1,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    precision.fMin.setFracDigitCount(2);
    {
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 1, 2},
            {UNUM_FRACTION_FIELD, 2, 4},
            {UNUM_CURRENCY_FIELD, 4, 12},
            {0, -1, 0}};
        verifyAffixesAndPaddingInt32(
                "1.00 Dollars",
                aap,
                1,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
}

void NumberFormat2Test::TestPluralsAndRounding() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    DigitFormatter formatter(symbols);
    DigitGrouping grouping;
    FixedPrecision precision;
    precision.fSignificant.setMax(3);
    DigitFormatterOptions options;
    ValueFormatter vf;
    vf.prepareFixedDecimalFormatting(
            formatter,
            grouping,
            precision,
            options);
    DigitList digits;
    DigitAffixesAndPadding aap;
    // Set up for plural currencies.
    aap.fNegativePrefix.append("-", UNUM_SIGN_FIELD);
    {
        PluralAffix part;
        part.setVariant("one", " Dollar", status);
        part.setVariant("other", " Dollars", status);
        aap.fPositiveSuffix.append(part, UNUM_CURRENCY_FIELD, status);
    }
    aap.fNegativeSuffix = aap.fPositiveSuffix;
    aap.fWidth = 14;
    LocalPointer<PluralRules> rules(PluralRules::forLocale("en", status));
    {
        digits.set(0.999);
        verifyAffixesAndPadding(
                "*0.999 Dollars",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    {
        digits.set(0.9996);
        verifyAffixesAndPadding(
                "******1 Dollar",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    {
        digits.set(1.004);
        verifyAffixesAndPadding(
                "******1 Dollar",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    precision.fSignificant.setMin(2);
    {
        digits.set(0.9996);
        verifyAffixesAndPadding(
                "***1.0 Dollars",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    {
        digits.set(1.004);
        verifyAffixesAndPadding(
                "***1.0 Dollars",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    precision.fSignificant.setMin(0);
    {
        digits.set(-79.214);
        verifyAffixesAndPadding(
                "*-79.2 Dollars",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    // No more sig digits just max fractions
    precision.fSignificant.setMax(0); 
    precision.fMax.setFracDigitCount(4);
    {
        digits.set(79.213562);
        verifyAffixesAndPadding(
                "79.2136 Dollars",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }

}


void NumberFormat2Test::TestPluralsAndRoundingScientific() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    DigitFormatter formatter(symbols);
    SciFormatter sciformatter(symbols);
    ScientificPrecision precision;
    precision.fMantissa.fSignificant.setMax(4);
    SciFormatterOptions options;
    ValueFormatter vf;
    vf.prepareScientificFormatting(
            sciformatter,
            formatter,
            precision,
            options);
    DigitList digits;
    DigitAffixesAndPadding aap;
    aap.fNegativePrefix.append("-", UNUM_SIGN_FIELD);
    {
        PluralAffix part;
        part.setVariant("one", " Meter", status);
        part.setVariant("other", " Meters", status);
        aap.fPositiveSuffix.append(part, UNUM_FIELD_COUNT, status);
    }
    aap.fNegativeSuffix = aap.fPositiveSuffix;
    LocalPointer<PluralRules> rules(PluralRules::forLocale("en", status));
    {
        digits.set(0.99996);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_EXPONENT_SYMBOL_FIELD, 1, 2},
            {UNUM_EXPONENT_FIELD, 2, 3},
            {0, -1, 0}};
        verifyAffixesAndPadding(
                "1E0 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    options.fMantissa.fAlwaysShowDecimal = TRUE;
    {
        digits.set(0.99996);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 1, 2},
            {UNUM_EXPONENT_SYMBOL_FIELD, 2, 3},
            {UNUM_EXPONENT_FIELD, 3, 4},
            {0, -1, 0}};
        verifyAffixesAndPadding(
                "1.E0 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    {
        digits.set(-299792458);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_SIGN_FIELD, 0, 1},
            {UNUM_INTEGER_FIELD, 1, 2},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 2, 3},
            {UNUM_FRACTION_FIELD, 3, 6},
            {UNUM_EXPONENT_SYMBOL_FIELD, 6, 7},
            {UNUM_EXPONENT_FIELD, 7, 8},
            {0, -1, 0}};
        verifyAffixesAndPadding(
                "-2.998E8 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    precision.fMantissa.fSignificant.setMin(4);
    options.fExponent.fAlwaysShowSign = TRUE;
    options.fExponent.fMinDigits = 3;
    {
        digits.set(3);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 1, 2},
            {UNUM_FRACTION_FIELD, 2, 5},
            {UNUM_EXPONENT_SYMBOL_FIELD, 5, 6},
            {UNUM_EXPONENT_SIGN_FIELD, 6, 7},
            {UNUM_EXPONENT_FIELD, 7, 10},
            {0, -1, 0}};
        verifyAffixesAndPadding(
                "3.000E+000 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    precision.fMantissa.fMax.setIntDigitCount(3);
    {
        digits.set(0.00025001);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 3},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 3, 4},
            {UNUM_FRACTION_FIELD, 4, 5},
            {UNUM_EXPONENT_SYMBOL_FIELD, 5, 6},
            {UNUM_EXPONENT_SIGN_FIELD, 6, 7},
            {UNUM_EXPONENT_FIELD, 7, 10},
            {0, -1, 0}};
        verifyAffixesAndPadding(
                "250.0E-006 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    {
        digits.set(0.0000025001);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 1, 2},
            {UNUM_FRACTION_FIELD, 2, 5},
            {UNUM_EXPONENT_SYMBOL_FIELD, 5, 6},
            {UNUM_EXPONENT_SIGN_FIELD, 6, 7},
            {UNUM_EXPONENT_FIELD, 7, 10},
            {0, -1, 0}};
        verifyAffixesAndPadding(
                "2.500E-006 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    precision.fMantissa.fMax.setFracDigitCount(1);
    {
        digits.set(0.0000025499);
        NumberFormat2Test_Attributes expectedAttributes[] = {
            {UNUM_INTEGER_FIELD, 0, 1},
            {UNUM_DECIMAL_SEPARATOR_FIELD, 1, 2},
            {UNUM_FRACTION_FIELD, 2, 3},
            {UNUM_EXPONENT_SYMBOL_FIELD, 3, 4},
            {UNUM_EXPONENT_SIGN_FIELD, 4, 5},
            {UNUM_EXPONENT_FIELD, 5, 8},
            {0, -1, 0}};
        verifyAffixesAndPadding(
                "2.5E-006 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                expectedAttributes);
    }
    precision.fMantissa.fMax.setIntDigitCount(1);
    precision.fMantissa.fMax.setFracDigitCount(2);
    {
        digits.set(299792458);
        verifyAffixesAndPadding(
                "3.00E+008 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    // clear significant digits
    precision.fMantissa.fSignificant.setMin(0);
    precision.fMantissa.fSignificant.setMax(0);

    // set int and fraction digits
    precision.fMantissa.fMin.setFracDigitCount(2);
    precision.fMantissa.fMax.setFracDigitCount(4);
    precision.fMantissa.fMin.setIntDigitCount(2);
    precision.fMantissa.fMax.setIntDigitCount(3);
    {
        digits.set(-0.0000025300001);
        verifyAffixesAndPadding(
                "-253.00E-008 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    {
        digits.set(-0.0000025300006);
        verifyAffixesAndPadding(
                "-253.0001E-008 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
    {
        digits.set(-0.000025300006);
        verifyAffixesAndPadding(
                "-25.30E-006 Meters",
                aap,
                digits,
                vf,
                rules.getAlias(),
                NULL);
    }
}


void NumberFormat2Test::TestRoundingIncrement() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols symbols("en", status);
    DigitFormatter formatter(symbols);
    SciFormatter sciformatter(symbols);
    ScientificPrecision precision;
    SciFormatterOptions options;
    precision.fMantissa.fRoundingIncrement.set(0.25);
    precision.fMantissa.fSignificant.setMax(4);
    DigitGrouping grouping;
    ValueFormatter vf;

    // fixed
    vf.prepareFixedDecimalFormatting(
            formatter,
            grouping,
            precision.fMantissa,
            options.fMantissa);
    DigitList digits;
    DigitAffixesAndPadding aap;
    aap.fNegativePrefix.append("-", UNUM_SIGN_FIELD);
    {
        digits.set(3.7);
        verifyAffixesAndPadding(
                "3.75",
                aap,
                digits,
                vf,
                NULL, NULL);
    }
    {
        digits.set(-7.4);
        verifyAffixesAndPadding(
                "-7.5",
                aap,
                digits,
                vf,
                NULL, NULL);
    }
    {
        digits.set(99.8);
        verifyAffixesAndPadding(
                "99.75",
                aap,
                digits,
                vf,
                NULL, NULL);
    }
    precision.fMantissa.fMin.setFracDigitCount(2);
    {
        digits.set(99.1);
        verifyAffixesAndPadding(
                "99.00",
                aap,
                digits,
                vf,
                NULL, NULL);
    }
    {
        digits.set(-639.65);
        verifyAffixesAndPadding(
                "-639.80",
                aap,
                digits,
                vf,
                NULL, NULL);
    }

    precision.fMantissa.fMin.setIntDigitCount(2);
    // Scientific notation
    vf.prepareScientificFormatting(
            sciformatter,
            formatter,
            precision,
            options);
    {
        digits.set(-6396.5);
        verifyAffixesAndPadding(
                "-64.00E2",
                aap,
                digits,
                vf,
                NULL, NULL);
    }
    {
        digits.set(-0.00092374);
        verifyAffixesAndPadding(
                "-92.25E-5",
                aap,
                digits,
                vf,
                NULL, NULL);
    }
    precision.fMantissa.fMax.setIntDigitCount(3);
    {
        digits.set(-0.00092374);
        verifyAffixesAndPadding(
                "-923.80E-6",
                aap,
                digits,
                vf,
                NULL, NULL);
    }
}


void NumberFormat2Test::TestDigitListToFixedDecimal() {
    DigitList digits;
    DigitInterval interval;
    digits.set(-9217.875);
    {
        interval.setIntDigitCount(2);
        interval.setFracDigitCount(1);
        FixedDecimal result(digits, interval);
        verifyFixedDecimal(result, 178, 10, TRUE, 1, 8);
    }
    {
        interval.setIntDigitCount(6);
        interval.setFracDigitCount(7);
        FixedDecimal result(digits, interval);
        verifyFixedDecimal(result, 9217875, 1000, TRUE, 7, 8750000);
    }
    {
        digits.set(1234.56);
        interval.setIntDigitCount(6);
        interval.setFracDigitCount(25);
        FixedDecimal result(digits, interval);
        verifyFixedDecimal(result, 123456, 100, FALSE, 18, 560000000000000000LL);
    }
}

void NumberFormat2Test::TestGetAffixes() {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString pattern("\\u00a4\\u00a4\\u00a4 0.00 %\\u00a4\\u00a4");
    pattern = pattern.unescape();
    DecimalFormat2 fmt("en_US", pattern, status);
    assertSuccess("", status);
    UnicodeString affixStr;
    assertEquals("", "US dollars ", fmt.getPositivePrefix(affixStr));
    assertEquals("", " %USD", fmt.getPositiveSuffix(affixStr));
    assertEquals("", "-US dollars ", fmt.getNegativePrefix(affixStr));
    assertEquals("", " %USD", fmt.getNegativeSuffix(affixStr));

    // Test equality with affixes. set affix methods can't capture special
    // characters which is why equality should fail.
    {
        DecimalFormat2 fmtCopy(fmt);
        assertTrue("", fmt == fmtCopy);
        UnicodeString someAffix;
        fmtCopy.setPositivePrefix(fmtCopy.getPositivePrefix(someAffix));
        assertTrue("", fmt != fmtCopy);
    }
    {
        DecimalFormat2 fmtCopy(fmt);
        assertTrue("", fmt == fmtCopy);
        UnicodeString someAffix;
        fmtCopy.setPositiveSuffix(fmtCopy.getPositiveSuffix(someAffix));
        assertTrue("", fmt != fmtCopy);
    }
    {
        DecimalFormat2 fmtCopy(fmt);
        assertTrue("", fmt == fmtCopy);
        UnicodeString someAffix;
        fmtCopy.setNegativePrefix(fmtCopy.getNegativePrefix(someAffix));
        assertTrue("", fmt != fmtCopy);
    }
    {
        DecimalFormat2 fmtCopy(fmt);
        assertTrue("", fmt == fmtCopy);
        UnicodeString someAffix;
        fmtCopy.setNegativeSuffix(fmtCopy.getNegativeSuffix(someAffix));
        assertTrue("", fmt != fmtCopy);
    }
    fmt.setPositivePrefix("Don't");
    fmt.setPositiveSuffix("do");
    UnicodeString someAffix("be''eet\\u00a4\\u00a4\\u00a4 it.");
    someAffix = someAffix.unescape();
    fmt.setNegativePrefix(someAffix);
    fmt.setNegativeSuffix("%");
    assertEquals("", "Don't", fmt.getPositivePrefix(affixStr));
    assertEquals("", "do", fmt.getPositiveSuffix(affixStr));
    assertEquals("", someAffix, fmt.getNegativePrefix(affixStr));
    assertEquals("", "%", fmt.getNegativeSuffix(affixStr));
}

void NumberFormat2Test::TestApplyPatternResets() {
    // TODO: Known to fail. See ticket 11645
/*
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString pattern("#,###0.0");
    DecimalFormat2 fmt("en", pattern, status);
    if (!assertSuccess("", status)) {
        return;
    }
    {
        DecimalFormat2 fmtCopy(fmt);
        fmtCopy.setMultiplier(37);
        fmtCopy.applyPattern(pattern, status);
        assertTrue("multiplier", fmt == fmtCopy);
    }
    {
        DecimalFormat2 fmtCopy(fmt);
        fmtCopy.setRoundingMode(DigitList::kRoundCeiling);
        fmtCopy.applyPattern(pattern, status);
        assertTrue("roundingMode", fmt == fmtCopy);
    }
    {
        DecimalFormat2 fmtCopy(fmt);
        assertFalse("", fmtCopy.isDecimalSeparatorAlwaysShown());
        fmtCopy.setDecimalSeparatorAlwaysShown(TRUE);
        fmtCopy.applyPattern(pattern, status);
        assertTrue("decimalSeparatorAlwaysShown", fmt == fmtCopy);
    }
    {
        DecimalFormat2 fmtCopy(fmt);
        static UChar funnyCurrency[] = {0x45, 0x41, 0x54, 0x0}; // EAT
        fmtCopy.setCurrency(funnyCurrency, status);
        fmtCopy.applyPattern(pattern, status);
        assertTrue("currency", fmt == fmtCopy);
    }
    {
        DecimalFormat2 fmtCopy(fmt);
        fmtCopy.setCurrencyUsage(UCURR_USAGE_CASH, status);
        fmtCopy.applyPattern(pattern, status);
        assertTrue("currencyUsage", fmt == fmtCopy);
    }
    assertSuccess("", status);
*/
}


void NumberFormat2Test::TestDataDriven() {
    NumberFormat2TestDataDriven dd;
//    dd.run("dumb.txt", FALSE);
    dd.run("numberformattestspecification.txt", TRUE);
}

void NumberFormat2Test::TestToPatternScientific11648() {
    UErrorCode status = U_ZERO_ERROR;
    Locale en("en");
    DecimalFormat2 fmt(en, "0.00", status);
    fmt.setScientificNotation(TRUE);
    UnicodeString pattern;
    // Fails, produces "0.00E"
    assertEquals("", "0.00E0", fmt.toPattern(pattern));
    DecimalFormat fmt2(pattern, status);
    // Fails, bad pattern.
    assertSuccess("", status);
}


void NumberFormat2Test::verifyFixedDecimal(
        const FixedDecimal &result,
        int64_t numerator,
        int64_t denominator,
        UBool bNegative,
        int32_t v,
        int64_t f) {
    assertEquals("", numerator, (int64_t) (result.source * (double) denominator + 0.5));
    assertEquals("", v, result.visibleDecimalDigitCount);
    assertEquals("", f, result.decimalDigits);
    assertEquals("", bNegative, result.isNegative);
}

void NumberFormat2Test::verifyAffixesAndPadding(
        const UnicodeString &expected,
        const DigitAffixesAndPadding &aaf,
        DigitList &digits,
        const ValueFormatter &vf,
        const PluralRules *optPluralRules,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    UnicodeString appendTo;
    NumberFormat2Test_FieldPositionHandler handler;
    UErrorCode status = U_ZERO_ERROR;
    assertEquals(
            "",
            expected,
            aaf.format(
                    digits,
                    vf,
                    handler,
                    optPluralRules,
                    appendTo,
                    status));
    assertSuccess("", status);
    if (expectedAttributes != NULL) {
        verifyAttributes(expectedAttributes, handler.attributes);
    }
}

void NumberFormat2Test::verifyAffixesAndPaddingInt32(
        const UnicodeString &expected,
        const DigitAffixesAndPadding &aaf,
        int32_t value,
        const ValueFormatter &vf,
        const PluralRules *optPluralRules,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    UnicodeString appendTo;
    NumberFormat2Test_FieldPositionHandler handler;
    UErrorCode status = U_ZERO_ERROR;
    assertEquals(
            "",
            expected,
            aaf.formatInt32(
                    value,
                    vf,
                    handler,
                    optPluralRules,
                    appendTo,
                    status));
    assertSuccess("", status);
    if (expectedAttributes != NULL) {
        verifyAttributes(expectedAttributes, handler.attributes);
    }
    DigitList digits;
    digits.set(value);
    verifyAffixesAndPadding(
            expected, aaf, digits, vf, optPluralRules, expectedAttributes);
}

void NumberFormat2Test::verifyAffix(
        const UnicodeString &expected,
        const DigitAffix &affix,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    UnicodeString appendTo;
    NumberFormat2Test_FieldPositionHandler handler;
    assertEquals(
            "",
            expected.unescape(),
            affix.format(handler, appendTo));
    if (expectedAttributes != NULL) {
        verifyAttributes(expectedAttributes, handler.attributes);
    }
}

void NumberFormat2Test::verifyValueFormatter(
        const UnicodeString &expected,
        const ValueFormatter &formatter,
        DigitList &digits,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    UErrorCode status = U_ZERO_ERROR;
    formatter.round(digits, status);
    assertSuccess("", status);
    assertEquals(
            "",
            expected.countChar32(),
            formatter.countChar32(digits));
    UnicodeString appendTo;
    NumberFormat2Test_FieldPositionHandler handler;
    assertEquals(
            "",
            expected,
            formatter.format(
                    digits,
                    handler,
                    appendTo));
    if (expectedAttributes != NULL) {
        verifyAttributes(expectedAttributes, handler.attributes);
    }
}

// Right now only works for positive values.
void NumberFormat2Test::verifyDigitList(
        const UnicodeString &expected,
        const DigitList &digits) {
    DigitFormatter formatter;
    DigitInterval interval;
    DigitGrouping grouping;
    verifyDigitFormatter(
            expected,
            formatter,
            digits,
            grouping,
            digits.getSmallestInterval(interval),
            FALSE,
            NULL);
}

void NumberFormat2Test::verifyDigitIntFormatter(
        const UnicodeString &expected,
        const DigitFormatter &formatter,
        int32_t value,
        int32_t minDigits,
        UBool alwaysShowSign,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    DigitFormatterIntOptions options;
    options.fMinDigits = minDigits;
    options.fAlwaysShowSign = alwaysShowSign;
    assertEquals(
            "",
            expected.countChar32(),
            formatter.countChar32ForInt32(value, options));
    UnicodeString appendTo;
    NumberFormat2Test_FieldPositionHandler handler(expectedAttributes != NULL);
    assertEquals(
            "",
            expected,
            formatter.formatInt32(
                    value,
                    options,
                    kSignField,
                    kIntField,
                    handler,
                    appendTo));
    if (expectedAttributes != NULL) {
        verifyAttributes(expectedAttributes, handler.attributes);
    }
}

void NumberFormat2Test::verifySciFormatter(
        const UnicodeString &expected,
        const SciFormatter &sciformatter,
        const DigitList &mantissa,
        int32_t exponent,
        const DigitFormatter &formatter,
        const DigitInterval &interval,
        const SciFormatterOptions &options,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    assertEquals(
            "",
            expected.countChar32(),
            sciformatter.countChar32(
                    exponent,
                    formatter,
                    interval,
                    options));
    UnicodeString appendTo;
    NumberFormat2Test_FieldPositionHandler handler;
    assertEquals(
            "",
            expected,
            sciformatter.format(
                    mantissa,
                    exponent,
                    formatter,
                    interval,
                    options,
                    handler,
                    appendTo));
    if (expectedAttributes != NULL) {
        verifyAttributes(expectedAttributes, handler.attributes);
    }
}

void NumberFormat2Test::verifyPositiveIntDigitFormatter(
        const UnicodeString &expected,
        const DigitFormatter &formatter,
        int32_t value,
        int32_t minDigits,
        int32_t maxDigits,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    IntDigitCountRange range(minDigits, maxDigits);
    UnicodeString appendTo;
    NumberFormat2Test_FieldPositionHandler handler;
    assertEquals(
            "",
            expected,
            formatter.formatPositiveInt32(
                    value,
                    range,
                    handler,
                    appendTo));
    if (expectedAttributes != NULL) {
        verifyAttributes(expectedAttributes, handler.attributes);
    }
}

void NumberFormat2Test::verifyDigitFormatter(
        const UnicodeString &expected,
        const DigitFormatter &formatter,
        const DigitList &digits,
        const DigitGrouping &grouping,
        const DigitInterval &interval,
        UBool alwaysShowDecimal,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    DigitFormatterOptions options;
    options.fAlwaysShowDecimal = alwaysShowDecimal;
    verifyDigitFormatter(
            expected,
            formatter,
            digits,
            grouping,
            interval,
            options,
            expectedAttributes);
}

void NumberFormat2Test::verifyDigitFormatter(
        const UnicodeString &expected,
        const DigitFormatter &formatter,
        const DigitList &digits,
        const DigitGrouping &grouping,
        const DigitInterval &interval,
        const DigitFormatterOptions &options,
        const NumberFormat2Test_Attributes *expectedAttributes) {
    assertEquals(
            "",
            expected.countChar32(),
            formatter.countChar32(grouping, interval, options));
    UnicodeString appendTo;
    NumberFormat2Test_FieldPositionHandler handler;
    assertEquals(
            "",
            expected,
            formatter.format(
                    digits,
                    grouping,
                    interval,
                    options,
                    handler,
                    appendTo));
    if (expectedAttributes != NULL) {
        verifyAttributes(expectedAttributes, handler.attributes);
    }
}

void NumberFormat2Test::verifySmallIntFormatter(
        const UnicodeString &expected,
        int32_t positiveValue,
        int32_t minDigits,
        int32_t maxDigits) {
    IntDigitCountRange range(minDigits, maxDigits);
    if (!SmallIntFormatter::canFormat(positiveValue, range)) {
        UnicodeString actual;
        assertEquals("", expected, actual);
        return;
    }
    UnicodeString actual;
    assertEquals("", expected, SmallIntFormatter::format(positiveValue, range, actual));
}

void NumberFormat2Test::verifyAttributes(
        const NumberFormat2Test_Attributes *expected,
        const NumberFormat2Test_Attributes *actual) {
    int32_t idx = 0;
    while (expected[idx].spos != -1 && actual[idx].spos != -1) {
        assertEquals("id", expected[idx].id, actual[idx].id);
        assertEquals("spos", expected[idx].spos, actual[idx].spos);
        assertEquals("epos", expected[idx].epos, actual[idx].epos);
        ++idx;
    }
    assertEquals(
            "expected and actual not same length",
            expected[idx].spos,
            actual[idx].spos);
}

extern IntlTest *createNumberFormat2Test() {
    return new NumberFormat2Test();
}

#endif /* !UCONFIG_NO_FORMATTING */