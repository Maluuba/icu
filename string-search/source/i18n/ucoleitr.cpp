/*
******************************************************************************
*   Copyright (C) 2001-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
******************************************************************************
*
* File ucoleitr.cpp
*
* Modification History:
*
* Date        Name        Description
* 02/15/2001  synwee      Modified all methods to process its own function 
*                         instead of calling the equivalent c++ api (coleitr.h)
******************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucoleitr.h"
#include "unicode/ustring.h"
#include "unicode/sortkey.h"
#include "unicode/uobject.h"
#include "ucol_imp.h"
#include "cmemory.h"

U_NAMESPACE_USE

#define BUFFER_LENGTH             100

#define DEFAULT_BUFFER_SIZE 16
#define BUFFER_GROW 8

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define ARRAY_COPY(dst, src, count) uprv_memcpy((void *) (dst), (void *) (src), (count) * sizeof (src)[0])

#define NEW_ARRAY(type, count) (type *) uprv_malloc((count) * sizeof(type))

#define GROW_ARRAY(array, newSize) uprv_realloc((void *) (array), (newSize) * sizeof (array)[0])

#define DELETE_ARRAY(array) uprv_free((void *) (array))

typedef struct collIterate collIterator;

struct RCEI
{
    uint32_t ce;
    int32_t  ix;
};

struct RCEBuffer
{
    RCEI    defaultBuffer[DEFAULT_BUFFER_SIZE];
    RCEI   *buffer;
    int32_t bufferIndex;
    int32_t bufferSize;

    RCEBuffer();
    ~RCEBuffer();

    UBool empty();
    void  put(uint32_t ce, int32_t ix);
    RCEI *get();
};

RCEBuffer::RCEBuffer()
{
    buffer = defaultBuffer;
    bufferIndex = 0;
    bufferSize = DEFAULT_BUFFER_SIZE;
}

RCEBuffer::~RCEBuffer()
{
    if (buffer != defaultBuffer) {
        DELETE_ARRAY(buffer);
    }
}

UBool RCEBuffer::empty()
{
    return bufferIndex <= 0;
}

void RCEBuffer::put(uint32_t ce, int32_t ix)
{
    if (bufferIndex >= bufferSize) {
        RCEI *newBuffer = NEW_ARRAY(RCEI, bufferSize + BUFFER_GROW);

        ARRAY_COPY(newBuffer, buffer, bufferSize);

        if (buffer != defaultBuffer) {
            DELETE_ARRAY(buffer);
        }

        buffer = newBuffer;
        bufferSize += BUFFER_GROW;
    }

    buffer[bufferIndex].ce = ce;
    buffer[bufferIndex].ix = ix;

    bufferIndex += 1;
}

RCEI *RCEBuffer::get()
{
    if (bufferIndex > 0) {
     return &buffer[--bufferIndex];
    }

    return NULL;
}

struct PCEI
{
    uint64_t ce;
    int32_t  ix;
};

struct PCEBuffer
{
    PCEI    defaultBuffer[DEFAULT_BUFFER_SIZE];
    PCEI   *buffer;
    int32_t bufferIndex;
    int32_t bufferSize;

    PCEBuffer();
    ~PCEBuffer();

    void  reset();
    UBool empty();
    void  put(uint64_t ce, int32_t ix);
    PCEI *get();
};

PCEBuffer::PCEBuffer()
{
    buffer = defaultBuffer;
    bufferIndex = 0;
    bufferSize = DEFAULT_BUFFER_SIZE;
}

PCEBuffer::~PCEBuffer()
{
    if (buffer != defaultBuffer) {
        DELETE_ARRAY(buffer);
    }
}

void PCEBuffer::reset()
{
    bufferIndex = 0;
}

UBool PCEBuffer::empty()
{
    return bufferIndex <= 0;
}

void PCEBuffer::put(uint64_t ce, int32_t ix)
{
    if (bufferIndex >= bufferSize) {
        PCEI *newBuffer = NEW_ARRAY(PCEI, bufferSize + BUFFER_GROW);

        ARRAY_COPY(newBuffer, buffer, bufferSize);

        if (buffer != defaultBuffer) {
            DELETE_ARRAY(buffer);
        }

        buffer = newBuffer;
        bufferSize += BUFFER_GROW;
    }

    buffer[bufferIndex].ce = ce;
    buffer[bufferIndex].ix = ix;

    bufferIndex += 1;
}

PCEI *PCEBuffer::get()
{
    if (bufferIndex > 0) {
     return &buffer[--bufferIndex];
    }

    return NULL;
}

/*
 * This inherits from UObject so that
 * it can be allocated by new and the
 * constructor for PCEBuffer is called.
 */
