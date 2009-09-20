/*
********************************************************************************
*   Copyright (C) 1996-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File UCHAR.C
*
* Modification History:
*
*   Date        Name        Description
*   04/02/97    aliu        Creation.
*   4/15/99     Madhu       Updated all the function definitions for C Implementation
*   5/20/99     Madhu       Added the function u_getVersion()
*   8/19/1999   srl         Upgraded scripts to Unicode3.0 
*   11/11/1999  weiv        added u_isalnum(), cleaned comments
*   01/11/2000  helena      Renamed u_getVersion to u_getUnicodeVersion.
*   06/20/2000  helena      OS/400 port changes; mostly typecast.
******************************************************************************
*/

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include "unicode/udata.h"
#include "umutex.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "utrie2.h"
#include "udataswp.h"
#include "unormimp.h" /* JAMO_L_BASE etc. */
#include "uprops.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/* dynamically loaded Unicode character properties -------------------------- */

#define UCHAR_HARDCODE_DATA 1

#if UCHAR_HARDCODE_DATA

/* uchar_props_data.c is machine-generated by genprops --csource */
#include "uchar_props_data.c"

#else

/*
 * loaded uprops.dat -
 * for a description of the file format, see icu/source/tools/genprops/store.c
 */
static const char DATA_NAME[] = "uprops";
static const char DATA_TYPE[] = "icu";

static UDataMemory *propsData=NULL;
static UErrorCode dataErrorCode=U_ZERO_ERROR;

static uint8_t formatVersion[4]={ 0, 0, 0, 0 };
static UVersionInfo dataVersion={ 0, 0, 0, 0 };

static UTrie propsTrie={ 0 }, propsVectorsTrie={ 0 };
static const uint32_t *pData32=NULL, *propsVectors=NULL;
static int32_t countPropsVectors=0, propsVectorsColumns=0;

static int8_t havePropsData=0;     /*  == 0   ->  Data has not been loaded.
                                    *   < 0   ->  Error occured attempting to load data.
                                    *   > 0   ->  Data has been successfully loaded.
                                    */

/* index values loaded from uprops.dat */
static int32_t indexes[UPROPS_INDEX_COUNT];

static UBool U_CALLCONV
isAcceptable(void *context,
             const char *type, const char *name,
             const UDataInfo *pInfo) {
    if(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x55 &&   /* dataFormat="UPro" */
        pInfo->dataFormat[1]==0x50 &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6f &&
        pInfo->formatVersion[0]==4 &&
        pInfo->formatVersion[2]==UTRIE_SHIFT &&
        pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT
    ) {
        uprv_memcpy(formatVersion, pInfo->formatVersion, 4);
        uprv_memcpy(dataVersion, pInfo->dataVersion, 4);
        return TRUE;
    } else {
        return FALSE;
    }
}

static UBool U_CALLCONV uchar_cleanup(void)
{
    if (propsData) {
        udata_close(propsData);
        propsData=NULL;
    }
    pData32=NULL;
    propsVectors=NULL;
    countPropsVectors=0;
    uprv_memset(dataVersion, 0, U_MAX_VERSION_LENGTH);
    dataErrorCode=U_ZERO_ERROR;
    havePropsData=0;

    return TRUE;
}

struct UCharProps {
    UDataMemory *propsData;
    UTrie propsTrie, propsVectorsTrie;
    const uint32_t *pData32;
};
typedef struct UCharProps UCharProps;

