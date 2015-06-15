/*
********************************************************************************
*   Copyright (C) 2015, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File DECIMFMTIMPL.H
********************************************************************************
*/

#ifndef DECIMFMTIMPL_H
#define DECIMFMTIMPL_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "affixpatternparser.h"
#include "sciformatter.h"
#include "digitformatter.h"
#include "digitgrouping.h"
#include "precision.h"
#include "digitaffixesandpadding.h"
#include "unicode/decimfmt.h"

U_NAMESPACE_BEGIN

class UnicodeString;
class FieldPosition;
class ValueFormatter;
class FieldPositionHandler;
class FixedDecimal;

class DecimalFormatImpl : public UObject {
public:

DecimalFormatImpl(
        const Locale &locale,
        const UnicodeString &pattern,
        UErrorCode &status);
DecimalFormatImpl(
        const UnicodeString &pattern,
        DecimalFormatSymbols *symbolsToAdopt,
        UParseError &parseError,
        UErrorCode &status);
DecimalFormatImpl(const DecimalFormatImpl &other);
DecimalFormatImpl &operator=(const DecimalFormatImpl &other);
virtual ~DecimalFormatImpl();
void adoptDecimalFormatSymbols(DecimalFormatSymbols *symbolsToAdopt);
const DecimalFormatSymbols &getDecimalFormatSymbols() const {
    return *fSymbols;
}
UnicodeString &format(
        int32_t number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const;
UnicodeString &format(
        int32_t number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const;
UnicodeString &format(
        int64_t number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const;
UnicodeString &format(
        double number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const;
UnicodeString &format(
        const DigitList &number,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const;
UnicodeString &format(
        int64_t number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const;
UnicodeString &format(
        double number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const;
UnicodeString &format(
        const DigitList &number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const;
UnicodeString &format(
        const StringPiece &number,
        UnicodeString &appendTo,
        FieldPositionIterator *posIter,
        UErrorCode &status) const;

UBool operator==(const DecimalFormatImpl &) const;

UBool operator!=(const DecimalFormatImpl &other) const {
    return !(*this == other);
}

void setRoundingMode(DecimalFormat::ERoundingMode mode) {
    fRoundingMode = mode;
    fEffPrecision.fMantissa.fExactOnly = (fRoundingMode == DecimalFormat::kRoundUnnecessary);
}
DecimalFormat::ERoundingMode getRoundingMode() const {
    return fRoundingMode;
}
void setFailIfMoreThanMaxDigits(UBool b) {
    fEffPrecision.fMantissa.fFailIfOverMax = b;
}
UBool isFailIfMoreThanMaxDigits() const { return fEffPrecision.fMantissa.fFailIfOverMax; }
void setMinimumIntegerDigits(int32_t newValue);
void setMaximumIntegerDigits(int32_t newValue);
void setMinMaxIntegerDigits(int32_t min, int32_t max);
void setMinimumFractionDigits(int32_t newValue);
void setMaximumFractionDigits(int32_t newValue);
void setMinMaxFractionDigits(int32_t min, int32_t max);
void setMinimumSignificantDigits(int32_t newValue);
void setMaximumSignificantDigits(int32_t newValue);
void setMinMaxSignificantDigits(int32_t min, int32_t max);
void setScientificNotation(UBool newValue);
void setSignificantDigitsUsed(UBool newValue);
int32_t getMinimumIntegerDigits() const { 
        return fMinIntDigits; }
int32_t getMaximumIntegerDigits() const { 
        return fMaxIntDigits; }
int32_t getMinimumFractionDigits() const { 
        return fMinFracDigits; }
int32_t getMaximumFractionDigits() const { 
        return fMaxFracDigits; }
int32_t getMinimumSignificantDigits() const { 
        return fMinSigDigits; }
int32_t getMaximumSignificantDigits() const { 
        return fMaxSigDigits; }
UBool isScientificNotation() const { return fUseScientific; }
UBool areSignificantDigitsUsed() const { return fUseSigDigits; }
void setGroupingSize(int32_t newValue);
void setSecondaryGroupingSize(int32_t newValue);
void setMinimumGroupingDigits(int32_t newValue);
void setGroupingUsed(UBool newValue);
int32_t getGroupingSize() const { return fGrouping.fGrouping; }
int32_t getSecondaryGroupingSize() const { return fGrouping.fGrouping2; }
int32_t getMinimumGroupingDigits() const { return fGrouping.fMinGrouping; }
UBool isGroupingUsed() const { return fUseGrouping; }
void setCurrency(const UChar *currency, UErrorCode &status);
const UChar *getCurrency() const { return fCurr; }
void applyPattern(const UnicodeString &pattern, UErrorCode &status);
void applyPattern(
        const UnicodeString &pattern, UParseError &perror, UErrorCode &status);
void applyLocalizedPattern(const UnicodeString &pattern, UErrorCode &status);
void applyLocalizedPattern(
        const UnicodeString &pattern, UParseError &perror, UErrorCode &status);
void setCurrencyUsage(UCurrencyUsage usage, UErrorCode &status);
UCurrencyUsage getCurrencyUsage() const { return fCurrencyUsage; }
void setRoundingIncrement(double d);
double getRoundingIncrement() const;
int32_t getMultiplier() const;
void setMultiplier(int32_t m);
UChar32 getPadCharacter() const { return fAap.fPadChar; }
void setPadCharacter(UChar32 c) { fAap.fPadChar = c; }
int32_t getFormatWidth() const { return fAap.fWidth; }
void setFormatWidth(int32_t x) { fAap.fWidth = x; }
DigitAffixesAndPadding::EPadPosition getPadPosition() const {
    return fAap.fPadPosition;
}
void setPadPosition(DigitAffixesAndPadding::EPadPosition x) {
    fAap.fPadPosition = x;
}
int32_t getMinimumExponentDigits() const {
    return fOptions.fExponent.fMinDigits;
}
void setMinimumExponentDigits(int32_t x) {
    fOptions.fExponent.fMinDigits = x;
}
UBool isExponentSignAlwaysShown() const {
    return fOptions.fExponent.fAlwaysShowSign;
}
void setExponentSignAlwaysShown(UBool x) {
    fOptions.fExponent.fAlwaysShowSign = x;
}
UBool isDecimalSeparatorAlwaysShown() const {
    return fOptions.fMantissa.fAlwaysShowDecimal;
}
void setDecimalSeparatorAlwaysShown(UBool x) {
    fOptions.fMantissa.fAlwaysShowDecimal = x;
}
UnicodeString &getPositivePrefix(UnicodeString &result) const;
UnicodeString &getPositiveSuffix(UnicodeString &result) const;
UnicodeString &getNegativePrefix(UnicodeString &result) const;
UnicodeString &getNegativeSuffix(UnicodeString &result) const;
void setPositivePrefix(const UnicodeString &str);
void setPositiveSuffix(const UnicodeString &str);
void setNegativePrefix(const UnicodeString &str);
void setNegativeSuffix(const UnicodeString &str);
UnicodeString &toPattern(UnicodeString& result) const;
UnicodeString select(double value, const PluralRules &rules) const;
UnicodeString select(DigitList &number, const PluralRules &rules) const;
FixedDecimal &getFixedDecimal(double value, FixedDecimal &result) const;
FixedDecimal &getFixedDecimal(DigitList &number, FixedDecimal &result) const;
DigitList &round(DigitList &number, UErrorCode &status) const;

private:

DigitList fMultiplier;
int32_t fScale;

DecimalFormat::ERoundingMode fRoundingMode;

// These fields include what the user can see and set.
// When the user updates these fields, it triggers automatic updates of
// other fields that may be invisible to user

// Updating any of the following fields triggers an update to
// fEffPrecision.fMantissa.fMin,
// fEffPrecision.fMantissa.fMax,
// fEffPrecision.fMantissa.fSignificant fields
// We have this two phase update because of backward compatibility. 
// DecimalFormat has to remember all settings even if those settings are
// invalid or disabled.
int32_t fMinIntDigits;
int32_t fMaxIntDigits;
int32_t fMinFracDigits;
int32_t fMaxFracDigits;
int32_t fMinSigDigits;
int32_t fMaxSigDigits;
UBool fUseScientific;
UBool fUseSigDigits;

// Updating any of the following fields triggers an update to
// fEffGrouping field Again we do it this way because original
// grouping settings have to be retained if grouping is turned off.
DigitGrouping fGrouping;
UBool fUseGrouping;

// Updating any of the following fields triggers updates on the following:
// fMonetary, fRules, fAffixParser, fCurrencyAffixInfo,
// fSciFormatter, fFormatter, fAap.fPositivePrefiix, fAap.fPositiveSuffix,
// fAap.fNegativePrefiix, fAap.fNegativeSuffix
// We do this two phase update because localizing the affix patterns
// and formatters can be expensive. Better to do it once with the setters
// than each time within format.
AffixPattern fPositivePrefixPattern;
AffixPattern fNegativePrefixPattern;
AffixPattern fPositiveSuffixPattern;
AffixPattern fNegativeSuffixPattern;
DecimalFormatSymbols *fSymbols;
UChar fCurr[4];
UCurrencyUsage fCurrencyUsage;

// Optional may be NULL
PluralRules *fRules;

// These fields are totally hidden from user and are used to derive the affixes
// in fAap below from the four affix patterns above.
UBool fMonetary;
AffixPatternParser fAffixParser;
CurrencyAffixInfo fCurrencyAffixInfo;

// The actual precision used when formatting
ScientificPrecision fEffPrecision;

// The actual grouping used when formatting
DigitGrouping fEffGrouping;
SciFormatterOptions fOptions;   // Encapsulates fixed precision options
SciFormatter fSciFormatter;
DigitFormatter fFormatter;
DigitAffixesAndPadding fAap;

UnicodeString &formatInt32(
        int32_t number,
        UnicodeString &appendTo,
        FieldPositionHandler &handler,
        UErrorCode &status) const;

// Scales for precent or permille symbols
UnicodeString &formatDigitList(
        DigitList &number,
        UnicodeString &appendTo,
        FieldPositionHandler &handler,
        UErrorCode &status) const;

// Does not scale for precent or permille symbols
UnicodeString &formatAdjustedDigitList(
        DigitList &number,
        UnicodeString &appendTo,
        FieldPositionHandler &handler,
        UErrorCode &status) const;

DigitList &adjustDigitList(DigitList &number, UErrorCode &status) const;

void applyPattern(
        const UnicodeString &pattern,
        UBool localized, UParseError &perror, UErrorCode &status);

ValueFormatter &prepareValueFormatter(ValueFormatter &vf) const;
void setMultiplierScale(int32_t s);
int32_t getPatternScale() const;
void setScale(int32_t s) { fScale = s; }
int32_t getScale() const { return fScale; }

// Updates everything
void updateAll(UErrorCode &status);

// Updates from changes to first group of attributes
void updatePrecision();

// Updates from changes to second group of attributes
void updateGrouping();

// Updates from changes to third group of attributes
void updateFormatting(int32_t changedFormattingFields, UErrorCode &status);

// Helper functions for updatePrecision
void updatePrecisionForScientific();
void updatePrecisionForFixed();
void extractMinMaxDigits(DigitInterval &min, DigitInterval &max) const;
void extractSigDigits(SignificantDigitInterval &sig) const;

// Helper functions for updateFormatting
void updateFormattingUsesCurrency(int32_t &changedFormattingFields);
void updateFormattingPluralRules(
        int32_t &changedFormattingFields, UErrorCode &status);
void updateFormattingAffixParser(int32_t &changedFormattingFields);
void updateFormattingCurrencyAffixInfo(
        int32_t &changedFormattingFields, UErrorCode &status);
void updateFormattingFixedPointFormatter(
        int32_t &changedFormattingFields);
void updateFormattingScientificFormatter(
        int32_t &changedFormattingFields);
void updateFormattingLocalizedPositivePrefix(
        int32_t &changedFormattingFields, UErrorCode &status);
void updateFormattingLocalizedPositiveSuffix(
        int32_t &changedFormattingFields, UErrorCode &status);
void updateFormattingLocalizedNegativePrefix(
        int32_t &changedFormattingFields, UErrorCode &status);
void updateFormattingLocalizedNegativeSuffix(
        int32_t &changedFormattingFields, UErrorCode &status);

int32_t computeExponentPatternLength() const;
int32_t countFractionDigitAndDecimalPatternLength(int32_t fracDigitCount) const;
UnicodeString &toNumberPattern(
        UBool hasPadding, int32_t minimumLength, UnicodeString& result) const;

int32_t getOldFormatWidth() const;
const UnicodeString &getConstSymbol(
        DecimalFormatSymbols::ENumberFormatSymbol symbol) const;
UBool isParseFastpath() const;

friend class DecimalFormat;

};


U_NAMESPACE_END

#endif // _DECIMFMT2
//eof