struct UCollationPCE : public UObject
{
    PCEBuffer          pceBuffer;
    UCollationStrength strength;
    UBool              toShift;
    UBool              isShifted;
    uint32_t           variableTop;

    UCollationPCE(UCollationElements *elems);
    ~UCollationPCE();

    virtual UClassID getDynamicClassID() const;
    static UClassID getStaticClassID();
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UCollationPCE)

UCollationPCE::UCollationPCE(UCollationElements *elems)
{
    const UCollator *coll = elems->iteratordata_.coll;
    UErrorCode status = U_ZERO_ERROR;

    strength    = ucol_getStrength(coll);
    toShift     = ucol_getAttribute(coll, UCOL_ALTERNATE_HANDLING, &status) == UCOL_SHIFTED;
    isShifted   = FALSE;
    variableTop = coll->variableTopValue;
}

UCollationPCE::~UCollationPCE()
{
    // nothing to do
}

inline uint64_t processCE(UCollationElements *elems, uint32_t ce)
{
    uint64_t primary = 0, secondary = 0, tertiary = 0, quaternary = 0;

    // This is clean, but somewhat slow...
    // We could apply the mask to ce and then
    // just get all three orders...
    switch(elems->pce->strength) {
    default:
        tertiary = ucol_tertiaryOrder(ce);
        /* note fall-through */

    case UCOL_SECONDARY:
        secondary = ucol_secondaryOrder(ce);
        /* note fall-through */

    case UCOL_PRIMARY:
        primary = ucol_primaryOrder(ce);
    }

    // Continuation?
    if (elems->pce->toShift && (elems->pce->variableTop > ce && primary != 0)
                || (elems->pce->isShifted && primary == 0)) {

        if (primary == 0) {
            return UCOL_IGNORABLE;
        }

        if (elems->pce->strength >= UCOL_QUATERNARY) {
            quaternary = primary;
        }

        primary = secondary = tertiary = 0;
        elems->pce->isShifted = TRUE;
    } else {
        if (elems->pce->strength >= UCOL_QUATERNARY) {
            quaternary = 0xFFFF;
        }

        elems->pce->isShifted = FALSE;
    }


    return primary << 48 | secondary << 32 | tertiary << 16 | quaternary;
}

/* public methods ---------------------------------------------------- */

U_CAPI UCollationElements* U_EXPORT2
ucol_openElements(const UCollator  *coll,
                  const UChar      *text,
                        int32_t    textLength,
                        UErrorCode *status)
{
  UCollationElements *result;

  if (U_FAILURE(*status)) {
    return NULL;
  }

  result = (UCollationElements *)uprv_malloc(sizeof(UCollationElements));
  /* test for NULL */
  if (result == NULL) {
      *status = U_MEMORY_ALLOCATION_ERROR;
      return NULL;
  }

  result->reset_     = TRUE;
  result->isWritable = FALSE;
  result->pce        = NULL;

  if (text == NULL) {
      textLength = 0;
  }
  uprv_init_collIterate(coll, text, textLength, &result->iteratordata_);

  return result;
}

U_CAPI void U_EXPORT2
ucol_closeElements(UCollationElements *elems)
{
  collIterate *ci = &elems->iteratordata_;

  if (ci->writableBuffer != ci->stackWritableBuffer) {
    uprv_free(ci->writableBuffer);
  }

  if (elems->isWritable && elems->iteratordata_.string != NULL)
  {
    uprv_free(elems->iteratordata_.string);
  }

  if (ci->extendCEs) {
	  uprv_free(ci->extendCEs);
  }

  if (elems->pce != NULL) {
      delete elems->pce;
  }

  uprv_free(elems);
}

U_CAPI void U_EXPORT2
ucol_reset(UCollationElements *elems)
{
  collIterate *ci = &(elems->iteratordata_);
  elems->reset_   = TRUE;
  ci->pos         = ci->string;
  if ((ci->flags & UCOL_ITER_HASLEN) == 0 || ci->endp == NULL) {
    ci->endp      = ci->string + u_strlen(ci->string);
  }
  ci->CEpos       = ci->toReturn = ci->CEs;
  ci->flags       = UCOL_ITER_HASLEN;
  if (ci->coll->normalizationMode == UCOL_ON) {
    ci->flags |= UCOL_ITER_NORM;
  }
  
  if (ci->stackWritableBuffer != ci->writableBuffer) {
    uprv_free(ci->writableBuffer);
    ci->writableBuffer = ci->stackWritableBuffer;
    ci->writableBufSize = UCOL_WRITABLE_BUFFER_SIZE;
  }
  ci->fcdPosition = NULL;
}