/* open uprops.icu */
static void
_openProps(UCharProps *ucp, UErrorCode *pErrorCode) {
    const uint32_t *p;
    int32_t length;

    ucp->propsData=udata_openChoice(NULL, DATA_TYPE, DATA_NAME, isAcceptable, NULL, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    ucp->pData32=p=(const uint32_t *)udata_getMemory(ucp->propsData);

    /* unserialize the trie; it is directly after the int32_t indexes[UPROPS_INDEX_COUNT] */
    length=(int32_t)p[UPROPS_PROPS32_INDEX]*4;
    length=utrie_unserialize(&ucp->propsTrie, (const uint8_t *)(p+UPROPS_INDEX_COUNT), length-64, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    /* unserialize the properties vectors trie */
    length=(int32_t)(p[UPROPS_ADDITIONAL_VECTORS_INDEX]-p[UPROPS_ADDITIONAL_TRIE_INDEX])*4;
    if(length>0) {
        length=utrie_unserialize(&ucp->propsVectorsTrie, (const uint8_t *)(p+p[UPROPS_ADDITIONAL_TRIE_INDEX]), length, pErrorCode);
    }
    if(length<=0 || U_FAILURE(*pErrorCode)) {
        /*
         * length==0:
         * Allow the properties vectors trie to be missing -
         * also requires propsVectorsColumns=indexes[UPROPS_ADDITIONAL_VECTORS_COLUMNS_INDEX]
         * to be zero so that this trie is never accessed.
         */
        uprv_memset(&ucp->propsVectorsTrie, 0, sizeof(ucp->propsVectorsTrie));
    }
}

#endif

#if !UCHAR_HARDCODE_DATA
static int8_t
uprv_loadPropsData(UErrorCode *pErrorCode) {
    /* load Unicode character properties data from file if necessary */

    /*
     * This lazy intialization with double-checked locking (without mutex protection for
     * haveNormData==0) is transiently unsafe under certain circumstances.
     * Check the readme and use u_init() if necessary.
     */
    if(havePropsData==0) {
        UCharProps ucp={ NULL };

        if(U_FAILURE(*pErrorCode)) {
            return havePropsData;
        }

        /* open the data outside the mutex block */
        _openProps(&ucp, pErrorCode);

        if(U_SUCCESS(*pErrorCode)) {
            /* in the mutex block, set the data for this process */
            umtx_lock(NULL);
            if(propsData==NULL) {
                propsData=ucp.propsData;
                ucp.propsData=NULL;
                pData32=ucp.pData32;
                ucp.pData32=NULL;
                uprv_memcpy(&propsTrie, &ucp.propsTrie, sizeof(propsTrie));
                uprv_memcpy(&propsVectorsTrie, &ucp.propsVectorsTrie, sizeof(propsVectorsTrie));
            }

            /* initialize some variables */
            uprv_memcpy(indexes, pData32, sizeof(indexes));

            /* additional properties */
            if(indexes[UPROPS_ADDITIONAL_VECTORS_INDEX]!=0) {
                propsVectors=pData32+indexes[UPROPS_ADDITIONAL_VECTORS_INDEX];
                countPropsVectors=indexes[UPROPS_RESERVED_INDEX]-indexes[UPROPS_ADDITIONAL_VECTORS_INDEX];
                propsVectorsColumns=indexes[UPROPS_ADDITIONAL_VECTORS_COLUMNS_INDEX];
            }

            havePropsData=1;
            umtx_unlock(NULL);
        } else {
            dataErrorCode=*pErrorCode;
            havePropsData=-1;
        }
        ucln_common_registerCleanup(UCLN_COMMON_UCHAR, uchar_cleanup);

        /* if a different thread set it first, then close the extra data */
        udata_close(ucp.propsData); /* NULL if it was set correctly */
    }

    return havePropsData;
}

static int8_t 
loadPropsData(void) {
    UErrorCode   errorCode = U_ZERO_ERROR;
    int8_t       retVal    = uprv_loadPropsData(&errorCode);
    return retVal;
}

#endif

/* constants and macros for access to the data ------------------------------ */

/* getting a uint32_t properties word from the data */
#if UCHAR_HARDCODE_DATA

#define GET_PROPS(c, result) ((result)=UTRIE2_GET16(&propsTrie, c));

#else

#define HAVE_DATA (havePropsData>0 || loadPropsData()>0)
#define GET_PROPS_UNSAFE(c, result) \
    UTRIE_GET16(&propsTrie, c, result);
#define GET_PROPS(c, result) \
    if(HAVE_DATA) { \
        GET_PROPS_UNSAFE(c, result); \
    } else { \
        (result)=0; \
    }

#endif

U_CFUNC UBool
uprv_haveProperties(UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return FALSE;
    }
#if !UCHAR_HARDCODE_DATA
    if(havePropsData==0) {
        uprv_loadPropsData(pErrorCode);
    }
    if(havePropsData<0) {
        *pErrorCode=dataErrorCode;
        return FALSE;
    }
#endif
    return TRUE;
}

/* API functions ------------------------------------------------------------ */

/* Gets the Unicode character's general category.*/
U_CAPI int8_t U_EXPORT2
u_charType(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (int8_t)GET_CATEGORY(props);
}

/* Enumerate all code points with their general categories. */
struct _EnumTypeCallback {
    UCharEnumTypeRange *enumRange;
    const void *context;
};

static uint32_t U_CALLCONV
_enumTypeValue(const void *context, uint32_t value) {
    return GET_CATEGORY(value);
}

static UBool U_CALLCONV
_enumTypeRange(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    /* just cast the value to UCharCategory */
    return ((struct _EnumTypeCallback *)context)->
        enumRange(((struct _EnumTypeCallback *)context)->context,
                  start, end+1, (UCharCategory)value);
}

U_CAPI void U_EXPORT2
u_enumCharTypes(UCharEnumTypeRange *enumRange, const void *context) {
    struct _EnumTypeCallback callback;

    if(enumRange==NULL
#if !UCHAR_HARDCODE_DATA
        || !HAVE_DATA
#endif
    ) {
        return;
    }

    callback.enumRange=enumRange;
    callback.context=context;
    utrie2_enum(&propsTrie, _enumTypeValue, _enumTypeRange, &callback);
}

/* Checks if ch is a lower case letter.*/
U_CAPI UBool U_EXPORT2
u_islower(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_LOWERCASE_LETTER);
}

