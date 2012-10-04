/*
*****************************************************************************************
* Copyright (C) 2010-2012, International Business Machines
* Corporation and others. All Rights Reserved.
*****************************************************************************************
*/

#ifndef UGENDER_H
#define UGENDER_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"

/**
 * \file
 * \brief C API: The purpose of this API is to compute the gender of a list as a
 * whole given the gender of each element.
 *
 */

/**
 * Genders
 * @draft ICU 50
 */
enum UGender {
    UGENDER_MALE,
    UGENDER_FEMALE,
    UGENDER_OTHER
};
/**
 * @draft ICU 50
 */
typedef enum UGender UGender;

/**
 * Opaque UGenderInfo object for use in C programs.
 * @draft ICU 50
 */
struct UGenderInfo;
typedef struct UGenderInfo UGenderInfo;

/**
 * Opens a new UGenderInfo object given locale.
 * @param locale The locale for which the rules are desired.
 * @return A UGenderInfo for the specified locale, or NULL if an error occurred.
 * @stable ICU 4.8
 */
U_DRAFT const UGenderInfo* U_EXPORT2
ugender_getInstance(const char *locale, UErrorCode *status);


/**
 * Given a list, returns the gender of the list as a whole.
 * @param genders the gender of each element in the list.
 * @param size the size of the list.
 * @param status A pointer to a UErrorCode to receive any errors.
 * @return The gender of the list.
 * @draft ICU 50
 */
U_DRAFT UGender U_EXPORT2
ugender_getListGender(UGender *genders, int32_t size, UErrorCode *status);

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