U_CAPI int32_t U_EXPORT2
ucol_next(UCollationElements *elems, 
          UErrorCode         *status)
{
  int32_t result;
  if (U_FAILURE(*status)) {
    return UCOL_NULLORDER;
  }

  elems->reset_ = FALSE;

  result = (int32_t)ucol_getNextCE(elems->iteratordata_.coll,
                                   &elems->iteratordata_, 
                                   status);
  
  if (result == UCOL_NO_MORE_CES) {
    result = UCOL_NULLORDER;
  }
  return result;
}

U_CAPI int64_t U_EXPORT2
ucol_nextProcessed(UCollationElements *elems,
                   int32_t            *srcIdx,
                   UErrorCode         *status)
{
    const UCollator *coll = elems->iteratordata_.coll;
    int64_t result = UCOL_IGNORABLE;
    uint32_t sidx = 0;

    if (U_FAILURE(*status)) {
        return UCOL_PROCESSED_NULLORDER;
    }

    if (elems->pce == NULL) {
        elems->pce = new UCollationPCE(elems);
    } else {
        elems->pce->pceBuffer.reset();
    }

    elems->reset_ = FALSE;

    do {
        sidx = ucol_getOffset(elems);
        uint32_t ce = (uint32_t) ucol_getNextCE(coll, &elems->iteratordata_, status);

        if (ce == UCOL_NO_MORE_CES) {
             result = UCOL_PROCESSED_NULLORDER;
             break;
        }

        result = processCE(elems, ce);
    } while (result == UCOL_IGNORABLE);

    if (srcIdx != NULL) {
        *srcIdx = sidx;
    }

    return result;
}

U_CAPI int32_t U_EXPORT2
ucol_previous(UCollationElements *elems,
              UErrorCode         *status)
{
  if(U_FAILURE(*status)) {
    return UCOL_NULLORDER;
  }
  else
  {
    int32_t result;

    if (elems->reset_ && 
        (elems->iteratordata_.pos == elems->iteratordata_.string)) {
        if (elems->iteratordata_.endp == NULL) {
            elems->iteratordata_.endp = elems->iteratordata_.string + 
                                        u_strlen(elems->iteratordata_.string);
            elems->iteratordata_.flags |= UCOL_ITER_HASLEN;
        }
        elems->iteratordata_.pos = elems->iteratordata_.endp;
        elems->iteratordata_.fcdPosition = elems->iteratordata_.endp;
    }

    elems->reset_ = FALSE;

    result = (int32_t)ucol_getPrevCE(elems->iteratordata_.coll,
                                     &(elems->iteratordata_), 
                                     status);

    if (result == UCOL_NO_MORE_CES) {
      result = UCOL_NULLORDER;
    }

    return result;
  }
}

U_CAPI int64_t U_EXPORT2
ucol_previousProcessed(UCollationElements *elems,
                   int32_t            *srcIdx,
                   UErrorCode         *status)
{
    const UCollator *coll = elems->iteratordata_.coll;
    int64_t result = UCOL_IGNORABLE;
 // int64_t primary = 0, secondary = 0, tertiary = 0, quaternary = 0;
 // UCollationStrength strength = ucol_getStrength(coll);
 //  UBool toShift   = ucol_getAttribute(coll, UCOL_ALTERNATE_HANDLING, status) ==  UCOL_SHIFTED;
 // uint32_t variableTop = coll->variableTopValue;
    uint32_t sidx = 0;

    if (U_FAILURE(*status)) {
        return UCOL_PROCESSED_NULLORDER;
    }

    if (elems->reset_ && 
        (elems->iteratordata_.pos == elems->iteratordata_.string)) {
        if (elems->iteratordata_.endp == NULL) {
            elems->iteratordata_.endp = elems->iteratordata_.string + 
                                        u_strlen(elems->iteratordata_.string);
            elems->iteratordata_.flags |= UCOL_ITER_HASLEN;
        }

        elems->iteratordata_.pos = elems->iteratordata_.endp;
        elems->iteratordata_.fcdPosition = elems->iteratordata_.endp;
    }

    if (elems->pce == NULL) {
        elems->pce = new UCollationPCE(elems);
    } else {
      //elems->pce->pceBuffer.reset();
    }

    elems->reset_ = FALSE;

    while (elems->pce->pceBuffer.empty()) {
        // buffer raw CEs up to non-ignorable primary
        RCEBuffer rceb;
        uint32_t ce;
        
        // **** do we need to reset receb, or will it always be empty at this point ****
        do {
            // ucol_getPrevCE is pre-decrement, so get offset after.
            ce   = ucol_getPrevCE(coll, &elems->iteratordata_, status);
            sidx = ucol_getOffset(elems);

            if (ce == UCOL_NO_MORE_CES) {
                if (! rceb.empty()) {
                    break;
                }

                goto finish;
            }

            rceb.put(ce, sidx);
        } while ((ce & UCOL_PRIMARYMASK) == 0);

        // process the raw CEs
        while (! rceb.empty()) {
            RCEI *rcei = rceb.get();

            result = processCE(elems, rcei->ce);

            if (result != UCOL_IGNORABLE) {
                elems->pce->pceBuffer.put(result, rcei->ix);
            }
        }
    }

finish:
    if (elems->pce->pceBuffer.empty()) {
        // **** set srcIdx? to what? ****
        return UCOL_PROCESSED_NULLORDER;
    }

    PCEI *pcei = elems->pce->pceBuffer.get();

    if (srcIdx != NULL) {
        *srcIdx = pcei->ix;
    }

    return pcei->ce;
}