/* Checks if ch is an upper case letter.*/
U_CAPI UBool U_EXPORT2
u_isupper(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_UPPERCASE_LETTER);
}

/* Checks if ch is a title case letter; usually upper case letters.*/
U_CAPI UBool U_EXPORT2
u_istitle(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_TITLECASE_LETTER);
}

/* Checks if ch is a decimal digit. */
U_CAPI UBool U_EXPORT2
u_isdigit(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_DECIMAL_DIGIT_NUMBER);
}

U_CAPI UBool U_EXPORT2
u_isxdigit(UChar32 c) {
    uint32_t props;

    /* check ASCII and Fullwidth ASCII a-fA-F */
    if(
        (c<=0x66 && c>=0x41 && (c<=0x46 || c>=0x61)) ||
        (c>=0xff21 && c<=0xff46 && (c<=0xff26 || c>=0xff41))
    ) {
        return TRUE;
    }

    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)==U_DECIMAL_DIGIT_NUMBER);
}

/* Checks if the Unicode character is a letter.*/
U_CAPI UBool U_EXPORT2
u_isalpha(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&U_GC_L_MASK)!=0);
}

U_CAPI UBool U_EXPORT2
u_isUAlphabetic(UChar32 c) {
    return (u_getUnicodeProperties(c, 1)&U_MASK(UPROPS_ALPHABETIC))!=0;
}

/* Checks if c is a letter or a decimal digit */
U_CAPI UBool U_EXPORT2
u_isalnum(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_L_MASK|U_GC_ND_MASK))!=0);
}

/**
 * Checks if c is alphabetic, or a decimal digit; implements UCHAR_POSIX_ALNUM.
 * @internal
 */
U_CFUNC UBool
u_isalnumPOSIX(UChar32 c) {
    return (UBool)(u_isUAlphabetic(c) || u_isdigit(c));
}

/* Checks if ch is a unicode character with assigned character type.*/
U_CAPI UBool U_EXPORT2
u_isdefined(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(GET_CATEGORY(props)!=0);
}

/* Checks if the Unicode character is a base form character that can take a diacritic.*/
U_CAPI UBool U_EXPORT2
u_isbase(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_L_MASK|U_GC_N_MASK|U_GC_MC_MASK|U_GC_ME_MASK))!=0);
}

