/*
*******************************************************************************
* Copyright (C) 2008-2012, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
*
* File GENDER.H
*
* Modification History:*
*   Date        Name        Description
*
********************************************************************************
*/

#ifndef _GENDER
#define _GENDER

#include "unicode/utypes.h"

/**
 * \file
 * \brief C++ API: The purpose of this API is to compute the gender of a list as
 * a whole given the gender of each element.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/locid.h"
#include "unicode/ugender.h"
#include "unicode/uobject.h"


U_NAMESPACE_BEGIN

class U_I18N_API GenderInfo : public UObject {
public:

    /**
     * Provides access to the predefined GenderInfo object for a given
     * locale.
     *
     * @param locale  The locale for which a <code>GenderInfo</code> object is
     *                returned.
     * @param status  Output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @return        The predefined <code>GenderInfo</code> object pointer for
     *                this locale. The returned object is immutable, so it is
     *                declared as const. Caller does not own the returned
     *                pointer, so it must not attempt to free it.
     * @draft ICU 50
     */
    static const GenderInfo* U_EXPORT2 getInstance(const Locale& locale, UErrorCode& status);

    /**
     * Determines the gender of a list as a whole given the gender of each
     * of the elements.
     * 
     * @param genders the gender of each element in the list.
     * @param length the length of gender array.
     * @param status  Output param set to success/failure code on exit, which
     *                must not indicate a failure before the function call.
     * @return        the gender of the whole list.
     * @draft ICU 50
     */
    UGender getListGender(const UGender* genders, int32_t length, UErrorCode& status) const;

private:
    int32_t _style;

    static GenderInfo* _neutral;
    static GenderInfo* _mixed;
    static GenderInfo* _taints;

    virtual ~GenderInfo();

    /**
     * No "poor man's RTTI"
     *
     * @draft ICU 50
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * Copy constructor. One object per locale invariant. Clients
     * must never copy GenderInfo objects.
     * @draft ICU 50
     */
    GenderInfo(const GenderInfo& other);

    /**
      * Assignment operator. Not applicable to immutable objects.
      * @draft ICU 50
      */
    GenderInfo& operator=(const GenderInfo&);

    GenderInfo(int32_t style);

    static GenderInfo* loadInstance(const Locale& locale, UErrorCode& status);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _GENDER
//eof