U_CAPI int32_t U_EXPORT2
ucol_getMaxExpansion(const UCollationElements *elems,
                           int32_t            order)
{
  uint8_t result;
  UCOL_GETMAXEXPANSION(elems->iteratordata_.coll, (uint32_t)order, result);
  return result;
}
 
U_CAPI void U_EXPORT2
ucol_setText(      UCollationElements *elems,
             const UChar              *text,
                   int32_t            textLength,
                   UErrorCode         *status)
{
  if (U_FAILURE(*status)) {
    return;
  }

  if (elems->isWritable && elems->iteratordata_.string != NULL)
  {
    uprv_free(elems->iteratordata_.string);
  }
 
  if (text == NULL) {
      textLength = 0;
  }

  elems->isWritable = FALSE;
  uprv_init_collIterate(elems->iteratordata_.coll, text, textLength, 
                   &elems->iteratordata_);

  elems->reset_   = TRUE;
}

U_CAPI int32_t U_EXPORT2
ucol_getOffset(const UCollationElements *elems)
{
  const collIterate *ci = &(elems->iteratordata_);

  if (ci->returnPos != NULL) {
      return ci->returnPos - ci->string;
  }

  // while processing characters in normalization buffer getOffset will 
  // return the next non-normalized character. 
  // should be inline with the old implementation since the old codes uses
  // nextDecomp in normalizer which also decomposes the string till the 
  // first base character is found.
  if (ci->flags & UCOL_ITER_INNORMBUF) {
      if (ci->fcdPosition == NULL) {
        return 0;
      }
      return (int32_t)(ci->fcdPosition - ci->string);
  }
  else {
      return (int32_t)(ci->pos - ci->string);
  }
}

U_CAPI void U_EXPORT2
ucol_setOffset(UCollationElements    *elems,
               int32_t           offset,
               UErrorCode            *status)
{
  if (U_FAILURE(*status)) {
    return;
  }

  // this methods will clean up any use of the writable buffer and points to 
  // the original string
  collIterate *ci = &(elems->iteratordata_);
  ci->pos         = ci->string + offset;
  ci->CEpos       = ci->toReturn = ci->CEs;
  if (ci->flags & UCOL_ITER_INNORMBUF) {
    ci->flags = ci->origFlags;
  }
  if ((ci->flags & UCOL_ITER_HASLEN) == 0) {
      ci->endp  = ci->string + u_strlen(ci->string);
      ci->flags |= UCOL_ITER_HASLEN;
  }
  ci->fcdPosition = NULL;
  elems->reset_ = FALSE;
}

U_CAPI int32_t U_EXPORT2
ucol_primaryOrder (int32_t order) 
{
  order &= UCOL_PRIMARYMASK;
  return (order >> UCOL_PRIMARYORDERSHIFT);
}

U_CAPI int32_t U_EXPORT2
ucol_secondaryOrder (int32_t order) 
{
  order &= UCOL_SECONDARYMASK;
  return (order >> UCOL_SECONDARYORDERSHIFT);
}

U_CAPI int32_t U_EXPORT2
ucol_tertiaryOrder (int32_t order) 
{
  return (order & UCOL_TERTIARYMASK);
}

#endif /* #if !UCONFIG_NO_COLLATION */