/* Checks if the Unicode character is a control character.*/
U_CAPI UBool U_EXPORT2
u_iscntrl(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_CC_MASK|U_GC_CF_MASK|U_GC_ZL_MASK|U_GC_ZP_MASK))!=0);
}

U_CAPI UBool U_EXPORT2
u_isISOControl(UChar32 c) {
    return (uint32_t)c<=0x9f && (c<=0x1f || c>=0x7f);
}

/* Some control characters that are used as space. */
#define IS_THAT_CONTROL_SPACE(c) \
    (c<=0x9f && ((c>=TAB && c<=CR) || (c>=0x1c && c <=0x1f) || c==NL))

/* Checks if the Unicode character is a space character.*/
U_CAPI UBool U_EXPORT2
u_isspace(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&U_GC_Z_MASK)!=0 || IS_THAT_CONTROL_SPACE(c));
}

U_CAPI UBool U_EXPORT2
u_isJavaSpaceChar(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&U_GC_Z_MASK)!=0);
}

/* Checks if the Unicode character is a whitespace character.*/
U_CAPI UBool U_EXPORT2
u_isWhitespace(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
                ((CAT_MASK(props)&U_GC_Z_MASK)!=0 &&
                    c!=NBSP && c!=FIGURESP && c!=NNBSP) || /* exclude no-break spaces */
                IS_THAT_CONTROL_SPACE(c)
           );
}

U_CAPI UBool U_EXPORT2
u_isblank(UChar32 c) {
    if((uint32_t)c<=0x9f) {
        return c==9 || c==0x20; /* TAB or SPACE */
    } else {
        /* Zs */
        uint32_t props;
        GET_PROPS(c, props);
        return (UBool)(GET_CATEGORY(props)==U_SPACE_SEPARATOR);
    }
}

U_CAPI UBool U_EXPORT2
u_isUWhiteSpace(UChar32 c) {
    return (u_getUnicodeProperties(c, 1)&U_MASK(UPROPS_WHITE_SPACE))!=0;
}

/* Checks if the Unicode character is printable.*/
U_CAPI UBool U_EXPORT2
u_isprint(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    /* comparing ==0 returns FALSE for the categories mentioned */
    return (UBool)((CAT_MASK(props)&U_GC_C_MASK)==0);
}

/**
 * Checks if c is in \p{graph}\p{blank} - \p{cntrl}.
 * Implements UCHAR_POSIX_PRINT.
 * @internal
 */
U_CFUNC UBool
u_isprintPOSIX(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    /*
     * The only cntrl character in graph+blank is TAB (in blank).
     * Here we implement (blank-TAB)=Zs instead of calling u_isblank().
     */
    return (UBool)((GET_CATEGORY(props)==U_SPACE_SEPARATOR) || u_isgraphPOSIX(c));
}

U_CAPI UBool U_EXPORT2
u_isgraph(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    /* comparing ==0 returns FALSE for the categories mentioned */
    return (UBool)((CAT_MASK(props)&
                    (U_GC_CC_MASK|U_GC_CF_MASK|U_GC_CS_MASK|U_GC_CN_MASK|U_GC_Z_MASK))
                   ==0);
}

/**
 * Checks if c is in
 * [^\p{space}\p{gc=Control}\p{gc=Surrogate}\p{gc=Unassigned}]
 * with space=\p{Whitespace} and Control=Cc.
 * Implements UCHAR_POSIX_GRAPH.
 * @internal
 */
U_CFUNC UBool
u_isgraphPOSIX(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    /* \p{space}\p{gc=Control} == \p{gc=Z}\p{Control} */
    /* comparing ==0 returns FALSE for the categories mentioned */
    return (UBool)((CAT_MASK(props)&
                    (U_GC_CC_MASK|U_GC_CS_MASK|U_GC_CN_MASK|U_GC_Z_MASK))
                   ==0);
}

U_CAPI UBool U_EXPORT2
u_ispunct(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&U_GC_P_MASK)!=0);
}

/* Checks if the Unicode character can start a Unicode identifier.*/
U_CAPI UBool U_EXPORT2
u_isIDStart(UChar32 c) {
    /* same as u_isalpha() */
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_L_MASK|U_GC_NL_MASK))!=0);
}

/* Checks if the Unicode character can be a Unicode identifier part other than starting the
 identifier.*/
U_CAPI UBool U_EXPORT2
u_isIDPart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
           (CAT_MASK(props)&
            (U_GC_ND_MASK|U_GC_NL_MASK|
             U_GC_L_MASK|
             U_GC_PC_MASK|U_GC_MC_MASK|U_GC_MN_MASK)
           )!=0 ||
           u_isIDIgnorable(c));
}

/*Checks if the Unicode character can be ignorable in a Java or Unicode identifier.*/
U_CAPI UBool U_EXPORT2
u_isIDIgnorable(UChar32 c) {
    if(c<=0x9f) {
        return u_isISOControl(c) && !IS_THAT_CONTROL_SPACE(c);
    } else {
        uint32_t props;
        GET_PROPS(c, props);
        return (UBool)(GET_CATEGORY(props)==U_FORMAT_CHAR);
    }
}

/*Checks if the Unicode character can start a Java identifier.*/
U_CAPI UBool U_EXPORT2
u_isJavaIDStart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)((CAT_MASK(props)&(U_GC_L_MASK|U_GC_SC_MASK|U_GC_PC_MASK))!=0);
}

/*Checks if the Unicode character can be a Java identifier part other than starting the
 * identifier.
 */
U_CAPI UBool U_EXPORT2
u_isJavaIDPart(UChar32 c) {
    uint32_t props;
    GET_PROPS(c, props);
    return (UBool)(
           (CAT_MASK(props)&
            (U_GC_ND_MASK|U_GC_NL_MASK|
             U_GC_L_MASK|
             U_GC_SC_MASK|U_GC_PC_MASK|
             U_GC_MC_MASK|U_GC_MN_MASK)
           )!=0 ||
           u_isIDIgnorable(c));
}

U_CAPI int32_t U_EXPORT2
u_charDigitValue(UChar32 c) {
    uint32_t props;
    int32_t value;
    GET_PROPS(c, props);
    value=(int32_t)GET_NUMERIC_TYPE_VALUE(props)-UPROPS_NTV_DECIMAL_START;
    if(0<=value && value<=9) {
        return value;
    } else {
        return -1;
    }
}

U_CAPI double U_EXPORT2
u_getNumericValue(UChar32 c) {
    uint32_t props;
    int32_t ntv;
    GET_PROPS(c, props);
    ntv=(int32_t)GET_NUMERIC_TYPE_VALUE(props);

    if(ntv==UPROPS_NTV_NONE) {
        return U_NO_NUMERIC_VALUE;
    } else if(ntv<UPROPS_NTV_DIGIT_START) {
        /* decimal digit */
        return ntv-UPROPS_NTV_DECIMAL_START;
    } else if(ntv<UPROPS_NTV_NUMERIC_START) {
        /* other digit */
        return ntv-UPROPS_NTV_DIGIT_START;
    } else if(ntv<UPROPS_NTV_FRACTION_START) {
        /* small integer */
        return ntv-UPROPS_NTV_NUMERIC_START;
    } else if(ntv<UPROPS_NTV_LARGE_START) {
        /* fraction */
        int32_t numerator=(ntv>>4)-12;
        int32_t denominator=(ntv&0xf)+1;
        return (double)numerator/denominator;
    } else if(ntv<UPROPS_NTV_RESERVED_START) {
        /* large, single-significant-digit integer */
        double numValue;
        int32_t mant=(ntv>>5)-14;
        int32_t exp=(ntv&0x1f)+2;
        numValue=mant;

        /* multiply by 10^exp without math.h */
        while(exp>=4) {
            numValue*=10000.;
            exp-=4;
        }
        switch(exp) {
        case 3:
            numValue*=1000.;
            break;
        case 2:
            numValue*=100.;
            break;
        case 1:
            numValue*=10.;
            break;
        case 0:
        default:
            break;
        }

        return numValue;
    } else {
        /* reserved */
        return U_NO_NUMERIC_VALUE;
    }
}

/* ICU 3.4: bidi/shaping properties moved to ubidi_props.c */

/* ICU 2.1: u_getCombiningClass() moved to unorm.cpp */

U_CAPI int32_t U_EXPORT2
u_digit(UChar32 ch, int8_t radix) {
    int8_t value;
    if((uint8_t)(radix-2)<=(36-2)) {
        value=(int8_t)u_charDigitValue(ch);
        if(value<0) {
            /* ch is not a decimal digit, try latin letters */
            if(ch>=0x61 && ch<=0x7A) {
                value=(int8_t)(ch-0x57);  /* ch - 'a' + 10 */
            } else if(ch>=0x41 && ch<=0x5A) {
                value=(int8_t)(ch-0x37);  /* ch - 'A' + 10 */
            } else if(ch>=0xFF41 && ch<=0xFF5A) {
                value=(int8_t)(ch-0xFF37);  /* fullwidth ASCII a-z */
            } else if(ch>=0xFF21 && ch<=0xFF3A) {
                value=(int8_t)(ch-0xFF17);  /* fullwidth ASCII A-Z */
            }
        }
    } else {
        value=-1;   /* invalid radix */
    }
    return (int8_t)((value<radix) ? value : -1);
}

U_CAPI UChar32 U_EXPORT2
u_forDigit(int32_t digit, int8_t radix) {
    if((uint8_t)(radix-2)>(36-2) || (uint32_t)digit>=(uint32_t)radix) {
        return 0;
    } else if(digit<10) {
        return (UChar32)(0x30+digit);
    } else {
        return (UChar32)((0x61-10)+digit);
    }
}

/* miscellaneous, and support for uprops.c ---------------------------------- */

U_CAPI void U_EXPORT2
u_getUnicodeVersion(UVersionInfo versionArray) {
    if(versionArray!=NULL) {
        uprv_memcpy(versionArray, dataVersion, U_MAX_VERSION_LENGTH);
    }
}

U_CFUNC uint32_t
u_getUnicodeProperties(UChar32 c, int32_t column) {
    uint16_t vecIndex;

    if(column==-1) {
        uint32_t props;
        GET_PROPS(c, props);
        return props;
    } else if(
#if !UCHAR_HARDCODE_DATA
               !HAVE_DATA || countPropsVectors==0 ||
#endif
               column<0 || column>=propsVectorsColumns
    ) {
        return 0;
    } else {
        vecIndex=UTRIE2_GET16(&propsVectorsTrie, c);
        return propsVectors[vecIndex+column];
    }
}

U_CFUNC int32_t
uprv_getMaxValues(int32_t column) {
#if !UCHAR_HARDCODE_DATA
    if(HAVE_DATA) {
#endif
        switch(column) {
        case 0:
            return indexes[UPROPS_MAX_VALUES_INDEX];
        case 2:
            return indexes[UPROPS_MAX_VALUES_2_INDEX];
        default:
            return 0;
        }
#if !UCHAR_HARDCODE_DATA
    } else {
        return 0;
    }
#endif
}

U_CAPI void U_EXPORT2
u_charAge(UChar32 c, UVersionInfo versionArray) {
    if(versionArray!=NULL) {
        uint32_t version=u_getUnicodeProperties(c, 0)>>UPROPS_AGE_SHIFT;
        versionArray[0]=(uint8_t)(version>>4);
        versionArray[1]=(uint8_t)(version&0xf);
        versionArray[2]=versionArray[3]=0;
    }
}

U_CAPI UScriptCode U_EXPORT2
uscript_getScript(UChar32 c, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return USCRIPT_INVALID_CODE;
    }
    if((uint32_t)c>0x10ffff) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return USCRIPT_INVALID_CODE;
    }

    return (UScriptCode)(u_getUnicodeProperties(c, 0)&UPROPS_SCRIPT_MASK);
}

U_CAPI UBlockCode U_EXPORT2
ublock_getCode(UChar32 c) {
    return (UBlockCode)((u_getUnicodeProperties(c, 0)&UPROPS_BLOCK_MASK)>>UPROPS_BLOCK_SHIFT);
}

/* property starts for UnicodeSet ------------------------------------------- */

static UBool U_CALLCONV
_enumPropertyStartsRange(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    /* add the start code point to the USet */
    const USetAdder *sa=(const USetAdder *)context;
    sa->add(sa->set, start);
    return TRUE;
}

#define USET_ADD_CP_AND_NEXT(sa, cp) sa->add(sa->set, cp); sa->add(sa->set, cp+1)

U_CFUNC void U_EXPORT2
uchar_addPropertyStarts(const USetAdder *sa, UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

#if !UCHAR_HARDCODE_DATA
    if(!HAVE_DATA) {
        *pErrorCode=dataErrorCode;
        return;
    }
#endif

    /* add the start code point of each same-value range of the main trie */
    utrie2_enum(&propsTrie, NULL, _enumPropertyStartsRange, sa);

    /* add code points with hardcoded properties, plus the ones following them */

    /* add for u_isblank() */
    USET_ADD_CP_AND_NEXT(sa, TAB);

    /* add for IS_THAT_CONTROL_SPACE() */
    sa->add(sa->set, CR+1); /* range TAB..CR */
    sa->add(sa->set, 0x1c);
    sa->add(sa->set, 0x1f+1);
    USET_ADD_CP_AND_NEXT(sa, NL);

    /* add for u_isIDIgnorable() what was not added above */
    sa->add(sa->set, DEL); /* range DEL..NBSP-1, NBSP added below */
    sa->add(sa->set, HAIRSP);
    sa->add(sa->set, RLM+1);
    sa->add(sa->set, INHSWAP);
    sa->add(sa->set, NOMDIG+1);
    USET_ADD_CP_AND_NEXT(sa, ZWNBSP);

    /* add no-break spaces for u_isWhitespace() what was not added above */
    USET_ADD_CP_AND_NEXT(sa, NBSP);
    USET_ADD_CP_AND_NEXT(sa, FIGURESP);
    USET_ADD_CP_AND_NEXT(sa, NNBSP);

    /* add for u_digit() */
    sa->add(sa->set, U_a);
    sa->add(sa->set, U_z+1);
    sa->add(sa->set, U_A);
    sa->add(sa->set, U_Z+1);
    sa->add(sa->set, U_FW_a);
    sa->add(sa->set, U_FW_z+1);
    sa->add(sa->set, U_FW_A);
    sa->add(sa->set, U_FW_Z+1);

    /* add for u_isxdigit() */
    sa->add(sa->set, U_f+1);
    sa->add(sa->set, U_F+1);
    sa->add(sa->set, U_FW_f+1);
    sa->add(sa->set, U_FW_F+1);

    /* add for UCHAR_DEFAULT_IGNORABLE_CODE_POINT what was not added above */
    sa->add(sa->set, WJ); /* range WJ..NOMDIG */
    sa->add(sa->set, 0xfff0);
    sa->add(sa->set, 0xfffb+1);
    sa->add(sa->set, 0xe0000);
    sa->add(sa->set, 0xe0fff+1);

    /* add for UCHAR_GRAPHEME_BASE and others */
    USET_ADD_CP_AND_NEXT(sa, CGJ);
}

U_CFUNC void U_EXPORT2
upropsvec_addPropertyStarts(const USetAdder *sa, UErrorCode *pErrorCode) {
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

#if !UCHAR_HARDCODE_DATA
    if(!HAVE_DATA) {
        *pErrorCode=dataErrorCode;
        return;
    }
#endif

    /* add the start code point of each same-value range of the properties vectors trie */
    if(propsVectorsColumns>0) {
        /* if propsVectorsColumns==0 then the properties vectors trie may not be there at all */
        utrie2_enum(&propsVectorsTrie, NULL, _enumPropertyStartsRange, sa);
    }
}
