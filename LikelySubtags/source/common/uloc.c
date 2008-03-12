/*
**********************************************************************
*   Copyright (C) 1997-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File ULOC.CPP
*
* Modification History:
*
*   Date        Name        Description
*   04/01/97    aliu        Creation.
*   08/21/98    stephen     JDK 1.2 sync
*   12/08/98    rtg         New Locale implementation and C API
*   03/15/99    damiba      overhaul.
*   04/06/99    stephen     changed setDefault() to realloc and copy
*   06/14/99    stephen     Changed calls to ures_open for new params
*   07/21/99    stephen     Modified setDefault() to propagate to C++
*   05/14/04    alan        7 years later: refactored, cleaned up, fixed bugs,
*                           brought canonicalization code into line with spec
*****************************************************************************/

/*
   POSIX's locale format, from putil.c: [no spaces]

     ll [ _CC ] [ . MM ] [ @ VV]

     l = lang, C = ctry, M = charmap, V = variant
*/

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"

#include "putilimp.h"
#include "ustr_imp.h"
#include "ulocimp.h"
#include "uresimp.h"
#include "umutex.h"
#include "cstring.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "locmap.h"
#include "uarrsort.h"
#include "uenumimp.h"
#include "uassert.h"

#include <stdio.h> /* for sprintf */

/* ### Declarations **************************************************/

/* Locale stuff from locid.cpp */
U_CFUNC void locale_set_default(const char *id);
U_CFUNC const char *locale_get_default(void);
U_CFUNC int32_t
locale_getKeywords(const char *localeID,
            char prev,
            char *keywords, int32_t keywordCapacity,
            char *values, int32_t valuesCapacity, int32_t *valLen,
            UBool valuesToo,
            UErrorCode *status);

/* ### Constants **************************************************/

/* These strings describe the resources we attempt to load from
 the locale ResourceBundle data file.*/
static const char _kLanguages[]       = "Languages";
static const char _kScripts[]         = "Scripts";
static const char _kCountries[]       = "Countries";
static const char _kVariants[]        = "Variants";
static const char _kKeys[]            = "Keys";
static const char _kTypes[]           = "Types";
static const char _kIndexLocaleName[] = "res_index";
static const char _kRootName[]        = "root";
static const char _kIndexTag[]        = "InstalledLocales";
static const char _kCurrency[]        = "currency";
static const char _kCurrencies[]      = "Currencies";
static char** _installedLocales = NULL;
static int32_t _installedLocalesCount = 0;

/* ### Data tables **************************************************/

/**
 * Table of language codes, both 2- and 3-letter, with preference
 * given to 2-letter codes where possible.  Includes 3-letter codes
 * that lack a 2-letter equivalent.
 *
 * This list must be in sorted order.  This list is returned directly
 * to the user by some API.
 *
 * This list must be kept in sync with LANGUAGES_3, with corresponding
 * entries matched.
 *
 * This table should be terminated with a NULL entry, followed by a
 * second list, and another NULL entry.  The first list is visible to
 * user code when this array is returned by API.  The second list
 * contains codes we support, but do not expose through user API.
 *
 * Notes
 *
 * Tables updated per http://lcweb.loc.gov/standards/iso639-2/ to
 * include the revisions up to 2001/7/27 *CWB*
 *
 * The 3 character codes are the terminology codes like RFC 3066.  This
 * is compatible with prior ICU codes
 *
 * "in" "iw" "ji" "jw" & "sh" have been withdrawn but are still in the
 * table but now at the end of the table because 3 character codes are
 * duplicates.  This avoids bad searches going from 3 to 2 character
 * codes.
 *
 * The range qaa-qtz is reserved for local use
 */
static const char * const LANGUAGES[] = {
    "aa",  "ab",  "ace", "ach", "ada", "ady", "ae",  "af",  "afa",
    "afh", "ain", "ak",  "akk", "ale", "alg", "alt", "am",  "an",  
    "ang", "anp", "apa",
    "ar",  "arc", "arn", "arp", "art", "arw", "as",  "ast",
    "ath", "aus", "av",  "awa", "ay",  "az",  "ba",  "bad",
    "bai", "bal", "ban", "bas", "bat", "be",  "bej",
    "bem", "ber", "bg",  "bh",  "bho", "bi",  "bik", "bin",
    "bla", "bm",  "bn",  "bnt", "bo",  "br",  "bra", "bs",
    "btk", "bua", "bug", "byn", "ca",  "cad", "cai", "car", "cau",
    "cch", "ce",  "ceb", "cel", "ch",  "chb", "chg", "chk", "chm",
    "chn", "cho", "chp", "chr", "chy", "cmc", "co",  "cop",
    "cpe", "cpf", "cpp", "cr",  "crh", "crp", "cs",  "csb", "cu",  "cus",
    "cv",  "cy",  "da",  "dak", "dar", "day", "de",  "del", "den",
    "dgr", "din", "doi", "dra", "dsb", "dua", "dum", "dv",  "dyu",
    "dz",  "ee",  "efi", "egy", "eka", "el",  "elx", "en",
    "enm", "eo",  "es",  "et",  "eu",  "ewo", "fa",
    "fan", "fat", "ff",  "fi",  "fil", "fiu", "fj",  "fo",  "fon",
    "fr",  "frm", "fro", "frr", "frs", "fur", "fy",  
    "ga",  "gaa", "gay", "gba", "gd",  "gem", "gez", "gil", 
    "gl",  "gmh", "gn",  "goh", "gon", "gor", "got", "grb", 
    "grc", "gsw", "gu",  "gv", "gwi", 
    "ha",  "hai", "haw", "he",  "hi",  "hil", "him",
    "hit", "hmn", "ho",  "hr",  "hsb", "ht",  "hu",  "hup", "hy",  "hz",
    "ia",  "iba", "id",  "ie",  "ig",  "ii",  "ijo", "ik",
    "ilo", "inc", "ine", "inh", "io",  "ira", "iro", "is",  "it",
    "iu",  "ja",  "jbo", "jpr", "jrb", "jv",  "ka",  "kaa", "kab",
    "kac", "kaj", "kam", "kar", "kaw", "kbd", "kcg", "kfo", "kg",  "kha", "khi",
    "kho", "ki",  "kj",  "kk",  "kl",  "km",  "kmb", "kn",
    "ko",  "kok", "kos", "kpe", "kr",  "krc", "krl", "kro", "kru", "ks",
    "ku",  "kum", "kut", "kv",  "kw",  "ky",  "la",  "lad",
    "lah", "lam", "lb",  "lez", "lg",  "li",  "ln",  "lo",  "lol",
    "loz", "lt",  "lu",  "lua", "lui", "lun", "luo", "lus",
    "lv",  "mad", "mag", "mai", "mak", "man", "map", "mas",
    "mdf", "mdr", "men", "mg",  "mga", "mh",  "mi",  "mic", "min",
    "mis", "mk",  "mkh", "ml",  "mn",  "mnc", "mni", "mno",
    "mo",  "moh", "mos", "mr",  "ms",  "mt",  "mul", "mun",
    "mus", "mwl", "mwr", "my",  "myn", "myv", "na",  "nah", "nai", "nap",
    "nb",  "nd",  "nds", "ne",  "new", "ng",  "nia", "nic",
    "niu", "nl",  "nn",  "no",  "nog", "non", "nqo", "nr",  "nso", "nub",
    "nv",  "nwc", "ny",  "nym", "nyn", "nyo", "nzi", "oc",  "oj",
    "om",  "or",  "os",  "osa", "ota", "oto", "pa",  "paa",
    "pag", "pal", "pam", "pap", "pau", "peo", "phi", "phn",
    "pi",  "pl",  "pon", "pra", "pro", "ps",  "pt",  "qu",
    "raj", "rap", "rar", "rm",  "rn",  "ro",  "roa", "rom",
    "ru",  "rup", "rw",  "sa",  "sad", "sah", "sai", "sal", "sam",
    "sas", "sat", "sc",  "scn", "sco", "sd",  "se",  "sel", "sem",
    "sg",  "sga", "sgn", "shn", "si",  "sid", "sio", "sit",
    "sk",  "sl",  "sla", "sm",  "sma", "smi", "smj", "smn",
    "sms", "sn",  "snk", "so",  "sog", "son", "sq",  "sr",
    "srn", "srr", "ss",  "ssa", "st",  "su",  "suk", "sus", "sux",
    "sv",  "sw",  "syc", "syr", "ta",  "tai", "te",  "tem", "ter",
    "tet", "tg",  "th",  "ti",  "tig", "tiv", "tk",  "tkl",
    "tl",  "tlh", "tli", "tmh", "tn",  "to",  "tog", "tpi", "tr",
    "ts",  "tsi", "tt",  "tum", "tup", "tut", "tvl", "tw",
    "ty",  "tyv", "udm", "ug",  "uga", "uk",  "umb", "und", "ur",
    "uz",  "vai", "ve",  "vi",  "vo",  "vot", "wa",  "wak",
    "wal", "war", "was", "wen", "wo",  "xal", "xh",  "yao", "yap",
    "yi",  "yo",  "ypk", "za",  "zap", "zbl", "zen", "zh",  "znd",
    "zu",  "zun", "zxx", "zza",
NULL,
    "in",  "iw",  "ji",  "jw",  "sh",    /* obsolete language codes */
NULL
};
static const char* const DEPRECATED_LANGUAGES[]={
    "in", "iw", "ji", "jw", NULL, NULL
};
static const char* const REPLACEMENT_LANGUAGES[]={
    "id", "he", "yi", "jv", NULL, NULL
};

/**
 * Table of 3-letter language codes.
 *
 * This is a lookup table used to convert 3-letter language codes to
 * their 2-letter equivalent, where possible.  It must be kept in sync
 * with LANGUAGES.  For all valid i, LANGUAGES[i] must refer to the
 * same language as LANGUAGES_3[i].  The commented-out lines are
 * copied from LANGUAGES to make eyeballing this baby easier.
 *
 * Where a 3-letter language code has no 2-letter equivalent, the
 * 3-letter code occupies both LANGUAGES[i] and LANGUAGES_3[i].
 *
 * This table should be terminated with a NULL entry, followed by a
 * second list, and another NULL entry.  The two lists correspond to
 * the two lists in LANGUAGES.
 */
static const char * const LANGUAGES_3[] = {
/*  "aa",  "ab",  "ace", "ach", "ada", "ady", "ae",  "af",  "afa",    */
    "aar", "abk", "ace", "ach", "ada", "ady", "ave", "afr", "afa",
/*  "afh", "ain", "ak",  "akk", "ale", "alg", "alt", "am",  "an",  "ang", "anp", "apa",    */
    "afh", "ain", "aka", "akk", "ale", "alg", "alt", "amh", "arg", "ang", "anp", "apa",
/*  "ar",  "arc", "arn", "arp", "art", "arw", "as",  "ast",    */
    "ara", "arc", "arn", "arp", "art", "arw", "asm", "ast",
/*  "ath", "aus", "av",  "awa", "ay",  "az",  "ba",  "bad",    */
    "ath", "aus", "ava", "awa", "aym", "aze", "bak", "bad",
/*  "bai", "bal", "ban", "bas", "bat", "be",  "bej",    */
    "bai", "bal", "ban", "bas", "bat", "bel", "bej",
/*  "bem", "ber", "bg",  "bh",  "bho", "bi",  "bik", "bin",    */
    "bem", "ber", "bul", "bih", "bho", "bis", "bik", "bin",
/*  "bla", "bm",  "bn",  "bnt", "bo",  "br",  "bra", "bs",     */
    "bla", "bam", "ben", "bnt", "bod", "bre", "bra", "bos",
/*  "btk", "bua", "bug", "byn", "ca",  "cad", "cai", "car", "cau",    */
    "btk", "bua", "bug", "byn", "cat", "cad", "cai", "car", "cau",
/*  "cch", "ce",  "ceb", "cel", "ch",  "chb", "chg", "chk", "chm",    */
    "cch", "che", "ceb", "cel", "cha", "chb", "chg", "chk", "chm",
/*  "chn", "cho", "chp", "chr", "chy", "cmc", "co",  "cop",    */
    "chn", "cho", "chp", "chr", "chy", "cmc", "cos", "cop",
/*  "cpe", "cpf", "cpp", "cr",  "crh", "crp", "cs",  "csb", "cu",  "cus",    */
    "cpe", "cpf", "cpp", "cre", "crh", "crp", "ces", "csb", "chu", "cus",
/*  "cv",  "cy",  "da",  "dak", "dar", "day", "de",  "del", "den",    */
    "chv", "cym", "dan", "dak", "dar", "day", "deu", "del", "den",
/*  "dgr", "din", "doi", "dra", "dsb", "dua", "dum", "dv",  "dyu",    */
    "dgr", "din", "doi", "dra", "dsb", "dua", "dum", "div", "dyu",
/*  "dz",  "ee",  "efi", "egy", "eka", "el",  "elx", "en",     */
    "dzo", "ewe", "efi", "egy", "eka", "ell", "elx", "eng",
/*  "enm", "eo",  "es",  "et",  "eu",  "ewo", "fa",     */
    "enm", "epo", "spa", "est", "eus", "ewo", "fas",
/*  "fan", "fat", "ff",  "fi",  "fil", "fiu", "fj",  "fo",  "fon",    */
    "fan", "fat", "ful", "fin", "fil", "fiu", "fij", "fao", "fon",
/*  "fr",  "frm", "fro", "frr", "frs", "fur", "fy",  "ga",  "gaa", "gay",    */
    "fra", "frm", "fro", "frr", "frs", "fur", "fry", "gle", "gaa", "gay",
/*  "gba", "gd",  "gem", "gez", "gil", "gl",  "gmh", "gn",     */
    "gba", "gla", "gem", "gez", "gil", "glg", "gmh", "grn",
/*  "goh", "gon", "gor", "got", "grb", "grc", "gsw", "gu",  "gv",     */
    "goh", "gon", "gor", "got", "grb", "grc", "gsw", "guj", "glv",
/*  "gwi", "ha",  "hai", "haw", "he",  "hi",  "hil", "him",    */
    "gwi", "hau", "hai", "haw", "heb", "hin", "hil", "him",
/*  "hit", "hmn", "ho",  "hr",  "hsb", "ht",  "hu",  "hup", "hy",  "hz",     */
    "hit", "hmn", "hmo", "hrv", "hsb", "hat", "hun", "hup", "hye", "her",
/*  "ia",  "iba", "id",  "ie",  "ig",  "ii",  "ijo", "ik",     */
    "ina", "iba", "ind", "ile", "ibo", "iii", "ijo", "ipk",
/*  "ilo", "inc", "ine", "inh", "io",  "ira", "iro", "is",  "it",      */
    "ilo", "inc", "ine", "inh", "ido", "ira", "iro", "isl", "ita",
/*  "iu",  "ja",  "jbo", "jpr", "jrb", "jv",  "ka",  "kaa", "kab",   */
    "iku", "jpn", "jbo", "jpr", "jrb", "jav", "kat", "kaa", "kab",
/*  "kac", "kaj", "kam", "kar", "kaw", "kbd", "kcg", "kfo", "kg",  "kha", "khi",*/
    "kac", "kaj", "kam", "kar", "kaw", "kbd", "kcg", "kfo", "kg",  "kha", "khi",
/*  "kho", "ki",  "kj",  "kk",  "kl",  "km",  "kmb", "kn",     */
    "kho", "kik", "kua", "kaz", "kal", "khm", "kmb", "kan",
/*  "ko",  "kok", "kos", "kpe", "kr",  "krc", "krl", "kro", "kru", "ks",     */
    "kor", "kok", "kos", "kpe", "kau", "krc", "krl", "kro", "kru", "kas",
/*  "ku",  "kum", "kut", "kv",  "kw",  "ky",  "la",  "lad",    */
    "kur", "kum", "kut", "kom", "cor", "kir", "lat", "lad",
/*  "lah", "lam", "lb",  "lez", "lg",  "li",  "ln",  "lo",  "lol",    */
    "lah", "lam", "ltz", "lez", "lug", "lim", "lin", "lao", "lol",
/*  "loz", "lt",  "lu",  "lua", "lui", "lun", "luo", "lus",    */
    "loz", "lit", "lub", "lua", "lui", "lun", "luo", "lus",
/*  "lv",  "mad", "mag", "mai", "mak", "man", "map", "mas",    */
    "lav", "mad", "mag", "mai", "mak", "man", "map", "mas",
/*  "mdf", "mdr", "men", "mg",  "mga", "mh",  "mi",  "mic", "min",    */
    "mdf", "mdr", "men", "mlg", "mga", "mah", "mri", "mic", "min",
/*  "mis", "mk",  "mkh", "ml",  "mn",  "mnc", "mni", "mno",    */
    "mis", "mkd", "mkh", "mal", "mon", "mnc", "mni", "mno",
/*  "mo",  "moh", "mos", "mr",  "ms",  "mt",  "mul", "mun",    */
    "mol", "moh", "mos", "mar", "msa", "mlt", "mul", "mun",
/*  "mus", "mwl", "mwr", "my",  "myn", "myv", "na",  "nah", "nai", "nap",    */
    "mus", "mwl", "mwr", "mya", "myn", "myv", "nau", "nah", "nai", "nap",
/*  "nb",  "nd",  "nds", "ne",  "new", "ng",  "nia", "nic",    */
    "nob", "nde", "nds", "nep", "new", "ndo", "nia", "nic",
/*  "niu", "nl",  "nn",  "no",  "nog", "non", "nqo", "nr",  "nso", "nub",    */
    "niu", "nld", "nno", "nor", "nog", "non", "nqo", "nbl", "nso", "nub",
/*  "nv",  "nwc", "ny",  "nym", "nyn", "nyo", "nzi", "oc",  "oj",     */
    "nav", "nwc", "nya", "nym", "nyn", "nyo", "nzi", "oci", "oji",
/*  "om",  "or",  "os",  "osa", "ota", "oto", "pa",  "paa",    */
    "orm", "ori", "oss", "osa", "ota", "oto", "pan", "paa",
/*  "pag", "pal", "pam", "pap", "pau", "peo", "phi", "phn",    */
    "pag", "pal", "pam", "pap", "pau", "peo", "phi", "phn",
/*  "pi",  "pl",  "pon", "pra", "pro", "ps",  "pt",  "qu",     */
    "pli", "pol", "pon", "pra", "pro", "pus", "por", "que",
/*  "raj", "rap", "rar", "rm",  "rn",  "ro",  "roa", "rom",    */
    "raj", "rap", "rar", "roh", "run", "ron", "roa", "rom",
/*  "ru",  "rup", "rw",  "sa",  "sad", "sah", "sai", "sal", "sam",    */
    "rus", "rup", "kin", "san", "sad", "sah", "sai", "sal", "sam",
/*  "sas", "sat", "sc",  "scn", "sco", "sd",  "se",  "sel", "sem",    */
    "sas", "sat", "srd", "scn", "sco", "snd", "sme", "sel", "sem",
/*  "sg",  "sga", "sgn", "shn", "si",  "sid", "sio", "sit",    */
    "sag", "sga", "sgn", "shn", "sin", "sid", "sio", "sit",
/*  "sk",  "sl",  "sla", "sm",  "sma", "smi", "smj", "smn",    */
    "slk", "slv", "sla", "smo", "sma", "smi", "smj", "smn",
/*  "sms", "sn",  "snk", "so",  "sog", "son", "sq",  "sr",     */
    "sms", "sna", "snk", "som", "sog", "son", "sqi", "srp",
/*  "srn", "srr", "ss",  "ssa", "st",  "su",  "suk", "sus", "sux",    */
    "srn", "srr", "ssw", "ssa", "sot", "sun", "suk", "sus", "sux",
/*  "sv",  "sw",  "syc", "syr", "ta",  "tai", "te",  "tem", "ter",    */
    "swe", "swa", "syc", "syr", "tam", "tai", "tel", "tem", "ter",
/*  "tet", "tg",  "th",  "ti",  "tig", "tiv", "tk",  "tkl",    */
    "tet", "tgk", "tha", "tir", "tig", "tiv", "tuk", "tkl",
/*  "tl",  "tlh", "tli", "tmh", "tn",  "to",  "tog", "tpi", "tr",     */
    "tgl", "tlh", "tli", "tmh", "tsn", "ton", "tog", "tpi", "tur",
/*  "ts",  "tsi", "tt",  "tum", "tup", "tut", "tvl", "tw",     */
    "tso", "tsi", "tat", "tum", "tup", "tut", "tvl", "twi",
/*  "ty",  "tyv", "udm", "ug",  "uga", "uk",  "umb", "und", "ur",     */
    "tah", "tyv", "udm", "uig", "uga", "ukr", "umb", "und", "urd",
/*  "uz",  "vai", "ve",  "vi",  "vo",  "vot", "wa",  "wak",    */
    "uzb", "vai", "ven", "vie", "vol", "vot", "wln", "wak",
/*  "wal", "war", "was", "wen", "wo",  "xal", "xh",  "yao", "yap",    */
    "wal", "war", "was", "wen", "wol", "xal", "xho", "yao", "yap",
/*  "yi",  "yo",  "ypk", "za",  "zap", "zbl", "zen", "zh",  "znd",    */
    "yid", "yor", "ypk", "zha", "zap", "zbl", "zen", "zho", "znd",
/*  "zu",  "zun", "zxx", "zza",                                         */
    "zul", "zun", "zxx", "zza",
NULL,
/*  "in",  "iw",  "ji",  "jw",  "sh",                          */
    "ind", "heb", "yid", "jaw", "srp",
NULL
};

/**
 * Table of 2-letter country codes.
 *
 * This list must be in sorted order.  This list is returned directly
 * to the user by some API.
 *
 * This list must be kept in sync with COUNTRIES_3, with corresponding
 * entries matched.
 *
 * This table should be terminated with a NULL entry, followed by a
 * second list, and another NULL entry.  The first list is visible to
 * user code when this array is returned by API.  The second list
 * contains codes we support, but do not expose through user API.
 *
 * Notes:
 *
 * ZR(ZAR) is now CD(COD) and FX(FXX) is PS(PSE) as per
 * http://www.evertype.com/standards/iso3166/iso3166-1-en.html added
 * new codes keeping the old ones for compatibility updated to include
 * 1999/12/03 revisions *CWB*
 *
 * RO(ROM) is now RO(ROU) according to
 * http://www.iso.org/iso/en/prods-services/iso3166ma/03updates-on-iso-3166/nlv3e-rou.html
 */
static const char * const COUNTRIES[] = {
    "AD",  "AE",  "AF",  "AG",  "AI",  "AL",  "AM",  "AN",
    "AO",  "AQ",  "AR",  "AS",  "AT",  "AU",  "AW",  "AX",  "AZ",
    "BA",  "BB",  "BD",  "BE",  "BF",  "BG",  "BH",  "BI",
    "BJ",  "BL",  "BM",  "BN",  "BO",  "BR",  "BS",  "BT",  "BV",
    "BW",  "BY",  "BZ",  "CA",  "CC",  "CD",  "CF",  "CG",
    "CH",  "CI",  "CK",  "CL",  "CM",  "CN",  "CO",  "CR",
    "CU",  "CV",  "CX",  "CY",  "CZ",  "DE",  "DJ",  "DK",
    "DM",  "DO",  "DZ",  "EC",  "EE",  "EG",  "EH",  "ER",
    "ES",  "ET",  "FI",  "FJ",  "FK",  "FM",  "FO",  "FR",
    "GA",  "GB",  "GD",  "GE",  "GF",  "GG",  "GH",  "GI",  "GL",
    "GM",  "GN",  "GP",  "GQ",  "GR",  "GS",  "GT",  "GU",
    "GW",  "GY",  "HK",  "HM",  "HN",  "HR",  "HT",  "HU",
    "ID",  "IE",  "IL",  "IM",  "IN",  "IO",  "IQ",  "IR",  "IS",
    "IT",  "JE",  "JM",  "JO",  "JP",  "KE",  "KG",  "KH",  "KI",
    "KM",  "KN",  "KP",  "KR",  "KW",  "KY",  "KZ",  "LA",
    "LB",  "LC",  "LI",  "LK",  "LR",  "LS",  "LT",  "LU",
    "LV",  "LY",  "MA",  "MC",  "MD",  "ME",  "MF",  "MG",  "MH",  "MK",
    "ML",  "MM",  "MN",  "MO",  "MP",  "MQ",  "MR",  "MS",
    "MT",  "MU",  "MV",  "MW",  "MX",  "MY",  "MZ",  "NA",
    "NC",  "NE",  "NF",  "NG",  "NI",  "NL",  "NO",  "NP",
    "NR",  "NU",  "NZ",  "OM",  "PA",  "PE",  "PF",  "PG",
    "PH",  "PK",  "PL",  "PM",  "PN",  "PR",  "PS",  "PT",
    "PW",  "PY",  "QA",  "RE",  "RO",  "RS",  "RU",  "RW",  "SA",
    "SB",  "SC",  "SD",  "SE",  "SG",  "SH",  "SI",  "SJ",
    "SK",  "SL",  "SM",  "SN",  "SO",  "SR",  "ST",  "SV",
    "SY",  "SZ",  "TC",  "TD",  "TF",  "TG",  "TH",  "TJ",
    "TK",  "TL",  "TM",  "TN",  "TO",  "TR",  "TT",  "TV",
    "TW",  "TZ",  "UA",  "UG",  "UM",  "US",  "UY",  "UZ",
    "VA",  "VC",  "VE",  "VG",  "VI",  "VN",  "VU",  "WF",
    "WS",  "YE",  "YT",  "ZA",  "ZM",  "ZW",
NULL,
    "FX",  "CS",  "RO",  "TP",  "YU",  "ZR",   /* obsolete country codes */
NULL
};

static const char* const DEPRECATED_COUNTRIES[] ={
    "BU", "CS", "DY", "FX", "HV", "NH", "RH", "TP", "YU", "ZR", NULL, NULL /* deprecated country list */
};
static const char* const REPLACEMENT_COUNTRIES[] = {
/*  "BU", "CS", "DY", "FX", "HV", "NH", "RH", "TP", "YU", "ZR" */
    "MM", "RS", "BJ", "FR", "BF", "VU", "ZW", "TL", "RS", "CD", NULL, NULL  /* replacement country codes */      
};
    
/**
 * Table of 3-letter country codes.
 *
 * This is a lookup table used to convert 3-letter country codes to
 * their 2-letter equivalent.  It must be kept in sync with COUNTRIES.
 * For all valid i, COUNTRIES[i] must refer to the same country as
 * COUNTRIES_3[i].  The commented-out lines are copied from COUNTRIES
 * to make eyeballing this baby easier.
 *
 * This table should be terminated with a NULL entry, followed by a
 * second list, and another NULL entry.  The two lists correspond to
 * the two lists in COUNTRIES.
 */
static const char * const COUNTRIES_3[] = {
/*  "AD",  "AE",  "AF",  "AG",  "AI",  "AL",  "AM",  "AN",     */
    "AND", "ARE", "AFG", "ATG", "AIA", "ALB", "ARM", "ANT",
/*  "AO",  "AQ",  "AR",  "AS",  "AT",  "AU",  "AW",  "AX",  "AZ",     */
    "AGO", "ATA", "ARG", "ASM", "AUT", "AUS", "ABW", "ALA", "AZE",
/*  "BA",  "BB",  "BD",  "BE",  "BF",  "BG",  "BH",  "BI",     */
    "BIH", "BRB", "BGD", "BEL", "BFA", "BGR", "BHR", "BDI",
/*  "BJ",  "BL",  "BM",  "BN",  "BO",  "BR",  "BS",  "BT",  "BV",     */
    "BEN", "BLM", "BMU", "BRN", "BOL", "BRA", "BHS", "BTN", "BVT",
/*  "BW",  "BY",  "BZ",  "CA",  "CC",  "CD",  "CF",  "CG",     */
    "BWA", "BLR", "BLZ", "CAN", "CCK", "COD", "CAF", "COG",
/*  "CH",  "CI",  "CK",  "CL",  "CM",  "CN",  "CO",  "CR",     */
    "CHE", "CIV", "COK", "CHL", "CMR", "CHN", "COL", "CRI",
/*  "CU",  "CV",  "CX",  "CY",  "CZ",  "DE",  "DJ",  "DK",     */
    "CUB", "CPV", "CXR", "CYP", "CZE", "DEU", "DJI", "DNK",
/*  "DM",  "DO",  "DZ",  "EC",  "EE",  "EG",  "EH",  "ER",     */
    "DMA", "DOM", "DZA", "ECU", "EST", "EGY", "ESH", "ERI",
/*  "ES",  "ET",  "FI",  "FJ",  "FK",  "FM",  "FO",  "FR",     */
    "ESP", "ETH", "FIN", "FJI", "FLK", "FSM", "FRO", "FRA",
/*  "GA",  "GB",  "GD",  "GE",  "GF",  "GG",  "GH",  "GI",  "GL",     */
    "GAB", "GBR", "GRD", "GEO", "GUF", "GGY", "GHA", "GIB", "GRL",
/*  "GM",  "GN",  "GP",  "GQ",  "GR",  "GS",  "GT",  "GU",     */
    "GMB", "GIN", "GLP", "GNQ", "GRC", "SGS", "GTM", "GUM",
/*  "GW",  "GY",  "HK",  "HM",  "HN",  "HR",  "HT",  "HU",     */
    "GNB", "GUY", "HKG", "HMD", "HND", "HRV", "HTI", "HUN",
/*  "ID",  "IE",  "IL",  "IM",  "IN",  "IO",  "IQ",  "IR",  "IS" */
    "IDN", "IRL", "ISR", "IMN", "IND", "IOT", "IRQ", "IRN", "ISL",
/*  "IT",  "JE",  "JM",  "JO",  "JP",  "KE",  "KG",  "KH",  "KI",     */
    "ITA", "JEY", "JAM", "JOR", "JPN", "KEN", "KGZ", "KHM", "KIR",
/*  "KM",  "KN",  "KP",  "KR",  "KW",  "KY",  "KZ",  "LA",     */
    "COM", "KNA", "PRK", "KOR", "KWT", "CYM", "KAZ", "LAO",
/*  "LB",  "LC",  "LI",  "LK",  "LR",  "LS",  "LT",  "LU",     */
    "LBN", "LCA", "LIE", "LKA", "LBR", "LSO", "LTU", "LUX",
/*  "LV",  "LY",  "MA",  "MC",  "MD",  "ME",  "MF",  "MG",  "MH",  "MK",     */
    "LVA", "LBY", "MAR", "MCO", "MDA", "MNE", "MAF", "MDG", "MHL", "MKD",
/*  "ML",  "MM",  "MN",  "MO",  "MP",  "MQ",  "MR",  "MS",     */
    "MLI", "MMR", "MNG", "MAC", "MNP", "MTQ", "MRT", "MSR",
/*  "MT",  "MU",  "MV",  "MW",  "MX",  "MY",  "MZ",  "NA",     */
    "MLT", "MUS", "MDV", "MWI", "MEX", "MYS", "MOZ", "NAM",
/*  "NC",  "NE",  "NF",  "NG",  "NI",  "NL",  "NO",  "NP",     */
    "NCL", "NER", "NFK", "NGA", "NIC", "NLD", "NOR", "NPL",
/*  "NR",  "NU",  "NZ",  "OM",  "PA",  "PE",  "PF",  "PG",     */
    "NRU", "NIU", "NZL", "OMN", "PAN", "PER", "PYF", "PNG",
/*  "PH",  "PK",  "PL",  "PM",  "PN",  "PR",  "PS",  "PT",     */
    "PHL", "PAK", "POL", "SPM", "PCN", "PRI", "PSE", "PRT",
/*  "PW",  "PY",  "QA",  "RE",  "RO",  "RS",  "RU",  "RW",  "SA",     */
    "PLW", "PRY", "QAT", "REU", "ROU", "SRB", "RUS", "RWA", "SAU",
/*  "SB",  "SC",  "SD",  "SE",  "SG",  "SH",  "SI",  "SJ",     */
    "SLB", "SYC", "SDN", "SWE", "SGP", "SHN", "SVN", "SJM",
/*  "SK",  "SL",  "SM",  "SN",  "SO",  "SR",  "ST",  "SV",     */
    "SVK", "SLE", "SMR", "SEN", "SOM", "SUR", "STP", "SLV",
/*  "SY",  "SZ",  "TC",  "TD",  "TF",  "TG",  "TH",  "TJ",     */
    "SYR", "SWZ", "TCA", "TCD", "ATF", "TGO", "THA", "TJK",
/*  "TK",  "TL",  "TM",  "TN",  "TO",  "TR",  "TT",  "TV",     */
    "TKL", "TLS", "TKM", "TUN", "TON", "TUR", "TTO", "TUV",
/*  "TW",  "TZ",  "UA",  "UG",  "UM",  "US",  "UY",  "UZ",     */
    "TWN", "TZA", "UKR", "UGA", "UMI", "USA", "URY", "UZB",
/*  "VA",  "VC",  "VE",  "VG",  "VI",  "VN",  "VU",  "WF",     */
    "VAT", "VCT", "VEN", "VGB", "VIR", "VNM", "VUT", "WLF",
/*  "WS",  "YE",  "YT",  "ZA",  "ZM",  "ZW",          */
    "WSM", "YEM", "MYT", "ZAF", "ZMB", "ZWE",
NULL,
/*  "FX",  "CS",  "RO",  "TP",  "YU",  "ZR",   */
    "FXX", "SCG", "ROM", "TMP", "YUG", "ZAR",
NULL
};

typedef struct CanonicalizationMap {
    const char *id;          /* input ID */
    const char *canonicalID; /* canonicalized output ID */
    const char *keyword;     /* keyword, or NULL if none */
    const char *value;       /* keyword value, or NULL if kw==NULL */
} CanonicalizationMap;

/**
 * A map to canonicalize locale IDs.  This handles a variety of
 * different semantic kinds of transformations.
 */
static const CanonicalizationMap CANONICALIZE_MAP[] = {
    { "",               "en_US_POSIX", NULL, NULL }, /* .NET name */
    { "C",              "en_US_POSIX", NULL, NULL }, /* POSIX name */
    { "posix",          "en_US_POSIX", NULL, NULL }, /* POSIX name (alias of C) */
    { "art_LOJBAN",     "jbo", NULL, NULL }, /* registered name */
    { "az_AZ_CYRL",     "az_Cyrl_AZ", NULL, NULL }, /* .NET name */
    { "az_AZ_LATN",     "az_Latn_AZ", NULL, NULL }, /* .NET name */
    { "ca_ES_PREEURO",  "ca_ES", "currency", "ESP" },
    { "cel_GAULISH",    "cel__GAULISH", NULL, NULL }, /* registered name */
    { "de_1901",        "de__1901", NULL, NULL }, /* registered name */
    { "de_1906",        "de__1906", NULL, NULL }, /* registered name */
    { "de__PHONEBOOK",  "de", "collation", "phonebook" }, /* Old ICU name */
    { "de_AT_PREEURO",  "de_AT", "currency", "ATS" },
    { "de_DE_PREEURO",  "de_DE", "currency", "DEM" },
    { "de_LU_PREEURO",  "de_LU", "currency", "LUF" },
    { "el_GR_PREEURO",  "el_GR", "currency", "GRD" },
    { "en_BOONT",       "en__BOONT", NULL, NULL }, /* registered name */
    { "en_SCOUSE",      "en__SCOUSE", NULL, NULL }, /* registered name */
    { "en_BE_PREEURO",  "en_BE", "currency", "BEF" },
    { "en_IE_PREEURO",  "en_IE", "currency", "IEP" },
    { "es__TRADITIONAL", "es", "collation", "traditional" }, /* Old ICU name */
    { "es_ES_PREEURO",  "es_ES", "currency", "ESP" },
    { "eu_ES_PREEURO",  "eu_ES", "currency", "ESP" },
    { "fi_FI_PREEURO",  "fi_FI", "currency", "FIM" },
    { "fr_BE_PREEURO",  "fr_BE", "currency", "BEF" },
    { "fr_FR_PREEURO",  "fr_FR", "currency", "FRF" },
    { "fr_LU_PREEURO",  "fr_LU", "currency", "LUF" },
    { "ga_IE_PREEURO",  "ga_IE", "currency", "IEP" },
    { "gl_ES_PREEURO",  "gl_ES", "currency", "ESP" },
    { "hi__DIRECT",     "hi", "collation", "direct" }, /* Old ICU name */
    { "it_IT_PREEURO",  "it_IT", "currency", "ITL" },
    { "ja_JP_TRADITIONAL", "ja_JP", "calendar", "japanese" }, /* Old ICU name */
    { "nb_NO_NY",       "nn_NO", NULL, NULL },  /* "markus said this was ok" :-) */
    { "nl_BE_PREEURO",  "nl_BE", "currency", "BEF" },
    { "nl_NL_PREEURO",  "nl_NL", "currency", "NLG" },
    { "pt_PT_PREEURO",  "pt_PT", "currency", "PTE" },
    { "sl_ROZAJ",       "sl__ROZAJ", NULL, NULL }, /* registered name */
    { "sr_SP_CYRL",     "sr_Cyrl_RS", NULL, NULL }, /* .NET name */
    { "sr_SP_LATN",     "sr_Latn_RS", NULL, NULL }, /* .NET name */
    { "sr_YU_CYRILLIC", "sr_Cyrl_RS", NULL, NULL }, /* Linux name */
    { "th_TH_TRADITIONAL", "th_TH", "calendar", "buddhist" }, /* Old ICU name */
    { "uz_UZ_CYRILLIC", "uz_Cyrl_UZ", NULL, NULL }, /* Linux name */
    { "uz_UZ_CYRL",     "uz_Cyrl_UZ", NULL, NULL }, /* .NET name */
    { "uz_UZ_LATN",     "uz_Latn_UZ", NULL, NULL }, /* .NET name */
    { "zh_CHS",         "zh_Hans", NULL, NULL }, /* .NET name */
    { "zh_CHT",         "zh_Hant", NULL, NULL }, /* .NET name */
    { "zh_GAN",         "zh__GAN", NULL, NULL }, /* registered name */
    { "zh_GUOYU",       "zh", NULL, NULL }, /* registered name */
    { "zh_HAKKA",       "zh__HAKKA", NULL, NULL }, /* registered name */
    { "zh_MIN",         "zh__MIN", NULL, NULL }, /* registered name */
    { "zh_MIN_NAN",     "zh__MINNAN", NULL, NULL }, /* registered name */
    { "zh_WUU",         "zh__WUU", NULL, NULL }, /* registered name */
    { "zh_XIANG",       "zh__XIANG", NULL, NULL }, /* registered name */
    { "zh_YUE",         "zh__YUE", NULL, NULL }, /* registered name */
};

typedef struct VariantMap {
    const char *variant;          /* input ID */
    const char *keyword;     /* keyword, or NULL if none */
    const char *value;       /* keyword value, or NULL if kw==NULL */
} VariantMap;

static const VariantMap VARIANT_MAP[] = {
    { "EURO",   "currency", "EUR" },
    { "PINYIN", "collation", "pinyin" }, /* Solaris variant */
    { "STROKE", "collation", "stroke" }  /* Solaris variant */
};

/* ### Keywords **************************************************/

#define ULOC_KEYWORD_BUFFER_LEN 25
#define ULOC_MAX_NO_KEYWORDS 25

static const char * 
locale_getKeywordsStart(const char *localeID) {
    const char *result = NULL;
    if((result = uprv_strchr(localeID, '@')) != NULL) {
        return result;
    }
#if (U_CHARSET_FAMILY == U_EBCDIC_FAMILY)
    else {
        /* We do this because the @ sign is variant, and the @ sign used on one
        EBCDIC machine won't be compiled the same way on other EBCDIC based
        machines. */
        static const uint8_t ebcdicSigns[] = { 0x7C, 0x44, 0x66, 0x80, 0xAC, 0xAE, 0xAF, 0xB5, 0xEC, 0xEF, 0x00 };
        const uint8_t *charToFind = ebcdicSigns;
        while(*charToFind) {
            if((result = uprv_strchr(localeID, *charToFind)) != NULL) {
                return result;
            }
            charToFind++;
        }
    }
#endif
    return NULL;
}

/**
 * @param buf buffer of size [ULOC_KEYWORD_BUFFER_LEN]
 * @param keywordName incoming name to be canonicalized
 * @param status return status (keyword too long)
 * @return length of the keyword name
 */
static int32_t locale_canonKeywordName(char *buf, const char *keywordName, UErrorCode *status)
{
  int32_t i;
  int32_t keywordNameLen = (int32_t)uprv_strlen(keywordName);
  
  if(keywordNameLen >= ULOC_KEYWORD_BUFFER_LEN) {
    /* keyword name too long for internal buffer */
    *status = U_INTERNAL_PROGRAM_ERROR;
          return 0;
  }
  
  /* normalize the keyword name */
  for(i = 0; i < keywordNameLen; i++) {
    buf[i] = uprv_tolower(keywordName[i]);
  }
  buf[i] = 0;
    
  return keywordNameLen;
}

typedef struct {
    char keyword[ULOC_KEYWORD_BUFFER_LEN];
    int32_t keywordLen;
    const char *valueStart;
    int32_t valueLen;
} KeywordStruct;

static int32_t U_CALLCONV
compareKeywordStructs(const void *context, const void *left, const void *right) {
    const char* leftString = ((const KeywordStruct *)left)->keyword;
    const char* rightString = ((const KeywordStruct *)right)->keyword;
    return uprv_strcmp(leftString, rightString);
}

/**
 * Both addKeyword and addValue must already be in canonical form.
 * Either both addKeyword and addValue are NULL, or neither is NULL.
 * If they are not NULL they must be zero terminated.
 * If addKeyword is not NULL is must have length small enough to fit in KeywordStruct.keyword.
 */
static int32_t
_getKeywords(const char *localeID,
             char prev,
             char *keywords, int32_t keywordCapacity,
             char *values, int32_t valuesCapacity, int32_t *valLen,
             UBool valuesToo,
             const char* addKeyword,
             const char* addValue,
             UErrorCode *status)
{
    KeywordStruct keywordList[ULOC_MAX_NO_KEYWORDS];
    
    int32_t maxKeywords = ULOC_MAX_NO_KEYWORDS;
    int32_t numKeywords = 0;
    const char* pos = localeID;
    const char* equalSign = NULL;
    const char* semicolon = NULL;
    int32_t i = 0, j, n;
    int32_t keywordsLen = 0;
    int32_t valuesLen = 0;

    if(prev == '@') { /* start of keyword definition */
        /* we will grab pairs, trim spaces, lowercase keywords, sort and return */
        do {
            UBool duplicate = FALSE;
            /* skip leading spaces */
            while(*pos == ' ') {
                pos++;
            }
            if (!*pos) { /* handle trailing "; " */
                break;
            }
            if(numKeywords == maxKeywords) {
                *status = U_INTERNAL_PROGRAM_ERROR;
                return 0;
            }
            equalSign = uprv_strchr(pos, '=');
            semicolon = uprv_strchr(pos, ';');
            /* lack of '=' [foo@currency] is illegal */
            /* ';' before '=' [foo@currency;collation=pinyin] is illegal */
            if(!equalSign || (semicolon && semicolon<equalSign)) {
                *status = U_INVALID_FORMAT_ERROR;
                return 0;
            }
            /* need to normalize both keyword and keyword name */
            if(equalSign - pos >= ULOC_KEYWORD_BUFFER_LEN) {
                /* keyword name too long for internal buffer */
                *status = U_INTERNAL_PROGRAM_ERROR;
                return 0;
            }
            for(i = 0, n = 0; i < equalSign - pos; ++i) {
                if (pos[i] != ' ') {
                    keywordList[numKeywords].keyword[n++] = uprv_tolower(pos[i]);
                }
            }
            keywordList[numKeywords].keyword[n] = 0;
            keywordList[numKeywords].keywordLen = n;
            /* now grab the value part. First we skip the '=' */
            equalSign++;
            /* then we leading spaces */
            while(*equalSign == ' ') {
                equalSign++;
            }
            keywordList[numKeywords].valueStart = equalSign;
            
            pos = semicolon;
            i = 0;
            if(pos) {
                while(*(pos - i - 1) == ' ') {
                    i++;
                }
                keywordList[numKeywords].valueLen = (int32_t)(pos - equalSign - i);
                pos++;
            } else {
                i = (int32_t)uprv_strlen(equalSign);
                while(equalSign[i-1] == ' ') {
                    i--;
                }
                keywordList[numKeywords].valueLen = i;
            }
            /* If this is a duplicate keyword, then ignore it */
            for (j=0; j<numKeywords; ++j) {
                if (uprv_strcmp(keywordList[j].keyword, keywordList[numKeywords].keyword) == 0) {
                    duplicate = TRUE;
                    break;
                }
            }
            if (!duplicate) {
                ++numKeywords;
            }
        } while(pos);

        /* Handle addKeyword/addValue. */
        if (addKeyword != NULL) {
            UBool duplicate = FALSE;
            U_ASSERT(addValue != NULL);
            /* Search for duplicate; if found, do nothing. Explicit keyword
               overrides addKeyword. */
            for (j=0; j<numKeywords; ++j) {
                if (uprv_strcmp(keywordList[j].keyword, addKeyword) == 0) {
                    duplicate = TRUE;
                    break;
                }
            }
            if (!duplicate) {
                if (numKeywords == maxKeywords) {
                    *status = U_INTERNAL_PROGRAM_ERROR;
                    return 0;
                }
                uprv_strcpy(keywordList[numKeywords].keyword, addKeyword);
                keywordList[numKeywords].keywordLen = (int32_t)uprv_strlen(addKeyword);
                keywordList[numKeywords].valueStart = addValue;
                keywordList[numKeywords].valueLen = (int32_t)uprv_strlen(addValue);
                ++numKeywords;
            }
        } else {
            U_ASSERT(addValue == NULL);
        }

        /* now we have a list of keywords */
        /* we need to sort it */
        uprv_sortArray(keywordList, numKeywords, sizeof(KeywordStruct), compareKeywordStructs, NULL, FALSE, status);
        
        /* Now construct the keyword part */
        for(i = 0; i < numKeywords; i++) {
            if(keywordsLen + keywordList[i].keywordLen + 1< keywordCapacity) {
                uprv_strcpy(keywords+keywordsLen, keywordList[i].keyword);
                if(valuesToo) {
                    keywords[keywordsLen + keywordList[i].keywordLen] = '=';
                } else {
                    keywords[keywordsLen + keywordList[i].keywordLen] = 0;
                }
            }
            keywordsLen += keywordList[i].keywordLen + 1;
            if(valuesToo) {
                if(keywordsLen + keywordList[i].valueLen < keywordCapacity) {
                    uprv_strncpy(keywords+keywordsLen, keywordList[i].valueStart, keywordList[i].valueLen);
                }
                keywordsLen += keywordList[i].valueLen;
                
                if(i < numKeywords - 1) {
                    if(keywordsLen < keywordCapacity) {       
                        keywords[keywordsLen] = ';';
                    }
                    keywordsLen++;
                }
            }
            if(values) {
                if(valuesLen + keywordList[i].valueLen + 1< valuesCapacity) {
                    uprv_strcpy(values+valuesLen, keywordList[i].valueStart);
                    values[valuesLen + keywordList[i].valueLen] = 0;
                }
                valuesLen += keywordList[i].valueLen + 1;
            }
        }
        if(values) {
            values[valuesLen] = 0;
            if(valLen) {
                *valLen = valuesLen;
            }
        }
        return u_terminateChars(keywords, keywordCapacity, keywordsLen, status);   
    } else {
        return 0;
    }
}

U_CFUNC int32_t
locale_getKeywords(const char *localeID,
                   char prev,
                   char *keywords, int32_t keywordCapacity,
                   char *values, int32_t valuesCapacity, int32_t *valLen,
                   UBool valuesToo,
                   UErrorCode *status) {
    return _getKeywords(localeID, prev, keywords, keywordCapacity,
                        values, valuesCapacity, valLen, valuesToo,
                        NULL, NULL, status);
}

U_CAPI int32_t U_EXPORT2
uloc_getKeywordValue(const char* localeID,
                     const char* keywordName,
                     char* buffer, int32_t bufferCapacity,
                     UErrorCode* status)
{ 
    const char* nextSeparator = NULL;
    char keywordNameBuffer[ULOC_KEYWORD_BUFFER_LEN];
    char localeKeywordNameBuffer[ULOC_KEYWORD_BUFFER_LEN];
    int32_t i = 0;
    int32_t result = 0;

    if(status && U_SUCCESS(*status) && localeID) {
    
      const char* startSearchHere = uprv_strchr(localeID, '@'); /* TODO: REVISIT: shouldn't this be locale_getKeywordsStart ? */
      if(startSearchHere == NULL) {
          /* no keywords, return at once */
          return 0;
      }

      locale_canonKeywordName(keywordNameBuffer, keywordName, status);
      if(U_FAILURE(*status)) {
        return 0;
      }
    
      /* find the first keyword */
      while(startSearchHere) {
          startSearchHere++;
          /* skip leading spaces (allowed?) */
          while(*startSearchHere == ' ') {
              startSearchHere++;
          }
          nextSeparator = uprv_strchr(startSearchHere, '=');
          /* need to normalize both keyword and keyword name */
          if(!nextSeparator) {
              break;
          }
          if(nextSeparator - startSearchHere >= ULOC_KEYWORD_BUFFER_LEN) {
              /* keyword name too long for internal buffer */
              *status = U_INTERNAL_PROGRAM_ERROR;
              return 0;
          }
          for(i = 0; i < nextSeparator - startSearchHere; i++) {
              localeKeywordNameBuffer[i] = uprv_tolower(startSearchHere[i]);
          }
          /* trim trailing spaces */
          while(startSearchHere[i-1] == ' ') {
              i--;
          }
          localeKeywordNameBuffer[i] = 0;
        
          startSearchHere = uprv_strchr(nextSeparator, ';');
        
          if(uprv_strcmp(keywordNameBuffer, localeKeywordNameBuffer) == 0) {
              nextSeparator++;
              while(*nextSeparator == ' ') {
                  nextSeparator++;
              }
              /* we actually found the keyword. Copy the value */
              if(startSearchHere && startSearchHere - nextSeparator < bufferCapacity) {
                  while(*(startSearchHere-1) == ' ') {
                      startSearchHere--;
                  }
                  uprv_strncpy(buffer, nextSeparator, startSearchHere - nextSeparator);
                  result = u_terminateChars(buffer, bufferCapacity, (int32_t)(startSearchHere - nextSeparator), status);
              } else if(!startSearchHere && (int32_t)uprv_strlen(nextSeparator) < bufferCapacity) { /* last item in string */
                  i = (int32_t)uprv_strlen(nextSeparator);
                  while(nextSeparator[i - 1] == ' ') {
                      i--;
                  }
                  uprv_strncpy(buffer, nextSeparator, i);
                  result = u_terminateChars(buffer, bufferCapacity, i, status);
              } else {
                  /* give a bigger buffer, please */
                  *status = U_BUFFER_OVERFLOW_ERROR;
                  if(startSearchHere) {
                      result = (int32_t)(startSearchHere - nextSeparator);
                  } else {
                      result = (int32_t)uprv_strlen(nextSeparator); 
                  }
              }
              return result;
          }
      }
    }
    return 0;
}

U_CAPI int32_t U_EXPORT2
uloc_setKeywordValue(const char* keywordName,
                     const char* keywordValue,
                     char* buffer, int32_t bufferCapacity,
                     UErrorCode* status)
{
    /* TODO: sorting. removal. */
    int32_t keywordNameLen;
    int32_t keywordValueLen;
    int32_t bufLen;
    int32_t needLen = 0;
    int32_t foundValueLen;
    int32_t keywordAtEnd = 0; /* is the keyword at the end of the string? */
    char keywordNameBuffer[ULOC_KEYWORD_BUFFER_LEN];
    char localeKeywordNameBuffer[ULOC_KEYWORD_BUFFER_LEN];
    int32_t i = 0;
    int32_t rc;
    char* nextSeparator = NULL;
    char* nextEqualsign = NULL;
    char* startSearchHere = NULL;
    char* keywordStart = NULL;
    char *insertHere = NULL;
    if(U_FAILURE(*status)) { 
        return -1; 
    }
    if(bufferCapacity>1) {
        bufLen = (int32_t)uprv_strlen(buffer);
    } else {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(bufferCapacity<bufLen) {
        /* The capacity is less than the length?! Is this NULL terminated? */
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(keywordValue && !*keywordValue) { 
        keywordValue = NULL;
    }
    if(keywordValue) {
        keywordValueLen = (int32_t)uprv_strlen(keywordValue);
    } else { 
        keywordValueLen = 0;
    }
    keywordNameLen = locale_canonKeywordName(keywordNameBuffer, keywordName, status);
    if(U_FAILURE(*status)) {
        return 0;
    }
    startSearchHere = (char*)locale_getKeywordsStart(buffer);
    if(startSearchHere == NULL || (startSearchHere[1]==0)) {
        if(!keywordValue) { /* no keywords = nothing to remove */
            return bufLen; 
        }

        needLen = bufLen+1+keywordNameLen+1+keywordValueLen;
        if(startSearchHere) { /* had a single @ */ 
            needLen--; /* already had the @ */
            /* startSearchHere points at the @ */
        } else {
            startSearchHere=buffer+bufLen;
        }
        if(needLen >= bufferCapacity) {
            *status = U_BUFFER_OVERFLOW_ERROR;
            return needLen; /* no change */
        }
        *startSearchHere = '@';
        startSearchHere++;
        uprv_strcpy(startSearchHere, keywordNameBuffer);
        startSearchHere += keywordNameLen;
        *startSearchHere = '=';
        startSearchHere++;
        uprv_strcpy(startSearchHere, keywordValue);
        startSearchHere+=keywordValueLen;
        return needLen;
    } /* end shortcut - no @ */
    
    keywordStart = startSearchHere;
    /* search for keyword */
    while(keywordStart) {
        keywordStart++;
        /* skip leading spaces (allowed?) */
        while(*keywordStart == ' ') {
            keywordStart++;
        }
        nextEqualsign = uprv_strchr(keywordStart, '=');
        /* need to normalize both keyword and keyword name */
        if(!nextEqualsign) {
            break;
        }
        if(nextEqualsign - keywordStart >= ULOC_KEYWORD_BUFFER_LEN) {
            /* keyword name too long for internal buffer */
            *status = U_INTERNAL_PROGRAM_ERROR;
            return 0;
        }
        for(i = 0; i < nextEqualsign - keywordStart; i++) {
            localeKeywordNameBuffer[i] = uprv_tolower(keywordStart[i]);
        }
        /* trim trailing spaces */
        while(keywordStart[i-1] == ' ') {
            i--;
        }
        localeKeywordNameBuffer[i] = 0;

        nextSeparator = uprv_strchr(nextEqualsign, ';');
        rc = uprv_strcmp(keywordNameBuffer, localeKeywordNameBuffer);
        if(rc == 0) {
            nextEqualsign++;
            while(*nextEqualsign == ' ') {
                nextEqualsign++;
            }
            /* we actually found the keyword. Change the value */
            if (nextSeparator) {
                keywordAtEnd = 0;
                foundValueLen = (int32_t)(nextSeparator - nextEqualsign);
            } else {
                keywordAtEnd = 1;
                foundValueLen = (int32_t)uprv_strlen(nextEqualsign);
            }
            if(keywordValue) { /* adding a value - not removing */
              if(foundValueLen == keywordValueLen) {
                uprv_strncpy(nextEqualsign, keywordValue, keywordValueLen);
                return bufLen; /* no change in size */
              } else if(foundValueLen > keywordValueLen) {
                int32_t delta = foundValueLen - keywordValueLen;
                if(nextSeparator) { /* RH side */
                  uprv_memmove(nextSeparator - delta, nextSeparator, bufLen-(nextSeparator-buffer));
                }
                uprv_strncpy(nextEqualsign, keywordValue, keywordValueLen);
                bufLen -= delta;
                buffer[bufLen]=0;
                return bufLen;
              } else { /* FVL < KVL */
                int32_t delta = keywordValueLen - foundValueLen;
                if((bufLen+delta) >= bufferCapacity) {
                  *status = U_BUFFER_OVERFLOW_ERROR;
                  return bufLen+delta;
                }
                if(nextSeparator) { /* RH side */
                  uprv_memmove(nextSeparator+delta,nextSeparator, bufLen-(nextSeparator-buffer));
                }
                uprv_strncpy(nextEqualsign, keywordValue, keywordValueLen);
                bufLen += delta;
                buffer[bufLen]=0;
                return bufLen;
              }
            } else { /* removing a keyword */
              if(keywordAtEnd) {
                /* zero out the ';' or '@' just before startSearchhere */
                keywordStart[-1] = 0;
                return (int32_t)((keywordStart-buffer)-1); /* (string length without keyword) minus separator */
              } else {
                uprv_memmove(keywordStart, nextSeparator+1, bufLen-((nextSeparator+1)-buffer));
                keywordStart[bufLen-((nextSeparator+1)-buffer)]=0;
                return (int32_t)(bufLen-((nextSeparator+1)-keywordStart));
              }
            }
        } else if(rc<0){ /* end match keyword */
          /* could insert at this location. */
          insertHere = keywordStart;
        }
        keywordStart = nextSeparator;
    } /* end loop searching */
    
    if(!keywordValue) {
      return bufLen; /* removal of non-extant keyword - no change */
    }

    /* we know there is at least one keyword. */
    needLen = bufLen+1+keywordNameLen+1+keywordValueLen;
    if(needLen >= bufferCapacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
        return needLen; /* no change */
    }
    
    if(insertHere) {
      uprv_memmove(insertHere+(1+keywordNameLen+1+keywordValueLen), insertHere, bufLen-(insertHere-buffer));
      keywordStart = insertHere;
    } else {
      keywordStart = buffer+bufLen;
      *keywordStart = ';';
      keywordStart++;
    }
    uprv_strncpy(keywordStart, keywordNameBuffer, keywordNameLen);
    keywordStart += keywordNameLen;
    *keywordStart = '=';
    keywordStart++;
    uprv_strncpy(keywordStart, keywordValue, keywordValueLen); /* terminates. */
    keywordStart+=keywordValueLen;
    if(insertHere) {
      *keywordStart = ';';
      keywordStart++;
    }
    buffer[needLen]=0;
    return needLen;
}

/* ### ID parsing implementation **************************************************/

/*returns TRUE if a is an ID separator FALSE otherwise*/
#define _isIDSeparator(a) (a == '_' || a == '-')

#define _isPrefixLetter(a) ((a=='x')||(a=='X')||(a=='i')||(a=='I'))

/*returns TRUE if one of the special prefixes is here (s=string)
  'x-' or 'i-' */
#define _isIDPrefix(s) (_isPrefixLetter(s[0])&&_isIDSeparator(s[1]))

/* Dot terminates it because of POSIX form  where dot precedes the codepage
 * except for variant
 */
#define _isTerminator(a)  ((a==0)||(a=='.')||(a=='@'))

static char* _strnchr(const char* str, int32_t len, char c) {
    U_ASSERT(str != 0 && len >= 0);
    while (len-- != 0) {
        char d = *str;
        if (d == c) {
            return (char*) str;
        } else if (d == 0) {
            break;
        }
        ++str;
    }
    return NULL;
}

/**
 * Lookup 'key' in the array 'list'.  The array 'list' should contain
 * a NULL entry, followed by more entries, and a second NULL entry.
 *
 * The 'list' param should be LANGUAGES, LANGUAGES_3, COUNTRIES, or
 * COUNTRIES_3.
 */
static int16_t _findIndex(const char* const* list, const char* key)
{
    const char* const* anchor = list;
    int32_t pass = 0;

    /* Make two passes through two NULL-terminated arrays at 'list' */
    while (pass++ < 2) {
        while (*list) {
            if (uprv_strcmp(key, *list) == 0) {
                return (int16_t)(list - anchor);
            }
            list++;
        }
        ++list;     /* skip final NULL *CWB*/
    }
    return -1;
}

/* count the length of src while copying it to dest; return strlen(src) */
static U_INLINE int32_t
_copyCount(char *dest, int32_t destCapacity, const char *src) {
    const char *anchor;
    char c;

    anchor=src;
    for(;;) {
        if((c=*src)==0) {
            return (int32_t)(src-anchor);
        }
        if(destCapacity<=0) {
            return (int32_t)((src-anchor)+uprv_strlen(src));
        }
        ++src;
        *dest++=c;
        --destCapacity;
    }
}

static const char* 
uloc_getCurrentCountryID(const char* oldID){
    int32_t offset = _findIndex(DEPRECATED_COUNTRIES, oldID);
    if (offset >= 0) {
        return REPLACEMENT_COUNTRIES[offset];
    }
    return oldID;
}
static const char* 
uloc_getCurrentLanguageID(const char* oldID){
    int32_t offset = _findIndex(DEPRECATED_LANGUAGES, oldID);
    if (offset >= 0) {
        return REPLACEMENT_LANGUAGES[offset];
    }
    return oldID;        
}
/*
 * the internal functions _getLanguage(), _getCountry(), _getVariant()
 * avoid duplicating code to handle the earlier locale ID pieces
 * in the functions for the later ones by
 * setting the *pEnd pointer to where they stopped parsing
 *
 * TODO try to use this in Locale
 */
static int32_t
_getLanguage(const char *localeID,
             char *language, int32_t languageCapacity,
             const char **pEnd) {
    int32_t i=0;
    int32_t offset;
    char lang[4]={ 0, 0, 0, 0 }; /* temporary buffer to hold language code for searching */

    /* if it starts with i- or x- then copy that prefix */
    if(_isIDPrefix(localeID)) {
        if(i<languageCapacity) {
            language[i]=(char)uprv_tolower(*localeID);
        }
        if(i<languageCapacity) {
            language[i+1]='-';
        }
        i+=2;
        localeID+=2;
    }
    
    /* copy the language as far as possible and count its length */
    while(!_isTerminator(*localeID) && !_isIDSeparator(*localeID)) {
        if(i<languageCapacity) {
            language[i]=(char)uprv_tolower(*localeID);
        }
        if(i<3) {
            lang[i]=(char)uprv_tolower(*localeID);
        }
        i++;
        localeID++;
    }

    if(i==3) {
        /* convert 3 character code to 2 character code if possible *CWB*/
        offset=_findIndex(LANGUAGES_3, lang);
        if(offset>=0) {
            i=_copyCount(language, languageCapacity, LANGUAGES[offset]);
        }
    }

    if(pEnd!=NULL) {
        *pEnd=localeID;
    }
    return i;
}

static int32_t
_getScript(const char *localeID,
            char *script, int32_t scriptCapacity,
            const char **pEnd)
{
    int32_t idLen = 0;

    if (pEnd != NULL) {
        *pEnd = localeID;
    }

    /* copy the second item as far as possible and count its length */
    while(!_isTerminator(localeID[idLen]) && !_isIDSeparator(localeID[idLen])) {
        idLen++;
    }

    /* If it's exactly 4 characters long, then it's a script and not a country. */
    if (idLen == 4) {
        int32_t i;
        if (pEnd != NULL) {
            *pEnd = localeID+idLen;
        }
        if(idLen > scriptCapacity) {
            idLen = scriptCapacity;
        }
        if (idLen >= 1) {
            script[0]=(char)uprv_toupper(*(localeID++));
        }
        for (i = 1; i < idLen; i++) {
            script[i]=(char)uprv_tolower(*(localeID++));
        }
    }
    else {
        idLen = 0;
    }
    return idLen;
}

static int32_t
_getCountry(const char *localeID,
            char *country, int32_t countryCapacity,
            const char **pEnd)
{
    int32_t i=0;
    char cnty[ULOC_COUNTRY_CAPACITY]={ 0, 0, 0, 0 };
    int32_t offset;

    /* copy the country as far as possible and count its length */
    while(!_isTerminator(*localeID) && !_isIDSeparator(*localeID)) {
        if(i<countryCapacity) {
            country[i]=(char)uprv_toupper(*localeID);
        }
        if(i<(ULOC_COUNTRY_CAPACITY-1)) {   /*CWB*/
            cnty[i]=(char)uprv_toupper(*localeID);
        }
        i++;
        localeID++;
    }

    /* convert 3 character code to 2 character code if possible *CWB*/
    if(i==3) {
        offset=_findIndex(COUNTRIES_3, cnty);
        if(offset>=0) {
            i=_copyCount(country, countryCapacity, COUNTRIES[offset]);
        }
    }

    if(pEnd!=NULL) {
        *pEnd=localeID;
    }
    return i;
}

/**
 * @param needSeparator if true, then add leading '_' if any variants
 * are added to 'variant'
 */
static int32_t
_getVariantEx(const char *localeID,
              char prev,
              char *variant, int32_t variantCapacity,
              UBool needSeparator) {
    int32_t i=0;

    /* get one or more variant tags and separate them with '_' */
    if(_isIDSeparator(prev)) {
        /* get a variant string after a '-' or '_' */
        while(!_isTerminator(*localeID)) {
            if (needSeparator) {
                if (i<variantCapacity) {
                    variant[i] = '_';
                }
                ++i;
                needSeparator = FALSE;
            }
            if(i<variantCapacity) {
                variant[i]=(char)uprv_toupper(*localeID);
                if(variant[i]=='-') {
                    variant[i]='_';
                }
            }
            i++;
            localeID++;
        }
    }

    /* if there is no variant tag after a '-' or '_' then look for '@' */
    if(i==0) {
        if(prev=='@') {
            /* keep localeID */
        } else if((localeID=locale_getKeywordsStart(localeID))!=NULL) {
            ++localeID; /* point after the '@' */
        } else {
            return 0;
        }
        while(!_isTerminator(*localeID)) {
            if (needSeparator) {
                if (i<variantCapacity) {
                    variant[i] = '_';
                }
                ++i;
                needSeparator = FALSE;
            }
            if(i<variantCapacity) {
                variant[i]=(char)uprv_toupper(*localeID);
                if(variant[i]=='-' || variant[i]==',') {
                    variant[i]='_';
                }
            }
            i++;
            localeID++;
        }
    }
    
    return i;
}

static int32_t
_getVariant(const char *localeID,
            char prev,
            char *variant, int32_t variantCapacity) {
    return _getVariantEx(localeID, prev, variant, variantCapacity, FALSE);
}

/**
 * Delete ALL instances of a variant from the given list of one or
 * more variants.  Example: "FOO_EURO_BAR_EURO" => "FOO_BAR".
 * @param variants the source string of one or more variants,
 * separated by '_'.  This will be MODIFIED IN PLACE.  Not zero
 * terminated; if it is, trailing zero will NOT be maintained.
 * @param variantsLen length of variants
 * @param toDelete variant to delete, without separators, e.g.  "EURO"
 * or "PREEURO"; not zero terminated
 * @param toDeleteLen length of toDelete
 * @return number of characters deleted from variants
 */
static int32_t
_deleteVariant(char* variants, int32_t variantsLen,
               const char* toDelete, int32_t toDeleteLen)
{
    int32_t delta = 0; /* number of chars deleted */
    for (;;) {
        UBool flag = FALSE;
        if (variantsLen < toDeleteLen) {
            return delta;
        }
        if (uprv_strncmp(variants, toDelete, toDeleteLen) == 0 &&
            (variantsLen == toDeleteLen ||
             (flag=(variants[toDeleteLen] == '_'))))
        {
            int32_t d = toDeleteLen + (flag?1:0);
            variantsLen -= d;
            delta += d;
            if (variantsLen > 0) {
                uprv_memmove(variants, variants+d, variantsLen);
            }
        } else {
            char* p = _strnchr(variants, variantsLen, '_');
            if (p == NULL) {
                return delta;
            }
            ++p;
            variantsLen -= (int32_t)(p - variants);
            variants = p;
        }
    }
}

/* Keyword enumeration */

typedef struct UKeywordsContext {
    char* keywords;
    char* current;
} UKeywordsContext;

static void U_CALLCONV
uloc_kw_closeKeywords(UEnumeration *enumerator) {
    uprv_free(((UKeywordsContext *)enumerator->context)->keywords);
    uprv_free(enumerator->context);
    uprv_free(enumerator);
}

static int32_t U_CALLCONV
uloc_kw_countKeywords(UEnumeration *en, UErrorCode *status) {
    char *kw = ((UKeywordsContext *)en->context)->keywords;
    int32_t result = 0;
    while(*kw) {
        result++;
        kw += uprv_strlen(kw)+1;
    }
    return result;
}

static const char* U_CALLCONV 
uloc_kw_nextKeyword(UEnumeration* en,
                    int32_t* resultLength,
                    UErrorCode* status) {
    const char* result = ((UKeywordsContext *)en->context)->current;
    int32_t len = 0;
    if(*result) {
        len = (int32_t)uprv_strlen(((UKeywordsContext *)en->context)->current);
        ((UKeywordsContext *)en->context)->current += len+1;
    } else {
        result = NULL;
    }
    if (resultLength) {
        *resultLength = len;
    }
    return result;
}

static void U_CALLCONV 
uloc_kw_resetKeywords(UEnumeration* en, 
                      UErrorCode* status) {
    ((UKeywordsContext *)en->context)->current = ((UKeywordsContext *)en->context)->keywords;
}

static const UEnumeration gKeywordsEnum = {
    NULL,
    NULL,
    uloc_kw_closeKeywords,
    uloc_kw_countKeywords,
    uenum_unextDefault,
    uloc_kw_nextKeyword,
    uloc_kw_resetKeywords
};

U_CAPI UEnumeration* U_EXPORT2
uloc_openKeywordList(const char *keywordList, int32_t keywordListSize, UErrorCode* status)
{
    UKeywordsContext *myContext = NULL;
    UEnumeration *result = NULL;

    if(U_FAILURE(*status)) {
        return NULL;
    }
    result = (UEnumeration *)uprv_malloc(sizeof(UEnumeration));
    /* Null pointer test */
    if (result == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memcpy(result, &gKeywordsEnum, sizeof(UEnumeration));
    myContext = uprv_malloc(sizeof(UKeywordsContext));
    if (myContext == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(result);
        return NULL;
    }
    myContext->keywords = (char *)uprv_malloc(keywordListSize+1);
    uprv_memcpy(myContext->keywords, keywordList, keywordListSize);
    myContext->keywords[keywordListSize] = 0;
    myContext->current = myContext->keywords;
    result->context = myContext;
    return result;
}

U_CAPI UEnumeration* U_EXPORT2
uloc_openKeywords(const char* localeID,
                        UErrorCode* status) 
{
    int32_t i=0;
    char keywords[256];
    int32_t keywordsCapacity = 256;
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }
    
    if(localeID==NULL) {
        localeID=uloc_getDefault();
    }

    /* Skip the language */
    _getLanguage(localeID, NULL, 0, &localeID);
    if(_isIDSeparator(*localeID)) {
        const char *scriptID;
        /* Skip the script if available */
        _getScript(localeID+1, NULL, 0, &scriptID);
        if(scriptID != localeID+1) {
            /* Found optional script */
            localeID = scriptID;
        }
        /* Skip the Country */
        if (_isIDSeparator(*localeID)) {
            _getCountry(localeID+1, NULL, 0, &localeID);
            if(_isIDSeparator(*localeID)) {
                _getVariant(localeID+1, *localeID, NULL, 0);
            }
        }
    }

    /* keywords are located after '@' */
    if((localeID = locale_getKeywordsStart(localeID)) != NULL) {
        i=locale_getKeywords(localeID+1, '@', keywords, keywordsCapacity, NULL, 0, NULL, FALSE, status);
    }

    if(i) {
        return uloc_openKeywordList(keywords, i, status);
    } else {
        return NULL;
    }
}


/* bit-flags for 'options' parameter of _canonicalize */
#define _ULOC_STRIP_KEYWORDS 0x2
#define _ULOC_CANONICALIZE   0x1

#define OPTION_SET(options, mask) ((options & mask) != 0)

static const char i_default[] = {'i', '-', 'd', 'e', 'f', 'a', 'u', 'l', 't'};
#define I_DEFAULT_LENGTH (sizeof i_default / sizeof i_default[0])

/**
 * Canonicalize the given localeID, to level 1 or to level 2,
 * depending on the options.  To specify level 1, pass in options=0.
 * To specify level 2, pass in options=_ULOC_CANONICALIZE.
 *
 * This is the code underlying uloc_getName and uloc_canonicalize.
 */
static int32_t
_canonicalize(const char* localeID,
              char* result,
              int32_t resultCapacity,
              uint32_t options,
              UErrorCode* err) {
    int32_t j, len, fieldCount=0, scriptSize=0, variantSize=0, nameCapacity;
    char localeBuffer[ULOC_FULLNAME_CAPACITY];
    const char* origLocaleID;
    const char* keywordAssign = NULL;
    const char* separatorIndicator = NULL;
    const char* addKeyword = NULL;
    const char* addValue = NULL;
    char* name;
    char* variant = NULL; /* pointer into name, or NULL */

    if (U_FAILURE(*err)) {
        return 0;
    }
    
    if (localeID==NULL) {
        localeID=uloc_getDefault();
    }
    origLocaleID=localeID;

    /* if we are doing a full canonicalization, then put results in
       localeBuffer, if necessary; otherwise send them to result. */
    if (OPTION_SET(options, _ULOC_CANONICALIZE) &&
        (result == NULL || resultCapacity <  sizeof(localeBuffer))) {
        name = localeBuffer;
        nameCapacity = sizeof(localeBuffer);
    } else {
        name = result;
        nameCapacity = resultCapacity;
    }

    /* get all pieces, one after another, and separate with '_' */
    len=_getLanguage(localeID, name, nameCapacity, &localeID);

    if(len == I_DEFAULT_LENGTH && uprv_strncmp(origLocaleID, i_default, len) == 0) {
        const char *d = uloc_getDefault();
        
        len = uprv_strlen(d);

        if (name != NULL) {
            uprv_strncpy(name, d, len);
        }
    } else if(_isIDSeparator(*localeID)) {
        const char *scriptID;

        ++fieldCount;
        if(len<nameCapacity) {
            name[len]='_';
        }
        ++len;

        scriptSize=_getScript(localeID+1, name+len, nameCapacity-len, &scriptID);
        if(scriptSize > 0) {
            /* Found optional script */
            localeID = scriptID;
            ++fieldCount;
            len+=scriptSize;
            if (_isIDSeparator(*localeID)) {
                /* If there is something else, then we add the _ */
                if(len<nameCapacity) {
                    name[len]='_';
                }
                ++len;
            }
        }

        if (_isIDSeparator(*localeID)) {
            len+=_getCountry(localeID+1, name+len, nameCapacity-len, &localeID);
            if(_isIDSeparator(*localeID)) {
                ++fieldCount;
                if(len<nameCapacity) {
                    name[len]='_';
                }
                ++len;
                variantSize = _getVariant(localeID+1, *localeID, name+len, nameCapacity-len);
                if (variantSize > 0) {
                    variant = name+len;
                    len += variantSize;
                    localeID += variantSize + 1; /* skip '_' and variant */
                }
            }
        }
    }

    /* Copy POSIX-style charset specifier, if any [mr.utf8] */
    if (!OPTION_SET(options, _ULOC_CANONICALIZE) && *localeID == '.') {
        UBool done = FALSE;
        do {
            char c = *localeID;
            switch (c) {
            case 0:
            case '@':
                done = TRUE;
                break;
            default:
                if (len<nameCapacity) {
                    name[len] = c;
                }
                ++len;
                ++localeID;
                break;
            }
        } while (!done);
    }

    /* Scan ahead to next '@' and determine if it is followed by '=' and/or ';'
       After this, localeID either points to '@' or is NULL */
    if ((localeID=locale_getKeywordsStart(localeID))!=NULL) {
        keywordAssign = uprv_strchr(localeID, '=');
        separatorIndicator = uprv_strchr(localeID, ';');
    }

    /* Copy POSIX-style variant, if any [mr@FOO] */
    if (!OPTION_SET(options, _ULOC_CANONICALIZE) &&
        localeID != NULL && keywordAssign == NULL) {
        for (;;) {
            char c = *localeID;
            if (c == 0) {
                break;
            }
            if (len<nameCapacity) {
                name[len] = c;
            }
            ++len;
            ++localeID;
        }
    }

    if (OPTION_SET(options, _ULOC_CANONICALIZE)) {
        /* Handle @FOO variant if @ is present and not followed by = */
        if (localeID!=NULL && keywordAssign==NULL) {
            int32_t posixVariantSize;
            /* Add missing '_' if needed */
            if (fieldCount < 2 || (fieldCount < 3 && scriptSize > 0)) {
                do {
                    if(len<nameCapacity) {
                        name[len]='_';
                    }
                    ++len;
                    ++fieldCount;
                } while(fieldCount<2);
            }
            posixVariantSize = _getVariantEx(localeID+1, '@', name+len, nameCapacity-len,
                                             (UBool)(variantSize > 0));
            if (posixVariantSize > 0) {
                if (variant == NULL) {
                    variant = name+len;
                }
                len += posixVariantSize;
                variantSize += posixVariantSize;
            }
        }

        /* Handle generic variants first */
        if (variant) {
            for (j=0; j<(int32_t)(sizeof(VARIANT_MAP)/sizeof(VARIANT_MAP[0])); j++) {
                const char* variantToCompare = VARIANT_MAP[j].variant;
                int32_t n = (int32_t)uprv_strlen(variantToCompare);
                int32_t variantLen = _deleteVariant(variant, uprv_min(variantSize, (nameCapacity-len)), variantToCompare, n);
                len -= variantLen;
                if (variantLen > 0) {
                    if (name[len-1] == '_') { /* delete trailing '_' */
                        --len;
                    }
                    addKeyword = VARIANT_MAP[j].keyword;
                    addValue = VARIANT_MAP[j].value;
                    break;
                }
            }
            if (name[len-1] == '_') { /* delete trailing '_' */
                --len;
            }
        }

        /* Look up the ID in the canonicalization map */
        for (j=0; j<(int32_t)(sizeof(CANONICALIZE_MAP)/sizeof(CANONICALIZE_MAP[0])); j++) {
            const char* id = CANONICALIZE_MAP[j].id;
            int32_t n = (int32_t)uprv_strlen(id);
            if (len == n && uprv_strncmp(name, id, n) == 0) {
                if (n == 0 && localeID != NULL) {
                    break; /* Don't remap "" if keywords present */
                }
                len = _copyCount(name, nameCapacity, CANONICALIZE_MAP[j].canonicalID);
                if (CANONICALIZE_MAP[j].keyword) {
                    addKeyword = CANONICALIZE_MAP[j].keyword;
                    addValue = CANONICALIZE_MAP[j].value;
                }
                break;
            }
        }
    }

    if (!OPTION_SET(options, _ULOC_STRIP_KEYWORDS)) {
        if (localeID!=NULL && keywordAssign!=NULL &&
            (!separatorIndicator || separatorIndicator > keywordAssign)) {
            if(len<nameCapacity) {
                name[len]='@';
            }
            ++len;
            ++fieldCount;
            len += _getKeywords(localeID+1, '@', name+len, nameCapacity-len, NULL, 0, NULL, TRUE,
                                addKeyword, addValue, err);
        } else if (addKeyword != NULL) {
            U_ASSERT(addValue != NULL);
            /* inelegant but works -- later make _getKeywords do this? */
            len += _copyCount(name+len, nameCapacity-len, "@");
            len += _copyCount(name+len, nameCapacity-len, addKeyword);
            len += _copyCount(name+len, nameCapacity-len, "=");
            len += _copyCount(name+len, nameCapacity-len, addValue);
        }
    }

    if (U_SUCCESS(*err) && result != NULL && name == localeBuffer) {
        uprv_strncpy(result, localeBuffer, (len > resultCapacity) ? resultCapacity : len);
    }

    return u_terminateChars(result, resultCapacity, len, err);
}

/* ### ID parsing API **************************************************/

U_CAPI int32_t  U_EXPORT2
uloc_getParent(const char*    localeID,
               char* parent,
               int32_t parentCapacity,
               UErrorCode* err)
{
    const char *lastUnderscore;
    int32_t i;
    
    if (U_FAILURE(*err))
        return 0;
    
    if (localeID == NULL)
        localeID = uloc_getDefault();

    lastUnderscore=uprv_strrchr(localeID, '_');
    if(lastUnderscore!=NULL) {
        i=(int32_t)(lastUnderscore-localeID);
    } else {
        i=0;
    }

    if(i>0 && parent != localeID) {
        uprv_memcpy(parent, localeID, uprv_min(i, parentCapacity));
    }
    return u_terminateChars(parent, parentCapacity, i, err);
}

U_CAPI int32_t U_EXPORT2
uloc_getLanguage(const char*    localeID,
         char* language,
         int32_t languageCapacity,
         UErrorCode* err)
{
    /* uloc_getLanguage will return a 2 character iso-639 code if one exists. *CWB*/
    int32_t i=0;

    if (err==NULL || U_FAILURE(*err)) {
        return 0;
    }
    
    if(localeID==NULL) {
        localeID=uloc_getDefault();
    }

    i=_getLanguage(localeID, language, languageCapacity, NULL);
    return u_terminateChars(language, languageCapacity, i, err);
}

U_CAPI int32_t U_EXPORT2
uloc_getScript(const char*    localeID,
         char* script,
         int32_t scriptCapacity,
         UErrorCode* err)
{
    int32_t i=0;

    if(err==NULL || U_FAILURE(*err)) {
        return 0;
    }

    if(localeID==NULL) {
        localeID=uloc_getDefault();
    }

    /* skip the language */
    _getLanguage(localeID, NULL, 0, &localeID);
    if(_isIDSeparator(*localeID)) {
        i=_getScript(localeID+1, script, scriptCapacity, NULL);
    }
    return u_terminateChars(script, scriptCapacity, i, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_getCountry(const char* localeID,
            char* country,
            int32_t countryCapacity,
            UErrorCode* err) 
{
    int32_t i=0;

    if(err==NULL || U_FAILURE(*err)) {
        return 0;
    }

    if(localeID==NULL) {
        localeID=uloc_getDefault();
    }

    /* Skip the language */
    _getLanguage(localeID, NULL, 0, &localeID);
    if(_isIDSeparator(*localeID)) {
        const char *scriptID;
        /* Skip the script if available */
        _getScript(localeID+1, NULL, 0, &scriptID);
        if(scriptID != localeID+1) {
            /* Found optional script */
            localeID = scriptID;
        }
        if(_isIDSeparator(*localeID)) {
            i=_getCountry(localeID+1, country, countryCapacity, NULL);
        }
    }
    return u_terminateChars(country, countryCapacity, i, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_getVariant(const char* localeID,
                char* variant,
                int32_t variantCapacity,
                UErrorCode* err) 
{
    int32_t i=0;
    
    if(err==NULL || U_FAILURE(*err)) {
        return 0;
    }
    
    if(localeID==NULL) {
        localeID=uloc_getDefault();
    }
    
    /* Skip the language */
    _getLanguage(localeID, NULL, 0, &localeID);
    if(_isIDSeparator(*localeID)) {
        const char *scriptID;
        /* Skip the script if available */
        _getScript(localeID+1, NULL, 0, &scriptID);
        if(scriptID != localeID+1) {
            /* Found optional script */
            localeID = scriptID;
        }
        /* Skip the Country */
        if (_isIDSeparator(*localeID)) {
            _getCountry(localeID+1, NULL, 0, &localeID);
            if(_isIDSeparator(*localeID)) {
                i=_getVariant(localeID+1, *localeID, variant, variantCapacity);
            }
        }
    }
    
    /* removed by weiv. We don't want to handle POSIX variants anymore. Use canonicalization function */
    /* if we do not have a variant tag yet then try a POSIX variant after '@' */
/*
    if(!haveVariant && (localeID=uprv_strrchr(localeID, '@'))!=NULL) {
        i=_getVariant(localeID+1, '@', variant, variantCapacity);
    }
*/
    return u_terminateChars(variant, variantCapacity, i, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_getName(const char* localeID,
             char* name,
             int32_t nameCapacity,
             UErrorCode* err)  
{
    return _canonicalize(localeID, name, nameCapacity, 0, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_getBaseName(const char* localeID,
                 char* name,
                 int32_t nameCapacity,
                 UErrorCode* err)  
{
    return _canonicalize(localeID, name, nameCapacity, _ULOC_STRIP_KEYWORDS, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_canonicalize(const char* localeID,
                  char* name,
                  int32_t nameCapacity,
                  UErrorCode* err)  
{
    return _canonicalize(localeID, name, nameCapacity, _ULOC_CANONICALIZE, err);
}
  
U_CAPI const char*  U_EXPORT2
uloc_getISO3Language(const char* localeID) 
{
    int16_t offset;
    char lang[ULOC_LANG_CAPACITY];
    UErrorCode err = U_ZERO_ERROR;
    
    if (localeID == NULL)
    {
        localeID = uloc_getDefault();
    }
    uloc_getLanguage(localeID, lang, ULOC_LANG_CAPACITY, &err);
    if (U_FAILURE(err))
        return "";
    offset = _findIndex(LANGUAGES, lang);
    if (offset < 0)
        return "";
    return LANGUAGES_3[offset];
}

U_CAPI const char*  U_EXPORT2
uloc_getISO3Country(const char* localeID) 
{
    int16_t offset;
    char cntry[ULOC_LANG_CAPACITY];
    UErrorCode err = U_ZERO_ERROR;
    
    if (localeID == NULL)
    {
        localeID = uloc_getDefault();
    }
    uloc_getCountry(localeID, cntry, ULOC_LANG_CAPACITY, &err);
    if (U_FAILURE(err))
        return "";
    offset = _findIndex(COUNTRIES, cntry);
    if (offset < 0)
        return "";
    
    return COUNTRIES_3[offset];
}

U_CAPI uint32_t  U_EXPORT2
uloc_getLCID(const char* localeID) 
{
    UErrorCode status = U_ZERO_ERROR;
    char       langID[ULOC_FULLNAME_CAPACITY];

    uloc_getLanguage(localeID, langID, sizeof(langID), &status);
    if (U_FAILURE(status)) {
        return 0;
    }

    return uprv_convertToLCID(langID, localeID, &status);
}

U_CAPI int32_t U_EXPORT2
uloc_getLocaleForLCID(uint32_t hostid, char *locale, int32_t localeCapacity,
                UErrorCode *status)
{
    int32_t length;
    const char *posix = uprv_convertToPosix(hostid, status);
    if (U_FAILURE(*status) || posix == NULL) {
        return 0;
    }
    length = (int32_t)uprv_strlen(posix);
    if (length+1 > localeCapacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    else {
        uprv_strcpy(locale, posix);
    }
    return length;
}

/* ### Default locale **************************************************/

U_CAPI const char*  U_EXPORT2
uloc_getDefault()
{
    return locale_get_default();
}

U_CAPI void  U_EXPORT2
uloc_setDefault(const char*   newDefaultLocale,
             UErrorCode* err) 
{
    if (U_FAILURE(*err))
        return;
    /* the error code isn't currently used for anything by this function*/
    
    /* propagate change to C++ */
    locale_set_default(newDefaultLocale);
}

/* ### Display name **************************************************/

/*
 * Lookup a resource bundle table item with fallback on the table level.
 * Regular resource bundle lookups perform fallback to parent locale bundles
 * and eventually the root bundle, but only for top-level items.
 * This function takes the name of a top-level table and of an item in that table
 * and performs a lookup of both, falling back until a bundle contains a table
 * with this item.
 *
 * Note: Only the opening of entire bundles falls back through the default locale
 * before root. Once a bundle is open, item lookups do not go through the
 * default locale because that would result in a mix of languages that is
 * unpredictable to the programmer and most likely useless.
 */
static const UChar *
_res_getTableStringWithFallback(const char *path, const char *locale,
                              const char *tableKey, const char *subTableKey,
                              const char *itemKey,
                              int32_t *pLength,
                              UErrorCode *pErrorCode)
{
/*    char localeBuffer[ULOC_FULLNAME_CAPACITY*4];*/
    UResourceBundle *rb=NULL, table, subTable;
    const UChar *item=NULL;
    UErrorCode errorCode;
    char explicitFallbackName[ULOC_FULLNAME_CAPACITY] = {0};

    /*
     * open the bundle for the current locale
     * this falls back through the locale's chain to root
     */
    errorCode=U_ZERO_ERROR;
    rb=ures_open(path, locale, &errorCode);
    if(U_FAILURE(errorCode)) {
        /* total failure, not even root could be opened */
        *pErrorCode=errorCode;
        return NULL;
    } else if(errorCode==U_USING_DEFAULT_WARNING ||
                (errorCode==U_USING_FALLBACK_WARNING && *pErrorCode!=U_USING_DEFAULT_WARNING)
    ) {
        /* set the "strongest" error code (success->fallback->default->failure) */
        *pErrorCode=errorCode;
    }

    for(;;){
        ures_initStackObject(&table);
        ures_initStackObject(&subTable);
        ures_getByKeyWithFallback(rb, tableKey, &table, &errorCode);
        if (subTableKey != NULL) {
            /*
            ures_getByKeyWithFallback(&table,subTableKey, &subTable, &errorCode);
            item = ures_getStringByKeyWithFallback(&subTable, itemKey, pLength, &errorCode);
            if(U_FAILURE(errorCode)){
                *pErrorCode = errorCode;
            }
            
            break;*/
            
            ures_getByKeyWithFallback(&table,subTableKey, &table, &errorCode);
        }
        if(U_SUCCESS(errorCode)){
            item = ures_getStringByKeyWithFallback(&table, itemKey, pLength, &errorCode);
            if(U_FAILURE(errorCode)){
                const char* replacement = NULL;
                *pErrorCode = errorCode; /*save the errorCode*/
                errorCode = U_ZERO_ERROR;
                /* may be a deprecated code */
                if(uprv_strcmp(tableKey, "Countries")==0){
                    replacement =  uloc_getCurrentCountryID(itemKey);
                }else if(uprv_strcmp(tableKey, "Languages")==0){
                    replacement =  uloc_getCurrentLanguageID(itemKey);
                }
                /*pointer comparison is ok since uloc_getCurrentCountryID & uloc_getCurrentLanguageID return the key itself is replacement is not found*/
                if(replacement!=NULL && itemKey != replacement){
                    item = ures_getStringByKeyWithFallback(&table, replacement, pLength, &errorCode);
                    if(U_SUCCESS(errorCode)){
                        *pErrorCode = errorCode;
                        break;
                    }
                }
            }else{
                break;
            }
        }
        
        if(U_FAILURE(errorCode)){    

            /* still can't figure out ?.. try the fallback mechanism */
            int32_t len = 0;
            const UChar* fallbackLocale =  NULL;
            *pErrorCode = errorCode;
            errorCode = U_ZERO_ERROR;

            fallbackLocale = ures_getStringByKeyWithFallback(&table, "Fallback", &len, &errorCode);
            if(U_FAILURE(errorCode)){
               *pErrorCode = errorCode;
                break;
            }
            
            u_UCharsToChars(fallbackLocale, explicitFallbackName, len);
            
            /* guard against recursive fallback */
            if(uprv_strcmp(explicitFallbackName, locale)==0){
                *pErrorCode = U_INTERNAL_PROGRAM_ERROR;
                break;
            }
            ures_close(rb);
            rb = ures_open(NULL, explicitFallbackName, &errorCode);
            if(U_FAILURE(errorCode)){
                *pErrorCode = errorCode;
                break;
            }
            /* succeeded in opening the fallback bundle .. continue and try to fetch the item */
        }else{
            break;
        }
    }
    /* done with the locale string - ready to close table and rb */
    ures_close(&subTable);
    ures_close(&table);
    ures_close(rb);
    return item;
}

static int32_t
_getStringOrCopyKey(const char *path, const char *locale,
                    const char *tableKey, 
                    const char* subTableKey,
                    const char *itemKey,
                    const char *substitute,
                    UChar *dest, int32_t destCapacity,
                    UErrorCode *pErrorCode) {
    const UChar *s = NULL;
    int32_t length = 0;

    if(itemKey==NULL) {
        /* top-level item: normal resource bundle access */
        UResourceBundle *rb;

        rb=ures_open(path, locale, pErrorCode);
        if(U_SUCCESS(*pErrorCode)) {
            s=ures_getStringByKey(rb, tableKey, &length, pErrorCode);
            /* see comment about closing rb near "return item;" in _res_getTableStringWithFallback() */
            ures_close(rb);
        }
    } else {
        /* second-level item, use special fallback */
        s=_res_getTableStringWithFallback(path, locale,
                                           tableKey, 
                                           subTableKey,
                                           itemKey,
                                           &length,
                                           pErrorCode);
    }
    if(U_SUCCESS(*pErrorCode)) {
        int32_t copyLength=uprv_min(length, destCapacity);
        if(copyLength>0 && s != NULL) {
            u_memcpy(dest, s, copyLength);
        }
    } else {
        /* no string from a resource bundle: convert the substitute */
        length=(int32_t)uprv_strlen(substitute);
        u_charsToUChars(substitute, dest, uprv_min(length, destCapacity));
        *pErrorCode=U_USING_DEFAULT_WARNING;
    }

    return u_terminateUChars(dest, destCapacity, length, pErrorCode);
}

static int32_t
_getDisplayNameForComponent(const char *locale,
                            const char *displayLocale,
                            UChar *dest, int32_t destCapacity,
                            int32_t (*getter)(const char *, char *, int32_t, UErrorCode *),
                            const char *tag,
                            UErrorCode *pErrorCode) {
    char localeBuffer[ULOC_FULLNAME_CAPACITY*4];
    int32_t length;
    UErrorCode localStatus;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    localStatus = U_ZERO_ERROR;
    length=(*getter)(locale, localeBuffer, sizeof(localeBuffer), &localStatus);
    if(U_FAILURE(localStatus) || localStatus==U_STRING_NOT_TERMINATED_WARNING) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(length==0) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    }

    return _getStringOrCopyKey(NULL, displayLocale,
                               tag, NULL, localeBuffer,
                               localeBuffer, 
                               dest, destCapacity,
                               pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayLanguage(const char *locale,
                        const char *displayLocale,
                        UChar *dest, int32_t destCapacity,
                        UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getLanguage, _kLanguages, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayScript(const char* locale,
                      const char* displayLocale,
                      UChar *dest, int32_t destCapacity,
                      UErrorCode *pErrorCode)
{
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getScript, _kScripts, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayCountry(const char *locale,
                       const char *displayLocale,
                       UChar *dest, int32_t destCapacity,
                       UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getCountry, _kCountries, pErrorCode);
}

/*
 * TODO separate variant1_variant2_variant3...
 * by getting each tag's display string and concatenating them with ", "
 * in between - similar to uloc_getDisplayName()
 */
U_CAPI int32_t U_EXPORT2
uloc_getDisplayVariant(const char *locale,
                       const char *displayLocale,
                       UChar *dest, int32_t destCapacity,
                       UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getVariant, _kVariants, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayName(const char *locale,
                    const char *displayLocale,
                    UChar *dest, int32_t destCapacity,
                    UErrorCode *pErrorCode)
{
    int32_t length, length2, length3 = 0;
    UBool hasLanguage, hasScript, hasCountry, hasVariant, hasKeywords;
    UEnumeration* keywordEnum = NULL;
    int32_t keywordCount = 0;
    const char *keyword = NULL;
    int32_t keywordLen = 0;
    char keywordValue[256];
    int32_t keywordValueLen = 0;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /*
     * if there is a language, then write "language (country, variant)"
     * otherwise write "country, variant"
     */

    /* write the language */
    length=uloc_getDisplayLanguage(locale, displayLocale,
                                   dest, destCapacity,
                                   pErrorCode);
    hasLanguage= length>0;

    if(hasLanguage) {
        /* append " (" */
        if(length<destCapacity) {
            dest[length]=0x20;
        }
        ++length;
        if(length<destCapacity) {
            dest[length]=0x28;
        }
        ++length;
    }

    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* keep preflighting */
        *pErrorCode=U_ZERO_ERROR;
    }

    /* append the script */
    if(length<destCapacity) {
        length2=uloc_getDisplayScript(locale, displayLocale,
                                       dest+length, destCapacity-length,
                                       pErrorCode);
    } else {
        length2=uloc_getDisplayScript(locale, displayLocale,
                                       NULL, 0,
                                       pErrorCode);
    }
    hasScript= length2>0;
    length+=length2;

    if(hasScript) {
        /* append ", " */
        if(length<destCapacity) {
            dest[length]=0x2c;
        }
        ++length;
        if(length<destCapacity) {
            dest[length]=0x20;
        }
        ++length;
    }

    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* keep preflighting */
        *pErrorCode=U_ZERO_ERROR;
    }

    /* append the country */
    if(length<destCapacity) {
        length2=uloc_getDisplayCountry(locale, displayLocale,
                                       dest+length, destCapacity-length,
                                       pErrorCode);
    } else {
        length2=uloc_getDisplayCountry(locale, displayLocale,
                                       NULL, 0,
                                       pErrorCode);
    }
    hasCountry= length2>0;
    length+=length2;

    if(hasCountry) {
        /* append ", " */
        if(length<destCapacity) {
            dest[length]=0x2c;
        }
        ++length;
        if(length<destCapacity) {
            dest[length]=0x20;
        }
        ++length;
    }

    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* keep preflighting */
        *pErrorCode=U_ZERO_ERROR;
    }

    /* append the variant */
    if(length<destCapacity) {
        length2=uloc_getDisplayVariant(locale, displayLocale,
                                       dest+length, destCapacity-length,
                                       pErrorCode);
    } else {
        length2=uloc_getDisplayVariant(locale, displayLocale,
                                       NULL, 0,
                                       pErrorCode);
    }
    hasVariant= length2>0;
    length+=length2;

    if(hasVariant) {
        /* append ", " */
        if(length<destCapacity) {
            dest[length]=0x2c;
        }
        ++length;
        if(length<destCapacity) {
            dest[length]=0x20;
        }
        ++length;
    }

    keywordEnum = uloc_openKeywords(locale, pErrorCode);
    
    for(keywordCount = uenum_count(keywordEnum, pErrorCode); keywordCount > 0 ; keywordCount--){
          if(U_FAILURE(*pErrorCode)){
              break;
          }
          /* the uenum_next returns NUL terminated string */
          keyword = uenum_next(keywordEnum, &keywordLen, pErrorCode);
          if(length + length3 < destCapacity) {
            length3 += uloc_getDisplayKeyword(keyword, displayLocale, dest+length+length3, destCapacity-length-length3, pErrorCode);
          } else {
            length3 += uloc_getDisplayKeyword(keyword, displayLocale, NULL, 0, pErrorCode);
          }
          if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
              /* keep preflighting */
              *pErrorCode=U_ZERO_ERROR;
          }
          keywordValueLen = uloc_getKeywordValue(locale, keyword, keywordValue, 256, pErrorCode);
          if(keywordValueLen) {
            if(length + length3 < destCapacity) {
              dest[length + length3] = 0x3D;
            }
            length3++;
            if(length + length3 < destCapacity) {
              length3 += uloc_getDisplayKeywordValue(locale, keyword, displayLocale, dest+length+length3, destCapacity-length-length3, pErrorCode);
            } else {
              length3 += uloc_getDisplayKeywordValue(locale, keyword, displayLocale, NULL, 0, pErrorCode);
            }
            if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                /* keep preflighting */
                *pErrorCode=U_ZERO_ERROR;
            }
          }
          if(keywordCount > 1) {
            if(length + length3 + 1 < destCapacity && keywordCount) {
              dest[length + length3]=0x2c;
              dest[length + length3+1]=0x20;
            }
            length3++; /* ',' */
            length3++; /* ' ' */ 
          }
    }
    uenum_close(keywordEnum);

    hasKeywords = length3 > 0;
    length += length3;



    if ((hasScript && !hasCountry)
        || ((hasScript || hasCountry) && !hasVariant && !hasKeywords)
        || ((hasScript || hasCountry || hasVariant) && !hasKeywords)
        || (hasLanguage && !hasScript && !hasCountry && !hasVariant && !hasKeywords))
    {
        /* remove ", " or " (" */
        length-=2;
    }

    if (hasLanguage && (hasScript || hasCountry || hasVariant || hasKeywords)) {
        /* append ")" */
        if(length<destCapacity) {
            dest[length]=0x29;
        }
        ++length;
    }

    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* keep preflighting */
        *pErrorCode=U_ZERO_ERROR;
    }

    return u_terminateUChars(dest, destCapacity, length, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayKeyword(const char* keyword,
                       const char* displayLocale,
                       UChar* dest,
                       int32_t destCapacity,
                       UErrorCode* status){

    /* argument checking */
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }


    /* pass itemKey=NULL to look for a top-level item */
    return _getStringOrCopyKey(NULL, displayLocale,
                               _kKeys, NULL, 
                               keyword, 
                               keyword,      
                               dest, destCapacity,
                               status);

}


#define UCURRENCY_DISPLAY_NAME_INDEX 1

U_CAPI int32_t U_EXPORT2
uloc_getDisplayKeywordValue(   const char* locale,
                               const char* keyword,
                               const char* displayLocale,
                               UChar* dest,
                               int32_t destCapacity,
                               UErrorCode* status){


    char keywordValue[ULOC_FULLNAME_CAPACITY*4];
    int32_t capacity = ULOC_FULLNAME_CAPACITY*4;
    int32_t keywordValueLen =0;

    /* argument checking */
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* get the keyword value */
    keywordValue[0]=0;
    keywordValueLen = uloc_getKeywordValue(locale, keyword, keywordValue, capacity, status);

    /* 
     * if the keyword is equal to currency .. then to get the display name 
     * we need to do the fallback ourselves
     */
    if(uprv_stricmp(keyword, _kCurrency)==0){

        int32_t dispNameLen = 0;
        const UChar *dispName = NULL;
        
        UResourceBundle *bundle     = ures_open(NULL, displayLocale, status);
        UResourceBundle *currencies = ures_getByKey(bundle, _kCurrencies, NULL, status);
        UResourceBundle *currency   = ures_getByKeyWithFallback(currencies, keywordValue, NULL, status);
        
        dispName = ures_getStringByIndex(currency, UCURRENCY_DISPLAY_NAME_INDEX, &dispNameLen, status);
        
        /*close the bundles */
        ures_close(currency);
        ures_close(currencies);
        ures_close(bundle);
        
        if(U_FAILURE(*status)){
            if(*status == U_MISSING_RESOURCE_ERROR){
                /* we just want to write the value over if nothing is available */
                *status = U_USING_DEFAULT_WARNING;
            }else{
                return 0;
            }
        }

        /* now copy the dispName over if not NULL */
        if(dispName != NULL){
            if(dispNameLen <= destCapacity){
                uprv_memcpy(dest, dispName, dispNameLen * U_SIZEOF_UCHAR);
                return u_terminateUChars(dest, destCapacity, dispNameLen, status);
            }else{
                *status = U_BUFFER_OVERFLOW_ERROR;
                return dispNameLen;
            }
        }else{
            /* we have not found the display name for the value .. just copy over */
            if(keywordValueLen <= destCapacity){
                u_charsToUChars(keywordValue, dest, keywordValueLen);
                return u_terminateUChars(dest, destCapacity, keywordValueLen, status);
            }else{
                 *status = U_BUFFER_OVERFLOW_ERROR;
                return keywordValueLen;
            }
        }

        
    }else{

        return _getStringOrCopyKey(NULL, displayLocale,
                                   _kTypes, keyword, 
                                   keywordValue,
                                   keywordValue,
                                   dest, destCapacity,
                                   status);
    }
}

/* ### Get available **************************************************/

static UBool U_CALLCONV uloc_cleanup(void) {
    char ** temp;

    if (_installedLocales) {
        temp = _installedLocales;
        _installedLocales = NULL;

        _installedLocalesCount = 0;

        uprv_free(temp);
    }
    return TRUE;
}

static void _load_installedLocales()
{
    UBool   localesLoaded;

    UMTX_CHECK(NULL, _installedLocales != NULL, localesLoaded);
    
    if (localesLoaded == FALSE) {
        UResourceBundle *index = NULL;
        UResourceBundle installed;
        UErrorCode status = U_ZERO_ERROR;
        char ** temp;
        int32_t i = 0;
        int32_t localeCount;
        
        ures_initStackObject(&installed);
        index = ures_openDirect(NULL, _kIndexLocaleName, &status);
        ures_getByKey(index, _kIndexTag, &installed, &status);
        
        if(U_SUCCESS(status)) {
            localeCount = ures_getSize(&installed);
            temp = (char **) uprv_malloc(sizeof(char*) * (localeCount+1));
            /* Check for null pointer */
            if (temp != NULL) {
                ures_resetIterator(&installed);
                while(ures_hasNext(&installed)) {
                    ures_getNextString(&installed, NULL, (const char **)&temp[i++], &status);
                }
                temp[i] = NULL;

                umtx_lock(NULL);
                if (_installedLocales == NULL)
                {
                    _installedLocales = temp;
                    _installedLocalesCount = localeCount;
                    temp = NULL;
                    ucln_common_registerCleanup(UCLN_COMMON_ULOC, uloc_cleanup);
                } 
                umtx_unlock(NULL);

                uprv_free(temp);
            }
        }
        ures_close(&installed);
        ures_close(index);
    }
}

U_CAPI const char* U_EXPORT2
uloc_getAvailable(int32_t offset) 
{
    
    _load_installedLocales();
    
    if (offset > _installedLocalesCount)
        return NULL;
    return _installedLocales[offset];
}

U_CAPI int32_t  U_EXPORT2
uloc_countAvailable()
{
    _load_installedLocales();
    return _installedLocalesCount;
}

/**
 * Returns a list of all language codes defined in ISO 639.  This is a pointer
 * to an array of pointers to arrays of char.  All of these pointers are owned
 * by ICU-- do not delete them, and do not write through them.  The array is
 * terminated with a null pointer.
 */
U_CAPI const char* const*  U_EXPORT2
uloc_getISOLanguages() 
{
    return LANGUAGES;
}

/**
 * Returns a list of all 2-letter country codes defined in ISO 639.  This is a
 * pointer to an array of pointers to arrays of char.  All of these pointers are
 * owned by ICU-- do not delete them, and do not write through them.  The array is
 * terminated with a null pointer.
 */
U_CAPI const char* const*  U_EXPORT2
uloc_getISOCountries() 
{
    return COUNTRIES;
}


/* this function to be moved into cstring.c later */
static char gDecimal = 0;

static /* U_CAPI */
double
/* U_EXPORT2 */
_uloc_strtod(const char *start, char **end) {
    char *decimal;
    char *myEnd;
    char buf[30];
    double rv;
    if (!gDecimal) {
        char rep[5];
        /* For machines that decide to change the decimal on you,
        and try to be too smart with localization.
        This normally should be just a '.'. */
        sprintf(rep, "%+1.1f", 1.0);
        gDecimal = rep[2];
    }

    if(gDecimal == '.') {
        return uprv_strtod(start, end); /* fall through to OS */
    } else {
        uprv_strncpy(buf, start, 29);
        buf[29]=0;
        decimal = uprv_strchr(buf, '.');
        if(decimal) {
            *decimal = gDecimal;
        } else {
            return uprv_strtod(start, end); /* no decimal point */
        }
        rv = uprv_strtod(buf, &myEnd);
        if(end) {
            *end = (char*)(start+(myEnd-buf)); /* cast away const (to follow uprv_strtod API.) */
        }
        return rv;
    }
}

typedef struct { 
    float q;
    int32_t dummy;  /* to avoid uninitialized memory copy from qsort */
    char *locale;
} _acceptLangItem;

static int32_t U_CALLCONV
uloc_acceptLanguageCompare(const void *context, const void *a, const void *b)
{
    const _acceptLangItem *aa = (const _acceptLangItem*)a;
    const _acceptLangItem *bb = (const _acceptLangItem*)b;

    int32_t rc = 0;
    if(bb->q < aa->q) {
        rc = -1;  /* A > B */
    } else if(bb->q > aa->q) {
        rc = 1;   /* A < B */
    } else {
        rc = 0;   /* A = B */
    }

    if(rc==0) {
        rc = uprv_stricmp(aa->locale, bb->locale);
    }

#if defined(ULOC_DEBUG)
    /*  fprintf(stderr, "a:[%s:%g], b:[%s:%g] -> %d\n", 
    aa->locale, aa->q, 
    bb->locale, bb->q,
    rc);*/
#endif

    return rc;
}

/* 
mt-mt, ja;q=0.76, en-us;q=0.95, en;q=0.92, en-gb;q=0.89, fr;q=0.87, iu-ca;q=0.84, iu;q=0.82, ja-jp;q=0.79, mt;q=0.97, de-de;q=0.74, de;q=0.71, es;q=0.68, it-it;q=0.66, it;q=0.63, vi-vn;q=0.61, vi;q=0.58, nl-nl;q=0.55, nl;q=0.53
*/

U_CAPI int32_t U_EXPORT2
uloc_acceptLanguageFromHTTP(char *result, int32_t resultAvailable, UAcceptResult *outResult,
                            const char *httpAcceptLanguage,
                            UEnumeration* availableLocales,
                            UErrorCode *status)
{
    _acceptLangItem *j;
    _acceptLangItem smallBuffer[30];
    char **strs;
    char tmp[ULOC_FULLNAME_CAPACITY +1];
    int32_t n = 0;
    const char *itemEnd;
    const char *paramEnd;
    const char *s;
    const char *t;
    int32_t res;
    int32_t i;
    int32_t l = (int32_t)uprv_strlen(httpAcceptLanguage);
    int32_t jSize;
    char *tempstr; /* Use for null pointer check */

    j = smallBuffer;
    jSize = sizeof(smallBuffer)/sizeof(smallBuffer[0]);
    if(U_FAILURE(*status)) {
        return -1;
    }

    for(s=httpAcceptLanguage;s&&*s;) {
        while(isspace(*s)) /* eat space at the beginning */
            s++;
        itemEnd=uprv_strchr(s,',');
        paramEnd=uprv_strchr(s,';');
        if(!itemEnd) {
            itemEnd = httpAcceptLanguage+l; /* end of string */
        }
        if(paramEnd && paramEnd<itemEnd) { 
            /* semicolon (;) is closer than end (,) */
            t = paramEnd+1;
            if(*t=='q') {
                t++;
            }
            while(isspace(*t)) {
                t++;
            }
            if(*t=='=') {
                t++;
            }
            while(isspace(*t)) {
                t++;
            }
            j[n].q = (float)_uloc_strtod(t,NULL);
        } else {
            /* no semicolon - it's 1.0 */
            j[n].q = 1.0f;
            paramEnd = itemEnd;
        }
        j[n].dummy=0;
        /* eat spaces prior to semi */
        for(t=(paramEnd-1);(paramEnd>s)&&isspace(*t);t--)
            ;
        /* Check for null pointer from uprv_strndup */
        tempstr = uprv_strndup(s,(int32_t)((t+1)-s));
        if (tempstr == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return -1;
        }
        j[n].locale = tempstr;
        uloc_canonicalize(j[n].locale,tmp,sizeof(tmp)/sizeof(tmp[0]),status);
        if(strcmp(j[n].locale,tmp)) {
            uprv_free(j[n].locale);
            j[n].locale=uprv_strdup(tmp);
        }
#if defined(ULOC_DEBUG)
        /*fprintf(stderr,"%d: s <%s> q <%g>\n", n, j[n].locale, j[n].q);*/
#endif
        n++;
        s = itemEnd;
        while(*s==',') { /* eat duplicate commas */
            s++;
        }
        if(n>=jSize) {
            if(j==smallBuffer) {  /* overflowed the small buffer. */
                j = uprv_malloc(sizeof(j[0])*(jSize*2));
                if(j!=NULL) {
                    uprv_memcpy(j,smallBuffer,sizeof(j[0])*jSize);
                }
#if defined(ULOC_DEBUG)
                fprintf(stderr,"malloced at size %d\n", jSize);
#endif
            } else {
                j = uprv_realloc(j, sizeof(j[0])*jSize*2);
#if defined(ULOC_DEBUG)
                fprintf(stderr,"re-alloced at size %d\n", jSize);
#endif
            }
            jSize *= 2;
            if(j==NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return -1;
            }
        }
    }
    uprv_sortArray(j, n, sizeof(j[0]), uloc_acceptLanguageCompare, NULL, TRUE, status);
    if(U_FAILURE(*status)) {
        if(j != smallBuffer) {
#if defined(ULOC_DEBUG)
            fprintf(stderr,"freeing j %p\n", j);
#endif
            uprv_free(j);
        }
        return -1;
    }
    strs = uprv_malloc((size_t)(sizeof(strs[0])*n));
    /* Check for null pointer */
    if (strs == NULL) {
        uprv_free(j); /* Free to avoid memory leak */
        *status = U_MEMORY_ALLOCATION_ERROR;
        return -1;
    }
    for(i=0;i<n;i++) {
#if defined(ULOC_DEBUG)
        /*fprintf(stderr,"%d: s <%s> q <%g>\n", i, j[i].locale, j[i].q);*/
#endif
        strs[i]=j[i].locale;
    }
    res =  uloc_acceptLanguage(result, resultAvailable, outResult, 
        (const char**)strs, n, availableLocales, status);
    for(i=0;i<n;i++) {
        uprv_free(strs[i]);
    }
    uprv_free(strs);
    if(j != smallBuffer) {
#if defined(ULOC_DEBUG)
        fprintf(stderr,"freeing j %p\n", j);
#endif
        uprv_free(j);
    }
    return res;
}


U_CAPI int32_t U_EXPORT2
uloc_acceptLanguage(char *result, int32_t resultAvailable, 
                    UAcceptResult *outResult, const char **acceptList,
                    int32_t acceptListCount,
                    UEnumeration* availableLocales,
                    UErrorCode *status)
{
    int32_t i,j;
    int32_t len;
    int32_t maxLen=0;
    char tmp[ULOC_FULLNAME_CAPACITY+1];
    const char *l;
    char **fallbackList;
    if(U_FAILURE(*status)) {
        return -1;
    }
    fallbackList = uprv_malloc((size_t)(sizeof(fallbackList[0])*acceptListCount));
    if(fallbackList==NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return -1;
    }
    for(i=0;i<acceptListCount;i++) {
#if defined(ULOC_DEBUG)
        fprintf(stderr,"%02d: %s\n", i, acceptList[i]);
#endif
        while((l=uenum_next(availableLocales, NULL, status))) {
#if defined(ULOC_DEBUG)
            fprintf(stderr,"  %s\n", l);
#endif
            len = (int32_t)uprv_strlen(l);
            if(!uprv_strcmp(acceptList[i], l)) {
                if(outResult) { 
                    *outResult = ULOC_ACCEPT_VALID;
                }
#if defined(ULOC_DEBUG)
                fprintf(stderr, "MATCH! %s\n", l);
#endif
                if(len>0) {
                    uprv_strncpy(result, l, uprv_min(len, resultAvailable));
                }
                for(j=0;j<i;j++) {
                    uprv_free(fallbackList[j]);
                }
                uprv_free(fallbackList);
                return u_terminateChars(result, resultAvailable, len, status);   
            }
            if(len>maxLen) {
                maxLen = len;
            }
        }
        uenum_reset(availableLocales, status);    
        /* save off parent info */
        if(uloc_getParent(acceptList[i], tmp, sizeof(tmp)/sizeof(tmp[0]), status)!=0) {
            fallbackList[i] = uprv_strdup(tmp);
        } else {
            fallbackList[i]=0;
        }
    }

    for(maxLen--;maxLen>0;maxLen--) {
        for(i=0;i<acceptListCount;i++) {
            if(fallbackList[i] && ((int32_t)uprv_strlen(fallbackList[i])==maxLen)) {
#if defined(ULOC_DEBUG)
                fprintf(stderr,"Try: [%s]", fallbackList[i]);
#endif
                while((l=uenum_next(availableLocales, NULL, status))) {
#if defined(ULOC_DEBUG)
                    fprintf(stderr,"  %s\n", l);
#endif
                    len = (int32_t)uprv_strlen(l);
                    if(!uprv_strcmp(fallbackList[i], l)) {
                        if(outResult) { 
                            *outResult = ULOC_ACCEPT_FALLBACK;
                        }
#if defined(ULOC_DEBUG)
                        fprintf(stderr, "fallback MATCH! %s\n", l);
#endif
                        if(len>0) {
                            uprv_strncpy(result, l, uprv_min(len, resultAvailable));
                        }
                        for(j=0;j<acceptListCount;j++) {
                            uprv_free(fallbackList[j]);
                        }
                        uprv_free(fallbackList);
                        return u_terminateChars(result, resultAvailable, len, status);
                    }
                }
                uenum_reset(availableLocales, status);    

                if(uloc_getParent(fallbackList[i], tmp, sizeof(tmp)/sizeof(tmp[0]), status)!=0) {
                    uprv_free(fallbackList[i]);
                    fallbackList[i] = uprv_strdup(tmp);
                } else {
                    uprv_free(fallbackList[i]);
                    fallbackList[i]=0;
                }
            }
        }
        if(outResult) { 
            *outResult = ULOC_ACCEPT_FAILED;
        }
    }
    for(i=0;i<acceptListCount;i++) {
        uprv_free(fallbackList[i]);
    }
    uprv_free(fallbackList);
    return -1;
}

/*
 * A struct that contains a pair of pointers to strings.
 * The key member contains a minimal set of subtags, and the
 * value contains the corresponding maximal set of subtags.
 */
typedef struct StringPair_tag {
  const char* minimal;
  const char* maximal;
} StringPair;

static int32_t U_CALLCONV
compareStringPairStructs(const void *left, const void *right) {
    const char* leftString = ((const StringPair *)left)->minimal;
    const char* rightString = ((const StringPair *)right)->minimal;
    return uprv_strcmp(leftString, rightString);
}

/*
 * This array maps a set of tags to its likely maximal set of tags.
 */
const StringPair likely_subtags[] = {
  {
    // { Afar; ?; ? } => { Afar; Latin; Ethiopia }
    "aa",
    "aa_Latn_ET"
  }, {
    // { Afrikaans; ?; ? } => { Afrikaans; Latin; South Africa }
    "af",
    "af_Latn_ZA"
  }, {
    // { Akan; ?; ? } => { Akan; Latin; Ghana }
    "ak",
    "ak_Latn_GH"
  }, {
    // { Amharic; ?; ? } => { Amharic; Ethiopic; Ethiopia }
    "am",
    "am_Ethi_ET"
  }, {
    // { Arabic; ?; ? } => { Arabic; Arabic; Egypt }
    "ar",
    "ar_Arab_EG"
  }, {
    // { Assamese; ?; ? } => { Assamese; Bengali; India }
    "as",
    "as_Beng_IN"
  }, {
    // { Azerbaijani; ?; ? } => { Azerbaijani; Latin; Azerbaijan }
    "az",
    "az_Latn_AZ"
  }, {
    // { Belarusian; ?; ? } => { Belarusian; Cyrillic; Belarus }
    "be",
    "be_Cyrl_BY"
  }, {
    // { Bulgarian; ?; ? } => { Bulgarian; Cyrillic; Bulgaria }
    "bg",
    "bg_Cyrl_BG"
  }, {
    // { Bengali; ?; ? } => { Bengali; Bengali; Bangladesh }
    "bn",
    "bn_Beng_BD"
  }, {
    // { Tibetan; ?; ? } => { Tibetan; Tibetan; China }
    "bo",
    "bo_Tibt_CN"
  }, {
    // { Bosnian; ?; ? } => { Bosnian; Latin; Bosnia and Herzegovina }
    "bs",
    "bs_Latn_BA"
  }, {
    // { Blin; ?; ? } => { Blin; Ethiopic; Eritrea }
    "byn",
    "byn_Ethi_ER"
  }, {
    // { Catalan; ?; ? } => { Catalan; Latin; Spain }
    "ca",
    "ca_Latn_ES"
  }, {
    // { Atsam; ?; ? } => { Atsam; Latin; Nigeria }
    "cch",
    "cch_Latn_NG"
  }, {
    // { Chamorro; ?; ? } => { Chamorro; Latin; Guam }
    "ch",
    "ch_Latn_GU"
  }, {
    // { Chuukese; ?; ? } => { Chuukese; Latin; Micronesia }
    "chk",
    "chk_Latn_FM"
  }, {
    // { Coptic; ?; ? } => { Coptic; Arabic; Egypt }
    "cop",
    "cop_Arab_EG"
  }, {
    // { Czech; ?; ? } => { Czech; Latin; Czech Republic }
    "cs",
    "cs_Latn_CZ"
  }, {
    // { Welsh; ?; ? } => { Welsh; Latin; United Kingdom }
    "cy",
    "cy_Latn_GB"
  }, {
    // { Danish; ?; ? } => { Danish; Latin; Denmark }
    "da",
    "da_Latn_DK"
  }, {
    // { German; ?; ? } => { German; Latin; Germany }
    "de",
    "de_Latn_DE"
  }, {
    // { Divehi; ?; ? } => { Divehi; Thaana; Maldives }
    "dv",
    "dv_Thaa_MV"
  }, {
    // { Dzongkha; ?; ? } => { Dzongkha; Tibetan; Bhutan }
    "dz",
    "dz_Tibt_BT"
  }, {
    // { Ewe; ?; ? } => { Ewe; Latin; Ghana }
    "ee",
    "ee_Latn_GH"
  }, {
    // { Greek; ?; ? } => { Greek; Greek; Greece }
    "el",
    "el_Grek_GR"
  }, {
    // { English; ?; ? } => { English; Latin; United States }
    "en",
    "en_Latn_US"
  }, {
    // { Spanish; ?; ? } => { Spanish; Latin; Spain }
    "es",
    "es_Latn_ES"
  }, {
    // { Estonian; ?; ? } => { Estonian; Latin; Estonia }
    "et",
    "et_Latn_EE"
  }, {
    // { Basque; ?; ? } => { Basque; Latin; Spain }
    "eu",
    "eu_Latn_ES"
  }, {
    // { Persian; ?; ? } => { Persian; Arabic; Iran }
    "fa",
    "fa_Arab_IR"
  }, {
    // { Finnish; ?; ? } => { Finnish; Latin; Finland }
    "fi",
    "fi_Latn_FI"
  }, {
    // { Filipino; ?; ? } => { Filipino; Latin; Philippines }
    "fil",
    "fil_Latn_PH"
  }, {
    // { Fijian; ?; ? } => { Fijian; Latin; Fiji }
    "fj",
    "fj_Latn_FJ"
  }, {
    // { Faroese; ?; ? } => { Faroese; Latin; Faroe Islands }
    "fo",
    "fo_Latn_FO"
  }, {
    // { French; ?; ? } => { French; Latin; France }
    "fr",
    "fr_Latn_FR"
  }, {
    // { Friulian; ?; ? } => { Friulian; Latin; Italy }
    "fur",
    "fur_Latn_IT"
  }, {
    // { Irish; ?; ? } => { Irish; Latin; Ireland }
    "ga",
    "ga_Latn_IE"
  }, {
    // { Ga; ?; ? } => { Ga; Latin; Ghana }
    "gaa",
    "gaa_Latn_GH"
  }, {
    // { Geez; ?; ? } => { Geez; Ethiopic; Eritrea }
    "gez",
    "gez_Ethi_ER"
  }, {
    // { Galician; ?; ? } => { Galician; Latin; Spain }
    "gl",
    "gl_Latn_ES"
  }, {
    // { Guarani; ?; ? } => { Guarani; Latin; Paraguay }
    "gn",
    "gn_Latn_PY"
  }, {
    // { Gujarati; ?; ? } => { Gujarati; Gujarati; India }
    "gu",
    "gu_Gujr_IN"
  }, {
    // { Manx; ?; ? } => { Manx; Latin; United Kingdom }
    "gv",
    "gv_Latn_GB"
  }, {
    // { Hausa; ?; ? } => { Hausa; Latin; Nigeria }
    "ha",
    "ha_Latn_NG"
  }, {
    // { Hawaiian; ?; ? } => { Hawaiian; Latin; United States }
    "haw",
    "haw_Latn_US"
  }, {
    // { Hindi; ?; ? } => { Hindi; Devanagari; India }
    "hi",
    "hi_Deva_IN"
  }, {
    // { Croatian; ?; ? } => { Croatian; Latin; Croatia }
    "hr",
    "hr_Latn_HR"
  }, {
    // { Haitian; ?; ? } => { Haitian; Latin; Haiti }
    "ht",
    "ht_Latn_HT"
  }, {
    // { Hungarian; ?; ? } => { Hungarian; Latin; Hungary }
    "hu",
    "hu_Latn_HU"
  }, {
    // { Armenian; ?; ? } => { Armenian; Armenian; Armenia }
    "hy",
    "hy_Armn_AM"
  }, {
    // { Indonesian; ?; ? } => { Indonesian; Latin; Indonesia }
    "id",
    "id_Latn_ID"
  }, {
    // { Igbo; ?; ? } => { Igbo; Latin; Nigeria }
    "ig",
    "ig_Latn_NG"
  }, {
    // { Sichuan Yi; ?; ? } => { Sichuan Yi; Yi; China }
    "ii",
    "ii_Yiii_CN"
  }, {
    // { Icelandic; ?; ? } => { Icelandic; Latin; Iceland }
    "is",
    "is_Latn_IS"
  }, {
    // { Italian; ?; ? } => { Italian; Latin; Italy }
    "it",
    "it_Latn_IT"
  }, {
    // { Inuktitut; ?; ? }
    //  => { Inuktitut; Unified Canadian Aboriginal Syllabics; Canada }
    "iu",
    "iu_Cans_CA"
  }, {
    // { null; ?; ? } => { null; Hebrew; Israel }
    "iw",
    "iw_Hebr_IL"
  }, {
    // { Japanese; ?; ? } => { Japanese; Japanese; Japan }
    "ja",
    "ja_Jpan_JP"
  }, {
    // { Georgian; ?; ? } => { Georgian; Georgian; Georgia }
    "ka",
    "ka_Geor_GE"
  }, {
    // { Jju; ?; ? } => { Jju; Latin; Nigeria }
    "kaj",
    "kaj_Latn_NG"
  }, {
    // { Kamba; ?; ? } => { Kamba; Latin; Kenya }
    "kam",
    "kam_Latn_KE"
  }, {
    // { Tyap; ?; ? } => { Tyap; Latin; Nigeria }
    "kcg",
    "kcg_Latn_NG"
  }, {
    // { Koro; ?; ? } => { Koro; Latin; Nigeria }
    "kfo",
    "kfo_Latn_NG"
  }, {
    // { Kazakh; ?; ? } => { Kazakh; Cyrillic; Kazakhstan }
    "kk",
    "kk_Cyrl_KZ"
  }, {
    // { Kalaallisut; ?; ? } => { Kalaallisut; Latin; Greenland }
    "kl",
    "kl_Latn_GL"
  }, {
    // { Khmer; ?; ? } => { Khmer; Khmer; Cambodia }
    "km",
    "km_Khmr_KH"
  }, {
    // { Kannada; ?; ? } => { Kannada; Kannada; India }
    "kn",
    "kn_Knda_IN"
  }, {
    // { Korean; ?; ? } => { Korean; Korean; South Korea }
    "ko",
    "ko_Kore_KR"
  }, {
    // { Konkani; ?; ? } => { Konkani; Devanagari; India }
    "kok",
    "kok_Deva_IN"
  }, {
    // { Kpelle; ?; ? } => { Kpelle; Latin; Liberia }
    "kpe",
    "kpe_Latn_LR"
  }, {
    // { Kurdish; ?; ? } => { Kurdish; Latin; Turkey }
    "ku",
    "ku_Latn_TR"
  }, {
    // { Cornish; ?; ? } => { Cornish; Latin; United Kingdom }
    "kw",
    "kw_Latn_GB"
  }, {
    // { Kirghiz; ?; ? } => { Kirghiz; Cyrillic; Kyrgyzstan }
    "ky",
    "ky_Cyrl_KG"
  }, {
    // { Latin; ?; ? } => { Latin; Latin; Vatican }
    "la",
    "la_Latn_VA"
  }, {
    // { Lingala; ?; ? } => { Lingala; Latin; Congo _ Kinshasa }
    "ln",
    "ln_Latn_CD"
  }, {
    // { Lao; ?; ? } => { Lao; Lao; Laos }
    "lo",
    "lo_Laoo_LA"
  }, {
    // { Lithuanian; ?; ? } => { Lithuanian; Latin; Lithuania }
    "lt",
    "lt_Latn_LT"
  }, {
    // { Latvian; ?; ? } => { Latvian; Latin; Latvia }
    "lv",
    "lv_Latn_LV"
  }, {
    // { Malagasy; ?; ? } => { Malagasy; Latin; Madagascar }
    "mg",
    "mg_Latn_MG"
  }, {
    // { Marshallese; ?; ? } => { Marshallese; Latin; Marshall Islands }
    "mh",
    "mh_Latn_MH"
  }, {
    // { Macedonian; ?; ? } => { Macedonian; Cyrillic; Macedonia }
    "mk",
    "mk_Cyrl_MK"
  }, {
    // { Malayalam; ?; ? } => { Malayalam; Malayalam; India }
    "ml",
    "ml_Mlym_IN"
  }, {
    // { Mongolian; ?; ? } => { Mongolian; Cyrillic; Mongolia }
    "mn",
    "mn_Cyrl_MN"
  }, {
    // { Marathi; ?; ? } => { Marathi; Devanagari; India }
    "mr",
    "mr_Deva_IN"
  }, {
    // { Malay; ?; ? } => { Malay; Latin; Malaysia }
    "ms",
    "ms_Latn_MY"
  }, {
    // { Maltese; ?; ? } => { Maltese; Latin; Malta }
    "mt",
    "mt_Latn_MT"
  }, {
    // { Burmese; ?; ? } => { Burmese; Myanmar; Myanmar }
    "my",
    "my_Mymr_MM"
  }, {
    // { Nauru; ?; ? } => { Nauru; Latin; Nauru }
    "na",
    "na_Latn_NR"
  }, {
    // { Nepali; ?; ? } => { Nepali; Devanagari; Nepal }
    "ne",
    "ne_Deva_NP"
  }, {
    // { Niuean; ?; ? } => { Niuean; Latin; Niue }
    "niu",
    "niu_Latn_NU"
  }, {
    // { Dutch; ?; ? } => { Dutch; Latin; Netherlands }
    "nl",
    "nl_Latn_NL"
  }, {
    // { Norwegian Nynorsk; ?; ? } => { Norwegian Nynorsk; Latin; Norway }
    "nn",
    "nn_Latn_NO"
  }, {
    // { Norwegian; ?; ? } => { Norwegian; Latin; Norway }
    "no",
    "no_Latn_NO"
  }, {
    // { South Ndebele; ?; ? } => { South Ndebele; Latin; South Africa }
    "nr",
    "nr_Latn_ZA"
  }, {
    // { Northern Sotho; ?; ? } => { Northern Sotho; Latin; South Africa }
    "nso",
    "nso_Latn_ZA"
  }, {
    // { Nyanja; ?; ? } => { Nyanja; Latin; Malawi }
    "ny",
    "ny_Latn_MW"
  }, {
    // { Oromo; ?; ? } => { Oromo; Latin; Ethiopia }
    "om",
    "om_Latn_ET"
  }, {
    // { Oriya; ?; ? } => { Oriya; Oriya; India }
    "or",
    "or_Orya_IN"
  }, {
    // { Punjabi; ?; ? } => { Punjabi; Gurmukhi; India }
    "pa",
    "pa_Guru_IN"
  }, {
    // { Punjabi; Arabic; ? } => { Punjabi; Arabic; Pakistan }
    "pa_Arab",
    "pa_Arab_PK"
  }, {
    // { Punjabi; ?; Pakistan } => { Punjabi; Arabic; Pakistan }
    "pa_PK",
    "pa_Arab_PK"
  }, {
    // { Papiamento; ?; ? } => { Papiamento; Latin; Netherlands Antilles }
    "pap",
    "pap_Latn_AN"
  }, {
    // { Palauan; ?; ? } => { Palauan; Latin; Palau }
    "pau",
    "pau_Latn_PW"
  }, {
    // { Polish; ?; ? } => { Polish; Latin; Poland }
    "pl",
    "pl_Latn_PL"
  }, {
    // { Pashto; ?; ? } => { Pashto; Arabic; Afghanistan }
    "ps",
    "ps_Arab_AF"
  }, {
    // { Portuguese; ?; ? } => { Portuguese; Latin; Brazil }
    "pt",
    "pt_Latn_BR"
  }, {
    // { Rundi; ?; ? } => { Rundi; Latin; Burundi }
    "rn",
    "rn_Latn_BI"
  }, {
    // { Romanian; ?; ? } => { Romanian; Latin; Romania }
    "ro",
    "ro_Latn_RO"
  }, {
    // { Russian; ?; ? } => { Russian; Cyrillic; Russia }
    "ru",
    "ru_Cyrl_RU"
  }, {
    // { Kinyarwanda; ?; ? } => { Kinyarwanda; Latin; Rwanda }
    "rw",
    "rw_Latn_RW"
  }, {
    // { Sanskrit; ?; ? } => { Sanskrit; Devanagari; India }
    "sa",
    "sa_Deva_IN"
  }, {
    // { Northern Sami; ?; ? } => { Northern Sami; Latin; Norway }
    "se",
    "se_Latn_NO"
  }, {
    // { Sango; ?; ? } => { Sango; Latin; Central African Republic }
    "sg",
    "sg_Latn_CF"
  }, {
    // { Serbo_Croatian; ?; ? } => { Serbian; Latin; Serbia }
    "sh",
    "sr_Latn_RS"
  }, {
    // { Sinhalese; ?; ? } => { Sinhalese; Sinhala; Sri Lanka }
    "si",
    "si_Sinh_LK"
  }, {
    // { Sidamo; ?; ? } => { Sidamo; Latin; Ethiopia }
    "sid",
    "sid_Latn_ET"
  }, {
    // { Slovak; ?; ? } => { Slovak; Latin; Slovakia }
    "sk",
    "sk_Latn_SK"
  }, {
    // { Slovenian; ?; ? } => { Slovenian; Latin; Slovenia }
    "sl",
    "sl_Latn_SI"
  }, {
    // { Samoan; ?; ? } => { Samoan; Latin; American Samoa }
    "sm",
    "sm_Latn_AS"
  }, {
    // { Somali; ?; ? } => { Somali; Latin; Somalia }
    "so",
    "so_Latn_SO"
  }, {
    // { Albanian; ?; ? } => { Albanian; Latin; Albania }
    "sq",
    "sq_Latn_AL"
  }, {
    // { Serbian; ?; ? } => { Serbian; Cyrillic; Serbia }
    "sr",
    "sr_Cyrl_RS"
  }, {
    // { Swati; ?; ? } => { Swati; Latin; South Africa }
    "ss",
    "ss_Latn_ZA"
  }, {
    // { Southern Sotho; ?; ? } => { Southern Sotho; Latin; South Africa }
    "st",
    "st_Latn_ZA"
  }, {
    // { Sundanese; ?; ? } => { Sundanese; Latin; Indonesia }
    "su",
    "su_Latn_ID"
  }, {
    // { Swedish; ?; ? } => { Swedish; Latin; Sweden }
    "sv",
    "sv_Latn_SE"
  }, {
    // { Swahili; ?; ? } => { Swahili; Latin; Tanzania }
    "sw",
    "sw_Latn_TZ"
  }, {
    // { Syriac; ?; ? } => { Syriac; Syriac; Syria }
    "syr",
    "syr_Syrc_SY"
  }, {
    // { Tamil; ?; ? } => { Tamil; Tamil; India }
    "ta",
    "ta_Taml_IN"
  }, {
    // { Telugu; ?; ? } => { Telugu; Telugu; India }
    "te",
    "te_Telu_IN"
  }, {
    // { Tetum; ?; ? } => { Tetum; Latin; East Timor }
    "tet",
    "tet_Latn_TL"
  }, {
    // { Tajik; ?; ? } => { Tajik; Cyrillic; Tajikistan }
    "tg",
    "tg_Cyrl_TJ"
  }, {
    // { Thai; ?; ? } => { Thai; Thai; Thailand }
    "th",
    "th_Thai_TH"
  }, {
    // { Tigrinya; ?; ? } => { Tigrinya; Ethiopic; Ethiopia }
    "ti",
    "ti_Ethi_ET"
  }, {
    // { Tigre; ?; ? } => { Tigre; Ethiopic; Eritrea }
    "tig",
    "tig_Ethi_ER"
  }, {
    // { Turkmen; ?; ? } => { Turkmen; Latin; Turkmenistan }
    "tk",
    "tk_Latn_TM"
  }, {
    // { Tokelau; ?; ? } => { Tokelau; Latin; Tokelau }
    "tkl",
    "tkl_Latn_TK"
  }, {
    // { Tswana; ?; ? } => { Tswana; Latin; South Africa }
    "tn",
    "tn_Latn_ZA"
  }, {
    // { Tonga; ?; ? } => { Tonga; Latin; Tonga }
    "to",
    "to_Latn_TO"
  }, {
    // { Tok Pisin; ?; ? } => { Tok Pisin; Latin; Papua New Guinea }
    "tpi",
    "tpi_Latn_PG"
  }, {
    // { Turkish; ?; ? } => { Turkish; Latin; Turkey }
    "tr",
    "tr_Latn_TR"
  }, {
    // { Tsonga; ?; ? } => { Tsonga; Latin; South Africa }
    "ts",
    "ts_Latn_ZA"
  }, {
    // { Tatar; ?; ? } => { Tatar; Cyrillic; Russia }
    "tt",
    "tt_Cyrl_RU"
  }, {
    // { Tuvalu; ?; ? } => { Tuvalu; Latin; Tuvalu }
    "tvl",
    "tvl_Latn_TV"
  }, {
    // { Tahitian; ?; ? } => { Tahitian; Latin; French Polynesia }
    "ty",
    "ty_Latn_PF"
  }, {
    // { Ukrainian; ?; ? } => { Ukrainian; Cyrillic; Ukraine }
    "uk",
    "uk_Cyrl_UA"
  }, {
    // { ?; ?; ? } => { English; Latin; United States }
    "und",
    "en_Latn_US"
  }, {
    // { ?; ?; Andorra } => { Catalan; Latin; Andorra }
    "und_AD",
    "ca_Latn_AD"
  }, {
    // { ?; ?; United Arab Emirates }
    //  => { Arabic; Arabic; United Arab Emirates }
    "und_AE",
    "ar_Arab_AE"
  }, {
    // { ?; ?; Afghanistan } => { Persian; Arabic; Afghanistan }
    "und_AF",
    "fa_Arab_AF"
  }, {
    // { ?; ?; Albania } => { Albanian; Latin; Albania }
    "und_AL",
    "sq_Latn_AL"
  }, {
    // { ?; ?; Armenia } => { Armenian; Armenian; Armenia }
    "und_AM",
    "hy_Armn_AM"
  }, {
    // { ?; ?; Netherlands Antilles }
    //  => { Papiamento; Latin; Netherlands Antilles }
    "und_AN",
    "pap_Latn_AN"
  }, {
    // { ?; ?; Angola } => { Portuguese; Latin; Angola }
    "und_AO",
    "pt_Latn_AO"
  }, {
    // { ?; ?; Argentina } => { Spanish; Latin; Argentina }
    "und_AR",
    "es_Latn_AR"
  }, {
    // { ?; ?; American Samoa } => { Samoan; Latin; American Samoa }
    "und_AS",
    "sm_Latn_AS"
  }, {
    // { ?; ?; Austria } => { German; Latin; Austria }
    "und_AT",
    "de_Latn_AT"
  }, {
    // { ?; ?; Aruba } => { Dutch; Latin; Aruba }
    "und_AW",
    "nl_Latn_AW"
  }, {
    // { ?; ?; Aland Islands } => { Swedish; Latin; Aland Islands }
    "und_AX",
    "sv_Latn_AX"
  }, {
    // { ?; ?; Azerbaijan } => { Azerbaijani; Latin; Azerbaijan }
    "und_AZ",
    "az_Latn_AZ"
  }, {
    // { ?; Arabic; ? } => { Arabic; Arabic; Egypt }
    "und_Arab",
    "ar_Arab_EG"
  }, {
    // { ?; Arabic; India } => { Urdu; Arabic; India }
    "und_Arab_IN",
    "ur_Arab_IN"
  }, {
    // { ?; Arabic; Pakistan } => { Punjabi; Arabic; Pakistan }
    "und_Arab_PK",
    "pa_Arab_PK"
  }, {
    // { ?; Arabic; Senegal } => { Wolof; Arabic; Senegal }
    "und_Arab_SN",
    "wo_Arab_SN"
  }, {
    // { ?; Armenian; ? } => { Armenian; Armenian; Armenia }
    "und_Armn",
    "hy_Armn_AM"
  }, {
    // { ?; ?; Bosnia and Herzegovina }
    //  => { Bosnian; Latin; Bosnia and Herzegovina }
    "und_BA",
    "bs_Latn_BA"
  }, {
    // { ?; ?; Bangladesh } => { Bengali; Bengali; Bangladesh }
    "und_BD",
    "bn_Beng_BD"
  }, {
    // { ?; ?; Belgium } => { Dutch; Latin; Belgium }
    "und_BE",
    "nl_Latn_BE"
  }, {
    // { ?; ?; Burkina Faso } => { French; Latin; Burkina Faso }
    "und_BF",
    "fr_Latn_BF"
  }, {
    // { ?; ?; Bulgaria } => { Bulgarian; Cyrillic; Bulgaria }
    "und_BG",
    "bg_Cyrl_BG"
  }, {
    // { ?; ?; Bahrain } => { Arabic; Arabic; Bahrain }
    "und_BH",
    "ar_Arab_BH"
  }, {
    // { ?; ?; Burundi } => { Rundi; Latin; Burundi }
    "und_BI",
    "rn_Latn_BI"
  }, {
    // { ?; ?; Benin } => { French; Latin; Benin }
    "und_BJ",
    "fr_Latn_BJ"
  }, {
    // { ?; ?; Brunei } => { Malay; Latin; Brunei }
    "und_BN",
    "ms_Latn_BN"
  }, {
    // { ?; ?; Bolivia } => { Spanish; Latin; Bolivia }
    "und_BO",
    "es_Latn_BO"
  }, {
    // { ?; ?; Brazil } => { Portuguese; Latin; Brazil }
    "und_BR",
    "pt_Latn_BR"
  }, {
    // { ?; ?; Bhutan } => { Dzongkha; Tibetan; Bhutan }
    "und_BT",
    "dz_Tibt_BT"
  }, {
    // { ?; ?; Belarus } => { Belarusian; Cyrillic; Belarus }
    "und_BY",
    "be_Cyrl_BY"
  }, {
    // { ?; Bengali; ? } => { Bengali; Bengali; Bangladesh }
    "und_Beng",
    "bn_Beng_BD"
  }, {
    // { ?; Bengali; India } => { Assamese; Bengali; India }
    "und_Beng_IN",
    "as_Beng_IN"
  }, {
    // { ?; ?; Congo _ Kinshasa } => { French; Latin; Congo _ Kinshasa }
    "und_CD",
    "fr_Latn_CD"
  }, {
    // { ?; ?; Central African Republic }
    //  => { Sango; Latin; Central African Republic }
    "und_CF",
    "sg_Latn_CF"
  }, {
    // { ?; ?; Congo _ Brazzaville }
    //  => { Lingala; Latin; Congo _ Brazzaville }
    "und_CG",
    "ln_Latn_CG"
  }, {
    // { ?; ?; Switzerland } => { German; Latin; Switzerland }
    "und_CH",
    "de_Latn_CH"
  }, {
    // { ?; ?; Ivory Coast } => { French; Latin; Ivory Coast }
    "und_CI",
    "fr_Latn_CI"
  }, {
    // { ?; ?; Chile } => { Spanish; Latin; Chile }
    "und_CL",
    "es_Latn_CL"
  }, {
    // { ?; ?; Cameroon } => { French; Latin; Cameroon }
    "und_CM",
    "fr_Latn_CM"
  }, {
    // { ?; ?; China } => { Chinese; Simplified Han; China }
    "und_CN",
    "zh_Hans_CN"
  }, {
    // { ?; ?; Colombia } => { Spanish; Latin; Colombia }
    "und_CO",
    "es_Latn_CO"
  }, {
    // { ?; ?; Costa Rica } => { Spanish; Latin; Costa Rica }
    "und_CR",
    "es_Latn_CR"
  }, {
    // { ?; ?; Cuba } => { Spanish; Latin; Cuba }
    "und_CU",
    "es_Latn_CU"
  }, {
    // { ?; ?; Cape Verde } => { Portuguese; Latin; Cape Verde }
    "und_CV",
    "pt_Latn_CV"
  }, {
    // { ?; ?; Cyprus } => { Greek; Greek; Cyprus }
    "und_CY",
    "el_Grek_CY"
  }, {
    // { ?; ?; Czech Republic } => { Czech; Latin; Czech Republic }
    "und_CZ",
    "cs_Latn_CZ"
  }, {
    // { ?; Unified Canadian Aboriginal Syllabics; ? }
    //  => { Inuktitut; Unified Canadian Aboriginal Syllabics; Canada }
    "und_Cans",
    "iu_Cans_CA"
  }, {
    // { ?; Cyrillic; ? } => { Russian; Cyrillic; Russia }
    "und_Cyrl",
    "ru_Cyrl_RU"
  }, {
    // { ?; Cyrillic; Kazakhstan } => { Kazakh; Cyrillic; Kazakhstan }
    "und_Cyrl_KZ",
    "kk_Cyrl_KZ"
  }, {
    // { ?; ?; Germany } => { German; Latin; Germany }
    "und_DE",
    "de_Latn_DE"
  }, {
    // { ?; ?; Djibouti } => { Arabic; Arabic; Djibouti }
    "und_DJ",
    "ar_Arab_DJ"
  }, {
    // { ?; ?; Denmark } => { Danish; Latin; Denmark }
    "und_DK",
    "da_Latn_DK"
  }, {
    // { ?; ?; Dominican Republic } => {Spanish; Latin; Dominican Republic}
    "und_DO",
    "es_Latn_DO"
  }, {
    // { ?; ?; Algeria } => { Arabic; Arabic; Algeria }
    "und_DZ",
    "ar_Arab_DZ"
  }, {
    // { ?; Devanagari; ? } => { Hindi; Devanagari; India }
    "und_Deva",
    "hi_Deva_IN"
  }, {
    // { ?; ?; Ecuador } => { Spanish; Latin; Ecuador }
    "und_EC",
    "es_Latn_EC"
  }, {
    // { ?; ?; Estonia } => { Estonian; Latin; Estonia }
    "und_EE",
    "et_Latn_EE"
  }, {
    // { ?; ?; Egypt } => { Arabic; Arabic; Egypt }
    "und_EG",
    "ar_Arab_EG"
  }, {
    // { ?; ?; Western Sahara } => { Arabic; Arabic; Western Sahara }
    "und_EH",
    "ar_Arab_EH"
  }, {
    // { ?; ?; Eritrea } => { Tigrinya; Ethiopic; Eritrea }
    "und_ER",
    "ti_Ethi_ER"
  }, {
    // { ?; ?; Spain } => { Spanish; Latin; Spain }
    "und_ES",
    "es_Latn_ES"
  }, {
    // { ?; ?; Ethiopia } => { Amharic; Ethiopic; Ethiopia }
    "und_ET",
    "am_Ethi_ET"
  }, {
    // { ?; Ethiopic; ? } => { Amharic; Ethiopic; Ethiopia }
    "und_Ethi",
    "am_Ethi_ET"
  }, {
    // { ?; Ethiopic; Eritrea } => { Blin; Ethiopic; Eritrea }
    "und_Ethi_ER",
    "byn_Ethi_ER"
  }, {
    // { ?; ?; Finland } => { Finnish; Latin; Finland }
    "und_FI",
    "fi_Latn_FI"
  }, {
    // { ?; ?; Fiji } => { Fijian; Latin; Fiji }
    "und_FJ",
    "fj_Latn_FJ"
  }, {
    // { ?; ?; Micronesia } => { Chuukese; Latin; Micronesia }
    "und_FM",
    "chk_Latn_FM"
  }, {
    // { ?; ?; Faroe Islands } => { Faroese; Latin; Faroe Islands }
    "und_FO",
    "fo_Latn_FO"
  }, {
    // { ?; ?; France } => { French; Latin; France }
    "und_FR",
    "fr_Latn_FR"
  }, {
    // { ?; ?; Gabon } => { French; Latin; Gabon }
    "und_GA",
    "fr_Latn_GA"
  }, {
    // { ?; ?; Georgia } => { Georgian; Georgian; Georgia }
    "und_GE",
    "ka_Geor_GE"
  }, {
    // { ?; ?; French Guiana } => { French; Latin; French Guiana }
    "und_GF",
    "fr_Latn_GF"
  }, {
    // { ?; ?; Greenland } => { Kalaallisut; Latin; Greenland }
    "und_GL",
    "kl_Latn_GL"
  }, {
    // { ?; ?; Guinea } => { French; Latin; Guinea }
    "und_GN",
    "fr_Latn_GN"
  }, {
    // { ?; ?; Guadeloupe } => { French; Latin; Guadeloupe }
    "und_GP",
    "fr_Latn_GP"
  }, {
    // { ?; ?; Equatorial Guinea } => { French; Latin; Equatorial Guinea }
    "und_GQ",
    "fr_Latn_GQ"
  }, {
    // { ?; ?; Greece } => { Greek; Greek; Greece }
    "und_GR",
    "el_Grek_GR"
  }, {
    // { ?; ?; Guatemala } => { Spanish; Latin; Guatemala }
    "und_GT",
    "es_Latn_GT"
  }, {
    // { ?; ?; Guam } => { Chamorro; Latin; Guam }
    "und_GU",
    "ch_Latn_GU"
  }, {
    // { ?; ?; Guinea_Bissau } => { Portuguese; Latin; Guinea_Bissau }
    "und_GW",
    "pt_Latn_GW"
  }, {
    // { ?; Georgian; ? } => { Georgian; Georgian; Georgia }
    "und_Geor",
    "ka_Geor_GE"
  }, {
    // { ?; Greek; ? } => { Greek; Greek; Greece }
    "und_Grek",
    "el_Grek_GR"
  }, {
    // { ?; Gujarati; ? } => { Gujarati; Gujarati; India }
    "und_Gujr",
    "gu_Gujr_IN"
  }, {
    // { ?; Gurmukhi; ? } => { Punjabi; Gurmukhi; India }
    "und_Guru",
    "pa_Guru_IN"
  }, {
    // { ?; ?; Hong Kong SAR China }
    //  => { Chinese; Traditional Han; Hong Kong SAR China }
    "und_HK",
    "zh_Hant_HK"
  }, {
    // { ?; ?; Honduras } => { Spanish; Latin; Honduras }
    "und_HN",
    "es_Latn_HN"
  }, {
    // { ?; ?; Croatia } => { Croatian; Latin; Croatia }
    "und_HR",
    "hr_Latn_HR"
  }, {
    // { ?; ?; Haiti } => { Haitian; Latin; Haiti }
    "und_HT",
    "ht_Latn_HT"
  }, {
    // { ?; ?; Hungary } => { Hungarian; Latin; Hungary }
    "und_HU",
    "hu_Latn_HU"
  }, {
    // { ?; Han; ? } => { Chinese; Simplified Han; China }
    "und_Hani",
    "zh_Hans_CN"
  }, {
    // { ?; Simplified Han; ? } => { Chinese; Simplified Han; China }
    "und_Hans",
    "zh_Hans_CN"
  }, {
    // { ?; Traditional Han; ? }
    //  => { Chinese; Traditional Han; Hong Kong SAR China }
    "und_Hant",
    "zh_Hant_HK"
  }, {
    // { ?; Hebrew; ? } => { null; Hebrew; Israel }
    "und_Hebr",
    "iw_Hebr_IL"
  }, {
    // { ?; ?; Indonesia } => { Sundanese; Latin; Indonesia }
    "und_ID",
    "su_Latn_ID"
  }, {
    // { ?; ?; Israel } => { null; Hebrew; Israel }
    "und_IL",
    "iw_Hebr_IL"
  }, {
    // { ?; ?; India } => { Hindi; Devanagari; India }
    "und_IN",
    "hi_Deva_IN"
  }, {
    // { ?; ?; Iraq } => { Arabic; Arabic; Iraq }
    "und_IQ",
    "ar_Arab_IQ"
  }, {
    // { ?; ?; Iran } => { Persian; Arabic; Iran }
    "und_IR",
    "fa_Arab_IR"
  }, {
    // { ?; ?; Iceland } => { Icelandic; Latin; Iceland }
    "und_IS",
    "is_Latn_IS"
  }, {
    // { ?; ?; Italy } => { Italian; Latin; Italy }
    "und_IT",
    "it_Latn_IT"
  }, {
    // { ?; ?; Jordan } => { Arabic; Arabic; Jordan }
    "und_JO",
    "ar_Arab_JO"
  }, {
    // { ?; ?; Japan } => { Japanese; Japanese; Japan }
    "und_JP",
    "ja_Jpan_JP"
  }, {
    // { ?; Japanese; ? } => { Japanese; Japanese; Japan }
    "und_Jpan",
    "ja_Jpan_JP"
  }, {
    // { ?; ?; Kyrgyzstan } => { Kirghiz; Cyrillic; Kyrgyzstan }
    "und_KG",
    "ky_Cyrl_KG"
  }, {
    // { ?; ?; Cambodia } => { Khmer; Khmer; Cambodia }
    "und_KH",
    "km_Khmr_KH"
  }, {
    // { ?; ?; Comoros } => { Arabic; Arabic; Comoros }
    "und_KM",
    "ar_Arab_KM"
  }, {
    // { ?; ?; North Korea } => { Korean; Korean; North Korea }
    "und_KP",
    "ko_Kore_KP"
  }, {
    // { ?; ?; South Korea } => { Korean; Korean; South Korea }
    "und_KR",
    "ko_Kore_KR"
  }, {
    // { ?; ?; Kuwait } => { Arabic; Arabic; Kuwait }
    "und_KW",
    "ar_Arab_KW"
  }, {
    // { ?; ?; Kazakhstan } => { Russian; Cyrillic; Kazakhstan }
    "und_KZ",
    "ru_Cyrl_KZ"
  }, {
    // { ?; Khmer; ? } => { Khmer; Khmer; Cambodia }
    "und_Khmr",
    "km_Khmr_KH"
  }, {
    // { ?; Kannada; ? } => { Kannada; Kannada; India }
    "und_Knda",
    "kn_Knda_IN"
  }, {
    // { ?; Korean; ? } => { Korean; Korean; South Korea }
    "und_Kore",
    "ko_Kore_KR"
  }, {
    // { ?; ?; Laos } => { Lao; Lao; Laos }
    "und_LA",
    "lo_Laoo_LA"
  }, {
    // { ?; ?; Lebanon } => { Arabic; Arabic; Lebanon }
    "und_LB",
    "ar_Arab_LB"
  }, {
    // { ?; ?; Liechtenstein } => { German; Latin; Liechtenstein }
    "und_LI",
    "de_Latn_LI"
  }, {
    // { ?; ?; Sri Lanka } => { Sinhalese; Sinhala; Sri Lanka }
    "und_LK",
    "si_Sinh_LK"
  }, {
    // { ?; ?; Lesotho } => { Southern Sotho; Latin; Lesotho }
    "und_LS",
    "st_Latn_LS"
  }, {
    // { ?; ?; Lithuania } => { Lithuanian; Latin; Lithuania }
    "und_LT",
    "lt_Latn_LT"
  }, {
    // { ?; ?; Luxembourg } => { French; Latin; Luxembourg }
    "und_LU",
    "fr_Latn_LU"
  }, {
    // { ?; ?; Latvia } => { Latvian; Latin; Latvia }
    "und_LV",
    "lv_Latn_LV"
  }, {
    // { ?; ?; Libya } => { Arabic; Arabic; Libya }
    "und_LY",
    "ar_Arab_LY"
  }, {
    // { ?; Lao; ? } => { Lao; Lao; Laos }
    "und_Laoo",
    "lo_Laoo_LA"
  }, {
    // { ?; Latin; Spain } => { Catalan; Latin; Spain }
    "und_Latn_ES",
    "ca_Latn_ES"
  }, {
    // { ?; Latin; Ethiopia } => { Afar; Latin; Ethiopia }
    "und_Latn_ET",
    "aa_Latn_ET"
  }, {
    // { ?; Latin; United Kingdom } => { Welsh; Latin; United Kingdom }
    "und_Latn_GB",
    "cy_Latn_GB"
  }, {
    // { ?; Latin; Ghana } => { Akan; Latin; Ghana }
    "und_Latn_GH",
    "ak_Latn_GH"
  }, {
    // { ?; Latin; Indonesia } => { Indonesian; Latin; Indonesia }
    "und_Latn_ID",
    "id_Latn_ID"
  }, {
    // { ?; Latin; Italy } => { Friulian; Latin; Italy }
    "und_Latn_IT",
    "fur_Latn_IT"
  }, {
    // { ?; Latin; Nigeria } => { Atsam; Latin; Nigeria }
    "und_Latn_NG",
    "cch_Latn_NG"
  }, {
    // { ?; Latin; Turkey } => { Kurdish; Latin; Turkey }
    "und_Latn_TR",
    "ku_Latn_TR"
  }, {
    // { ?; Latin; South Africa } => { Afrikaans; Latin; South Africa }
    "und_Latn_ZA",
    "af_Latn_ZA"
  }, {
    // { ?; ?; Morocco } => { Arabic; Arabic; Morocco }
    "und_MA",
    "ar_Arab_MA"
  }, {
    // { ?; ?; Monaco } => { French; Latin; Monaco }
    "und_MC",
    "fr_Latn_MC"
  }, {
    // { ?; ?; Moldova } => { Romanian; Latin; Moldova }
    "und_MD",
    "ro_Latn_MD"
  }, {
    // { ?; ?; Montenegro } => { Serbian; Cyrillic; Montenegro }
    "und_ME",
    "sr_Cyrl_ME"
  }, {
    // { ?; ?; Madagascar } => { Malagasy; Latin; Madagascar }
    "und_MG",
    "mg_Latn_MG"
  }, {
    // { ?; ?; Marshall Islands } => {Marshallese; Latin; Marshall Islands}
    "und_MH",
    "mh_Latn_MH"
  }, {
    // { ?; ?; Macedonia } => { Macedonian; Cyrillic; Macedonia }
    "und_MK",
    "mk_Cyrl_MK"
  }, {
    // { ?; ?; Mali } => { French; Latin; Mali }
    "und_ML",
    "fr_Latn_ML"
  }, {
    // { ?; ?; Myanmar } => { Burmese; Myanmar; Myanmar }
    "und_MM",
    "my_Mymr_MM"
  }, {
    // { ?; ?; Mongolia } => { Mongolian; Cyrillic; Mongolia }
    "und_MN",
    "mn_Cyrl_MN"
  }, {
    // { ?; ?; Macao SAR China }
    //  => { Chinese; Traditional Han; Macao SAR China }
    "und_MO",
    "zh_Hant_MO"
  }, {
    // { ?; ?; Martinique } => { French; Latin; Martinique }
    "und_MQ",
    "fr_Latn_MQ"
  }, {
    // { ?; ?; Mauritania } => { Arabic; Arabic; Mauritania }
    "und_MR",
    "ar_Arab_MR"
  }, {
    // { ?; ?; Malta } => { Maltese; Latin; Malta }
    "und_MT",
    "mt_Latn_MT"
  }, {
    // { ?; ?; Maldives } => { Divehi; Thaana; Maldives }
    "und_MV",
    "dv_Thaa_MV"
  }, {
    // { ?; ?; Malawi } => { Nyanja; Latin; Malawi }
    "und_MW",
    "ny_Latn_MW"
  }, {
    // { ?; ?; Mexico } => { Spanish; Latin; Mexico }
    "und_MX",
    "es_Latn_MX"
  }, {
    // { ?; ?; Malaysia } => { Malay; Latin; Malaysia }
    "und_MY",
    "ms_Latn_MY"
  }, {
    // { ?; ?; Mozambique } => { Portuguese; Latin; Mozambique }
    "und_MZ",
    "pt_Latn_MZ"
  }, {
    // { ?; Malayalam; ? } => { Malayalam; Malayalam; India }
    "und_Mlym",
    "ml_Mlym_IN"
  }, {
    // { ?; Myanmar; ? } => { Burmese; Myanmar; Myanmar }
    "und_Mymr",
    "my_Mymr_MM"
  }, {
    // { ?; ?; New Caledonia } => { French; Latin; New Caledonia }
    "und_NC",
    "fr_Latn_NC"
  }, {
    // { ?; ?; Niger } => { French; Latin; Niger }
    "und_NE",
    "fr_Latn_NE"
  }, {
    // { ?; ?; Nigeria } => { Hausa; Latin; Nigeria }
    "und_NG",
    "ha_Latn_NG"
  }, {
    // { ?; ?; Nicaragua } => { Spanish; Latin; Nicaragua }
    "und_NI",
    "es_Latn_NI"
  }, {
    // { ?; ?; Netherlands } => { Dutch; Latin; Netherlands }
    "und_NL",
    "nl_Latn_NL"
  }, {
    // { ?; ?; Norway } => { Norwegian; Latin; Norway }
    "und_NO",
    "no_Latn_NO"
  }, {
    // { ?; ?; Nepal } => { Nepali; Devanagari; Nepal }
    "und_NP",
    "ne_Deva_NP"
  }, {
    // { ?; ?; Nauru } => { Nauru; Latin; Nauru }
    "und_NR",
    "na_Latn_NR"
  }, {
    // { ?; ?; Niue } => { Niuean; Latin; Niue }
    "und_NU",
    "niu_Latn_NU"
  }, {
    // { ?; ?; Oman } => { Arabic; Arabic; Oman }
    "und_OM",
    "ar_Arab_OM"
  }, {
    // { ?; Oriya; ? } => { Oriya; Oriya; India }
    "und_Orya",
    "or_Orya_IN"
  }, {
    // { ?; ?; Panama } => { Spanish; Latin; Panama }
    "und_PA",
    "es_Latn_PA"
  }, {
    // { ?; ?; Peru } => { Spanish; Latin; Peru }
    "und_PE",
    "es_Latn_PE"
  }, {
    // { ?; ?; French Polynesia } => { Tahitian; Latin; French Polynesia }
    "und_PF",
    "ty_Latn_PF"
  }, {
    // { ?; ?; Papua New Guinea } => { Tok Pisin; Latin; Papua New Guinea }
    "und_PG",
    "tpi_Latn_PG"
  }, {
    // { ?; ?; Philippines } => { Filipino; Latin; Philippines }
    "und_PH",
    "fil_Latn_PH"
  }, {
    // { ?; ?; Poland } => { Polish; Latin; Poland }
    "und_PL",
    "pl_Latn_PL"
  }, {
    // { ?; ?; Saint Pierre and Miquelon }
    //  => { French; Latin; Saint Pierre and Miquelon }
    "und_PM",
    "fr_Latn_PM"
  }, {
    // { ?; ?; Puerto Rico } => { Spanish; Latin; Puerto Rico }
    "und_PR",
    "es_Latn_PR"
  }, {
    // { ?; ?; Palestinian Territory }
    //  => { Arabic; Arabic; Palestinian Territory }
    "und_PS",
    "ar_Arab_PS"
  }, {
    // { ?; ?; Portugal } => { Portuguese; Latin; Portugal }
    "und_PT",
    "pt_Latn_PT"
  }, {
    // { ?; ?; Palau } => { Palauan; Latin; Palau }
    "und_PW",
    "pau_Latn_PW"
  }, {
    // { ?; ?; Paraguay } => { Guarani; Latin; Paraguay }
    "und_PY",
    "gn_Latn_PY"
  }, {
    // { ?; ?; Qatar } => { Arabic; Arabic; Qatar }
    "und_QA",
    "ar_Arab_QA"
  }, {
    // { ?; ?; Reunion } => { French; Latin; Reunion }
    "und_RE",
    "fr_Latn_RE"
  }, {
    // { ?; ?; Romania } => { Romanian; Latin; Romania }
    "und_RO",
    "ro_Latn_RO"
  }, {
    // { ?; ?; Serbia } => { Serbian; Cyrillic; Serbia }
    "und_RS",
    "sr_Cyrl_RS"
  }, {
    // { ?; ?; Russia } => { Russian; Cyrillic; Russia }
    "und_RU",
    "ru_Cyrl_RU"
  }, {
    // { ?; ?; Rwanda } => { Kinyarwanda; Latin; Rwanda }
    "und_RW",
    "rw_Latn_RW"
  }, {
    // { ?; ?; Saudi Arabia } => { Arabic; Arabic; Saudi Arabia }
    "und_SA",
    "ar_Arab_SA"
  }, {
    // { ?; ?; Sudan } => { Arabic; Arabic; Sudan }
    "und_SD",
    "ar_Arab_SD"
  }, {
    // { ?; ?; Sweden } => { Swedish; Latin; Sweden }
    "und_SE",
    "sv_Latn_SE"
  }, {
    // { ?; ?; Singapore } => { Chinese; Simplified Han; Singapore }
    "und_SG",
    "zh_Hans_SG"
  }, {
    // { ?; ?; Slovenia } => { Slovenian; Latin; Slovenia }
    "und_SI",
    "sl_Latn_SI"
  }, {
    // { ?; ?; Svalbard and Jan Mayen }
    //  => { Norwegian; Latin; Svalbard and Jan Mayen }
    "und_SJ",
    "no_Latn_SJ"
  }, {
    // { ?; ?; Slovakia } => { Slovak; Latin; Slovakia }
    "und_SK",
    "sk_Latn_SK"
  }, {
    // { ?; ?; San Marino } => { Italian; Latin; San Marino }
    "und_SM",
    "it_Latn_SM"
  }, {
    // { ?; ?; Senegal } => { French; Latin; Senegal }
    "und_SN",
    "fr_Latn_SN"
  }, {
    // { ?; ?; Somalia } => { Somali; Latin; Somalia }
    "und_SO",
    "so_Latn_SO"
  }, {
    // { ?; ?; Suriname } => { Dutch; Latin; Suriname }
    "und_SR",
    "nl_Latn_SR"
  }, {
    // { ?; ?; Sao Tome and Principe }
    //  => { Portuguese; Latin; Sao Tome and Principe }
    "und_ST",
    "pt_Latn_ST"
  }, {
    // { ?; ?; El Salvador } => { Spanish; Latin; El Salvador }
    "und_SV",
    "es_Latn_SV"
  }, {
    // { ?; ?; Syria } => { Arabic; Arabic; Syria }
    "und_SY",
    "ar_Arab_SY"
  }, {
    // { ?; Sinhala; ? } => { Sinhalese; Sinhala; Sri Lanka }
    "und_Sinh",
    "si_Sinh_LK"
  }, {
    // { ?; Syriac; ? } => { Syriac; Syriac; Syria }
    "und_Syrc",
    "syr_Syrc_SY"
  }, {
    // { ?; ?; Chad } => { Arabic; Arabic; Chad }
    "und_TD",
    "ar_Arab_TD"
  }, {
    // { ?; ?; Togo } => { French; Latin; Togo }
    "und_TG",
    "fr_Latn_TG"
  }, {
    // { ?; ?; Thailand } => { Thai; Thai; Thailand }
    "und_TH",
    "th_Thai_TH"
  }, {
    // { ?; ?; Tajikistan } => { Tajik; Cyrillic; Tajikistan }
    "und_TJ",
    "tg_Cyrl_TJ"
  }, {
    // { ?; ?; Tokelau } => { Tokelau; Latin; Tokelau }
    "und_TK",
    "tkl_Latn_TK"
  }, {
    // { ?; ?; East Timor } => { Tetum; Latin; East Timor }
    "und_TL",
    "tet_Latn_TL"
  }, {
    // { ?; ?; Turkmenistan } => { Turkmen; Latin; Turkmenistan }
    "und_TM",
    "tk_Latn_TM"
  }, {
    // { ?; ?; Tunisia } => { Arabic; Arabic; Tunisia }
    "und_TN",
    "ar_Arab_TN"
  }, {
    // { ?; ?; Tonga } => { Tonga; Latin; Tonga }
    "und_TO",
    "to_Latn_TO"
  }, {
    // { ?; ?; Turkey } => { Turkish; Latin; Turkey }
    "und_TR",
    "tr_Latn_TR"
  }, {
    // { ?; ?; Tuvalu } => { Tuvalu; Latin; Tuvalu }
    "und_TV",
    "tvl_Latn_TV"
  }, {
    // { ?; ?; Taiwan } => { Chinese; Traditional Han; Taiwan }
    "und_TW",
    "zh_Hant_TW"
  }, {
    // { ?; Tamil; ? } => { Tamil; Tamil; India }
    "und_Taml",
    "ta_Taml_IN"
  }, {
    // { ?; Telugu; ? } => { Telugu; Telugu; India }
    "und_Telu",
    "te_Telu_IN"
  }, {
    // { ?; Thaana; ? } => { Divehi; Thaana; Maldives }
    "und_Thaa",
    "dv_Thaa_MV"
  }, {
    // { ?; Thai; ? } => { Thai; Thai; Thailand }
    "und_Thai",
    "th_Thai_TH"
  }, {
    // { ?; Tibetan; ? } => { Tibetan; Tibetan; China }
    "und_Tibt",
    "bo_Tibt_CN"
  }, {
    // { ?; ?; Ukraine } => { Ukrainian; Cyrillic; Ukraine }
    "und_UA",
    "uk_Cyrl_UA"
  }, {
    // { ?; ?; Uruguay } => { Spanish; Latin; Uruguay }
    "und_UY",
    "es_Latn_UY"
  }, {
    // { ?; ?; Uzbekistan } => { Uzbek; Cyrillic; Uzbekistan }
    "und_UZ",
    "uz_Cyrl_UZ"
  }, {
    // { ?; ?; Vatican } => { Latin; Latin; Vatican }
    "und_VA",
    "la_Latn_VA"
  }, {
    // { ?; ?; Venezuela } => { Spanish; Latin; Venezuela }
    "und_VE",
    "es_Latn_VE"
  }, {
    // { ?; ?; Vietnam } => { Vietnamese; Latin; Vietnam }
    "und_VN",
    "vi_Latn_VN"
  }, {
    // { ?; ?; Vanuatu } => { French; Latin; Vanuatu }
    "und_VU",
    "fr_Latn_VU"
  }, {
    // { ?; ?; Wallis and Futuna } => { French; Latin; Wallis and Futuna }
    "und_WF",
    "fr_Latn_WF"
  }, {
    // { ?; ?; Samoa } => { Samoan; Latin; Samoa }
    "und_WS",
    "sm_Latn_WS"
  }, {
    // { ?; ?; Yemen } => { Arabic; Arabic; Yemen }
    "und_YE",
    "ar_Arab_YE"
  }, {
    // { ?; ?; Mayotte } => { French; Latin; Mayotte }
    "und_YT",
    "fr_Latn_YT"
  }, {
    // { ?; Yi; ? } => { Sichuan Yi; Yi; China }
    "und_Yiii",
    "ii_Yiii_CN"
  }, {
    // { Urdu; ?; ? } => { Urdu; Arabic; India }
    "ur",
    "ur_Arab_IN"
  }, {
    // { Uzbek; ?; ? } => { Uzbek; Cyrillic; Uzbekistan }
    "uz",
    "uz_Cyrl_UZ"
  }, {
    // { Uzbek; ?; Afghanistan } => { Uzbek; Arabic; Afghanistan }
    "uz_AF",
    "uz_Arab_AF"
  }, {
    // { Uzbek; Arabic; ? } => { Uzbek; Arabic; Afghanistan }
    "uz_Arab",
    "uz_Arab_AF"
  }, {
    // { Venda; ?; ? } => { Venda; Latin; South Africa }
    "ve",
    "ve_Latn_ZA"
  }, {
    // { Vietnamese; ?; ? } => { Vietnamese; Latin; Vietnam }
    "vi",
    "vi_Latn_VN"
  }, {
    // { Walamo; ?; ? } => { Walamo; Ethiopic; Ethiopia }
    "wal",
    "wal_Ethi_ET"
  }, {
    // { Wolof; ?; ? } => { Wolof; Arabic; Senegal }
    "wo",
    "wo_Arab_SN"
  }, {
    // { Wolof; ?; Senegal } => { Wolof; Latin; Senegal }
    "wo_SN",
    "wo_Latn_SN"
  }, {
    // { Xhosa; ?; ? } => { Xhosa; Latin; South Africa }
    "xh",
    "xh_Latn_ZA"
  }, {
    // { Yoruba; ?; ? } => { Yoruba; Latin; Nigeria }
    "yo",
    "yo_Latn_NG"
  }, {
    // { Chinese; ?; ? } => { Chinese; Simplified Han; China }
    "zh",
    "zh_Hans_CN"
  }, {
    // { Chinese; ?; Hong Kong SAR China }
    //  => { Chinese; Traditional Han; Hong Kong SAR China }
    "zh_HK",
    "zh_Hant_HK"
  }, {
    // { Chinese; Han; ? } => { Chinese; Simplified Han; China }
    "zh_Hani",
    "zh_Hans_CN"
  }, {
    // { Chinese; Traditional Han; ? }
    //  => { Chinese; Traditional Han; Taiwan }
    "zh_Hant",
    "zh_Hant_TW"
  }, {
    // { Chinese; ?; Macao SAR China }
    //  => { Chinese; Traditional Han; Macao SAR China }
    "zh_MO",
    "zh_Hant_MO"
  }, {
    // { Chinese; ?; Taiwan } => { Chinese; Traditional Han; Taiwan }
    "zh_TW",
    "zh_Hant_TW"
  }, {
    // { Zulu; ?; ? } => { Zulu; Latin; South Africa }
    "zu",
    "zu_Latn_ZA"
  }
};

/**
 * This trivial function searches the array for a matching entry.
 *
 * @param localeID The tag to find.
 * @return The corresponding StringPair if found, or a null pointer if not.
 */
static const StringPair*  U_CALLCONV
findStringPair(const char* localeID)
{
  StringPair dummy;
  const void* entry = NULL;

  dummy.minimal = localeID;
  dummy.maximal = "";

  entry = bsearch(
        &dummy,
        likely_subtags,
        sizeof(likely_subtags) / sizeof(likely_subtags[0]),
        sizeof(likely_subtags[0]),
        compareStringPairStructs);

  return (const StringPair*)entry;
}

/**
 * Append a tag to a buffer, adding the separator if necessary.
 *
 * @param tag The tag to add.
 * @param tagLength The length of the tag.
 * @param buffer The output buffer.
 * @param bufferLength The length of the output buffer.  This is an input/ouput parameter.
 **/
static void U_CALLCONV
appendTag(
    const char* tag,
    int32_t tagLength,
    char* buffer,
    int32_t* bufferLength)
{
    if (*bufferLength > 0)
    {
        buffer[*bufferLength] = '_';
        ++(*bufferLength);
    }
    uprv_memmove(
        &buffer[*bufferLength],
        tag,
        tagLength);
    *bufferLength += tagLength;
}

/**
 * Create a tag string from the supplied parameters.  The lang parameter must
 * not be a NULL pointer.  The script and region parameters may be NULL pointers.
 * If they are, their corresponding length parameters must be less than or equal
 * to 0.
 *
 * If either the script or region parameters are empty, and the alternateTags
 * parameter is not NULL, it will be parsed for potential script and region tags
 * to be used when constructing the new tag.
 *
 * If the length of the new string exceeds the capacity of the output buffer, 
 * the function copies as many bytes to the output buffer as it can, and returns
 * the error U_BUFFER_OVERFLOW_ERROR.
 *
 * If an illegal argument is provided, the function returns the error
 * U_ILLEGAL_ARGUMENT_ERROR.
 *
 * Note that this function can return the warning U_STRING_NOT_TERMINATED_WARNING if
 * the tag string fits in the output buffer, but the null byte doesn't.
 *
 * @param lang The language tag to use.
 * @param langLength The length of the language tag.
 * @param script The script tag to use.
 * @param scriptLength The length of the script tag.
 * @param region The region tag to use.
 * @param regionLength The length of the region tag.
 * @param trailing Any trailing data to append to the new tag.
 * @param trailingLength The length of the trailing data.
 * @param alternateTags A string containing any alternate tags.
 * @param tag The output buffer.
 * @param tagCapacity The capacity of the output buffer.
 * @param err A pointer to a UErrorCode for error reporting.
 * @return The length of the tag string, which may be greater than tagCapacity.
 **/
static int32_t U_CALLCONV
createTagStringWithAlternates(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* trailing,
    int32_t trailingLength,
    const char* alternateTags,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err)
{
    if (U_FAILURE(*err)) {
        goto error;
    }
    else if (tag == NULL ||
             tagCapacity <= 0 ||
             lang == NULL ||
             langLength <= 0 ||
             langLength >= ULOC_LANG_CAPACITY ||
             scriptLength >= ULOC_SCRIPT_CAPACITY ||
             regionLength >= ULOC_COUNTRY_CAPACITY)
    {
        goto error;
    }
    else
    {
        /**
         * ULOC_FULLNAME_CAPACITY will provide enough capacity
         * that we can build a string that contains the language,
         * script and region code without worrying about overrunning
         * the user-supplied buffer.
         **/
        char tagBuffer[ULOC_FULLNAME_CAPACITY];
        int32_t tagLength = 0;
        int32_t capacityRemaining = tagCapacity;

        appendTag(
            lang,
            langLength,
            tagBuffer,
            &tagLength);

        if (scriptLength > 0)
        {
            appendTag(
                script,
                scriptLength,
                tagBuffer,
                &tagLength);
        }
        else if (alternateTags != NULL)
        {
            char alternateScript[ULOC_SCRIPT_CAPACITY];

            const int32_t alternateScriptLength =
                uloc_getScript(
                    alternateTags,
                    alternateScript,
                    sizeof(alternateScript),
                    err);
            if (U_FAILURE(*err) ||
                alternateScriptLength >= ULOC_SCRIPT_CAPACITY) {
                goto error;
            }
            else if (alternateScriptLength > 0)
            {
                appendTag(
                    alternateScript,
                    alternateScriptLength,
                    tagBuffer,
                    &tagLength);
            }
        }

        if (regionLength > 0)
        {
            appendTag(
                region,
                regionLength,
                tagBuffer,
                &tagLength);
        }
        else if (alternateTags != NULL)
        {
            char alternateRegion[ULOC_COUNTRY_CAPACITY];

            const int32_t alternateRegionLength =
                uloc_getCountry(
                    alternateTags,
                    alternateRegion,
                    sizeof(alternateRegion),
                    err);
            if (U_FAILURE(*err) ||
                alternateRegionLength >= ULOC_COUNTRY_CAPACITY) {
                goto error;
            }
            else if (alternateRegionLength > 0)
            {
                appendTag(
                    alternateRegion,
                    alternateRegionLength,
                    tagBuffer,
                    &tagLength);
            }
        }

        {
            const int32_t toCopy =
                tagLength >= tagCapacity ? tagCapacity : tagLength;

            /**
             * Copy the partial tag from our internal buffer to the supplied
             * target.
             **/
            uprv_memcpy(
                tag,
                tagBuffer,
                toCopy);

            capacityRemaining -= toCopy;
        }

        if (trailingLength > 0 && capacityRemaining > 0)
        {
            /*
             * Copy the trailing data into the supplied buffer.  Use uprv_memmove, since we
             * don't know if the user-supplied buffers overlap.
             */
            const int32_t toCopy =
                trailingLength >= capacityRemaining ? capacityRemaining : trailingLength;

            uprv_memmove(
                &tag[tagLength],
                trailing,
                toCopy);
        }

        tagLength += trailingLength;

        return u_terminateChars(
                    tag,
                    tagCapacity,
                    tagLength,
                    err);
    }

error:

    /**
     * An overflow indicates the locale ID passed in
     * is ill-formed.  If we got here, and there was
     * no previous error, it's an implicit overflow.
     **/
    if (*err ==  U_BUFFER_OVERFLOW_ERROR ||
        U_SUCCESS(*err))
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

/**
 * Create a tag string from the supplied parameters.  The lang parameter must
 * not be a NULL pointer.  The script and region parameters may be NULL pointers.
 * If they are, their corresponding length parameters must be 0.
 *
 * @param lang The language tag to use.
 * @param langLength The length of the language tag.
 * @param script The script tag to use.
 * @param scriptLength The length of the script tag.
 * @param region The region tag to use.
 * @param regionLength The length of the region tag.
 * @param trailing Any trailing data to append to the new tag.
 * @param trailingLength The length of the trailing data.
 * @param tag The output buffer.
 * @param tagCapacity The capacity of the output buffer.
 * @param err A pointer to a UErrorCode for error reporting.
 * @return The length of the tag string, which may be greater than tagCapacity.
 **/
static int32_t U_CALLCONV
createTagString(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* trailing,
    int32_t trailingLength,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err)
{
    return createTagStringWithAlternates(
                lang,
                langLength,
                script,
                scriptLength,
                region,
                regionLength,
                trailing,
                trailingLength,
                NULL,
                tag,
                tagCapacity,
                err);
}

/**
 * These are the canonical strings for unknown scripts and regions.
 **/
static const char* unknownScript = "Zzzz";
static const char* unknownRegion = "ZZ";

/**
 * Parse the language, script, and region from a tag string.
 * not be a NULL pointer.  The script and region parameters may be NULL pointers.
 * If they are, their corresponding length parameters must be 0.
 *
 * If either the script or region parameters are empty, and the alternateTags
 * parameter is not NULL, it will be parsed for potential script and region tags
 * to be used when constructing the new tag.
 *
 * @param lang The language tag to use.
 * @param langLength The length of the language tag.
 * @param script The script tag to use.
 * @param scriptLength The length of the script tag.
 * @param region The region tag to use.
 * @param regionLength The length of the region tag.
 * @param trailing Any trailing data to append to the new tag.
 * @param trailingLength The length of the trailing data.
 * @param alternateTags A string containing any alternate tags.
 * @param tag The output buffer.
 * @param tagCapacity The capacity of the output buffer.
 * @param err A pointer to a UErrorCode for error reporting.
 **/
#if 0
static int32_t U_CALLCONV
parseTagString(
    const char* localeID,
    char* lang,
    int32_t* langLength,
    char* script,
    int32_t* scriptLength,
    char* region,
    int32_t* regionLength,
    UErrorCode* err)
{
    int32_t index = 0;

    /**
     * Since we allow callers to pass NULL pointers
     * in for parts of the tag they're not interested
     * in, these dummy variables help keep things simple.
     **/
    char dummyLang[ULOC_LANG_CAPACITY];
    char dummyScript[ULOC_SCRIPT_CAPACITY];
    char dummyRegion[ULOC_COUNTRY_CAPACITY];

    int32_t dummyLangLength = sizeof(dummyLang);
    int32_t dummyScriptLength = sizeof(dummyScript);
    int32_t dummyRegionLength = sizeof(dummyRegion);

    if(U_FAILURE(*err)) {
        return 0;
    }

    /**
     * We allow any pair of pointer and length parameters to be
     * NULL, but they must both be NULL or non-NULL.
     *
     * Note also that we are re-seating the formal parameters
     * to make things easier.
     **/
    if (lang != NULL)
    {
        if (langLength == NULL)
        {
            goto error;
        }
    }
    else if (langLength != NULL)
    {
        goto error;
    }
    else
    {
        lang = dummyLang;
        langLength = &dummyLangLength;
    }

    if (script != NULL)
    {
        if (scriptLength == NULL)
        {
            goto error;
        }
    }
    else if (scriptLength != NULL)
    {
        goto error;
    }
    else
    {
        script = dummyScript;
        scriptLength = &dummyScriptLength;
    }

    if (region != NULL)
    {
        if (regionLength == NULL)
        {
            goto error;
        }
    }
    else if (regionLength != NULL)
    {
        goto error;
    }
    else
    {
        region = dummyRegion;
        regionLength = &dummyRegionLength;
    }

    *langLength = uloc_getLanguage(localeID, lang, *langLength, err);

    if(U_FAILURE(*err)) {
        goto error;
    }

    index += *langLength;

    *scriptLength = uloc_getScript(localeID, script, *scriptLength, err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    if (*scriptLength > 0)
    {
        index += *scriptLength + 1;

        if (uprv_strnicmp(script, unknownScript, *scriptLength) == 0)
        {
            /**
             * If the script part is the "unknown" script, then don't return it.
             **/
            *scriptLength = 0;
        }
    }

    *regionLength = uloc_getCountry(localeID, region, *regionLength, err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    if (*regionLength > 0)
    {
        index += *regionLength + 1;

        if (uprv_strnicmp(region, unknownRegion, *regionLength) == 0)
        {
            /**
             * If the region part is the "unknown" region, then don't return it.
             **/
            *regionLength = 0;
        }
    }

    return index;

error:

    /*
     * An overflow indicates the locale ID passed in
     * is ill-formed.  If we get here, a no error
     * has already occurred, it's the result of an 
     * illegal argument.
     */
    if (*err ==  U_BUFFER_OVERFLOW_ERROR ||
        !U_FAILURE(*err))
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return 0;
}

#else
static int32_t U_CALLCONV
parseTagString(
    const char* localeID,
    char* lang,
    int32_t* langLength,
    char* script,
    int32_t* scriptLength,
    char* region,
    int32_t* regionLength,
    UErrorCode* err)
{
    int32_t index = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }

    *langLength = uloc_getLanguage(localeID, lang, *langLength, err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    index += *langLength;

    *scriptLength = uloc_getScript(localeID, script, *scriptLength, err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    if (*scriptLength > 0)
    {
        index += *scriptLength + 1;

        if (uprv_strnicmp(script, unknownScript, *scriptLength) == 0)
        {
            /**
             * If the script part is the "unknown" script, then don't return it.
             **/
            *scriptLength = 0;
        }
    }

    *regionLength = uloc_getCountry(localeID, region, *regionLength, err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    if (*regionLength > 0)
    {
        index += *regionLength + 1;

        if (uprv_strnicmp(region, unknownRegion, *regionLength) == 0)
        {
            /**
             * If the region part is the "unknown" region, then don't return it.
             **/
            *regionLength = 0;
        }
    }

    return index;

error:

    /**
     * An overflow indicates the locale ID passed in
     * is ill-formed.  If we get here, a no error
     * has already occurred, it's the result of an 
     * illegal argument.
     **/
    if (*err ==  U_BUFFER_OVERFLOW_ERROR ||
        !U_FAILURE(*err))
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}
#endif

static int32_t U_CALLCONV
createLikelySubtagsString(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* variants,
    int32_t variantsLength,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err)
{
    /**
     * ULOC_FULLNAME_CAPACITY will provide enough capacity
     * that we can build a string that contains the language,
     * script and region code without worrying about overrunning
     * the user-supplied buffer.
     **/
    char tagBuffer[ULOC_FULLNAME_CAPACITY];
    int32_t tagBufferLength = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }

    /**
     * Try the language with the script and region first.
     **/
    if (scriptLength > 0 && regionLength > 0)
    {
        const StringPair* likelySubtags = NULL;

        tagBufferLength = createTagString(
            lang,
            langLength,
            script,
            scriptLength,
            region,
            regionLength,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags = findStringPair(tagBuffer);

        if (likelySubtags != NULL)
        {
            char newLang[ULOC_LANG_CAPACITY];
            int32_t newLangLength = sizeof(newLang);

            newLangLength =
                uloc_getLanguage(
                    likelySubtags->maximal,
                    newLang,
                    newLangLength,
                    err);
            if(U_FAILURE(*err)) {
                goto error;
            }

            return createTagStringWithAlternates(
                        newLang,
                        newLangLength,
                        NULL,
                        0,
                        NULL,
                        0,
                        variants,
                        variantsLength,
                        likelySubtags->maximal,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    /**
     * Try the language with just the script.
     **/
    if (scriptLength > 0)
    {
        const StringPair* likelySubtags = NULL;

        tagBufferLength = createTagString(
            lang,
            langLength,
            script,
            scriptLength,
            NULL,
            0,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags = findStringPair(tagBuffer);

        if (likelySubtags != NULL)
        {
            char newLang[ULOC_LANG_CAPACITY];
            int32_t newLangLength = sizeof(newLang);

            newLangLength =
                uloc_getLanguage(
                    likelySubtags->maximal,
                    newLang,
                    newLangLength,
                    err);
            if(U_FAILURE(*err)) {
                goto error;
            }

            return createTagStringWithAlternates(
                        newLang,
                        newLangLength,
                        NULL,
                        0,
                        region,
                        regionLength,
                        variants,
                        variantsLength,
                        likelySubtags->maximal,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    /**
     * Try the language with just the region.
     **/
    if (regionLength > 0)
    {
        const StringPair* likelySubtags = NULL;

        createTagString(
            lang,
            langLength,
            NULL,
            0,
            region,
            regionLength,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags = findStringPair(tagBuffer);

        if (likelySubtags != NULL)
        {
            char newLang[ULOC_LANG_CAPACITY];
            int32_t newLangLength = sizeof(newLang);

            newLangLength =
                uloc_getLanguage(
                    likelySubtags->maximal,
                    newLang,
                    newLangLength,
                    err);
            if(U_FAILURE(*err)) {
                goto error;
            }

            return createTagStringWithAlternates(
                        newLang,
                        newLangLength,
                        script,
                        scriptLength,
                        NULL,
                        0,
                        variants,
                        variantsLength,
                        likelySubtags->maximal,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    /**
     * Finally, try just the language.
     **/
    {
        const StringPair* likelySubtags = NULL;

        createTagString(
            lang,
            langLength,
            NULL,
            0,
            NULL,
            0,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags = findStringPair(tagBuffer);

        if (likelySubtags != NULL)
        {
            char newLang[ULOC_LANG_CAPACITY];
            int32_t newLangLength = sizeof(newLang);

            newLangLength =
                uloc_getLanguage(
                    likelySubtags->maximal,
                    newLang,
                    newLangLength,
                    err);
            if(U_FAILURE(*err)) {
                goto error;
            }

            return createTagStringWithAlternates(
                        newLang,
                        newLangLength,
                        script,
                        scriptLength,
                        region,
                        regionLength,
                        variants,
                        variantsLength,
                        likelySubtags->maximal,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    return u_terminateChars(
                tag,
                tagCapacity,
                0,
                err);

error:

    if (!U_FAILURE(*err))
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

U_DRAFT int32_t U_EXPORT2
uloc_addLikelySubtags(const char*    localeID,
         char* maximizedLocaleID,
         int32_t maximizedLocaleIDCapacity,
         UErrorCode* err)
{
    char lang[ULOC_LANG_CAPACITY];
    int32_t langLength = sizeof(lang);
    char script[ULOC_SCRIPT_CAPACITY];
    int32_t scriptLength = sizeof(script);
    char region[ULOC_COUNTRY_CAPACITY];
    int32_t regionLength = sizeof(region);
    const char* variants = "";
    int32_t variantsLength = 0;
    int32_t variantsIndex = 0;
    int32_t resultLength = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }
    else if (localeID == NULL ||
             maximizedLocaleID == NULL ||
             maximizedLocaleIDCapacity <= 0)
    {
        goto error;
    }

    variantsIndex = parseTagString(
        localeID,
        lang,
        &langLength,
        script,
        &scriptLength,
        region,
        &regionLength,
        err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    /* Find the spot where the variants begin, if any. */
    variants = &localeID[variantsIndex];
    variantsLength = uprv_strlen(variants);

    resultLength =
        createLikelySubtagsString(
            lang,
            langLength,
            script,
            scriptLength,
            region,
            regionLength,
            variants,
            variantsLength,
            maximizedLocaleID,
            maximizedLocaleIDCapacity,
            err);

    if (resultLength == 0)
    {
        const int32_t localIDLength =
            uprv_strlen(localeID);

        /*
         * If we get here, we need to returned the localeID.
         */
        uprv_memcpy(
            maximizedLocaleID,
            localeID,
            localIDLength <= maximizedLocaleIDCapacity ? 
                localIDLength : maximizedLocaleIDCapacity);

        resultLength =
            u_terminateChars(
                maximizedLocaleID,
                maximizedLocaleIDCapacity,
                localIDLength,
                err);
    }

    return resultLength;

error:

    if (!U_FAILURE(*err))
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

U_DRAFT int32_t U_EXPORT2
uloc_minimizeSubtags(const char*    localeID,
         char* minimizedLocaleID,
         int32_t minimizedLocaleIDCapacity,
         UErrorCode* err)
{
    /**
     * ULOC_FULLNAME_CAPACITY will provide enough capacity
     * that we can build a string that contains the language,
     * script and region code without worrying about overrunning
     * the user-supplied buffer.
     **/
    char maximizedTagBuffer[ULOC_FULLNAME_CAPACITY];
    int32_t maximizedTagBufferLength = sizeof(maximizedTagBuffer);

    char lang[ULOC_LANG_CAPACITY];
    int32_t langLength = sizeof(lang);
    char script[ULOC_SCRIPT_CAPACITY];
    int32_t scriptLength = sizeof(script);
    char region[ULOC_COUNTRY_CAPACITY];
    int32_t regionLength = sizeof(region);
    const char* variants = "";
    int32_t variantsLength = 0;
    int32_t variantsIndex = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }
    else if (localeID == NULL ||
             minimizedLocaleID == NULL ||
             minimizedLocaleIDCapacity <= 0)
    {
        goto error;
    }

    variantsIndex =
        parseTagString(
            localeID,
            lang,
            &langLength,
            script,
            &scriptLength,
            region,
            &regionLength,
            err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    /* Find the spot where the variants begin, if any. */
    variants = &localeID[variantsIndex];
    variantsLength = uprv_strlen(variants);

    /**
     * First, we need to first get the maximization
     * from AddLikelySubtags.
     **/
    maximizedTagBufferLength =
        uloc_addLikelySubtags(
            localeID,
            maximizedTagBuffer,
            maximizedTagBufferLength,
            err);

    if(U_FAILURE(*err)) {
        goto error;
    }

    /**
     * Start first with just the language.
     **/
    {
        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                NULL,
                0,
                NULL,
                0,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0)
        {
            return createTagString(
                        lang,
                        langLength,
                        NULL,
                        0,
                        NULL,
                        0,
                        variants,
                        variantsLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    /**
     * Next, try the language and region.
     **/
    if (regionLength > 0)
    {
        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                NULL,
                0,
                region,
                regionLength,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0)
        {
            return createTagString(
                        lang,
                        langLength,
                        NULL,
                        0,
                        region,
                        regionLength,
                        variants,
                        variantsLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    /**
     * Finally, try the language and script.  This is our last chance,
     * since trying with all three components would only yield the
     * maximal version that we already have.
     **/
    if (scriptLength > 0 && regionLength > 0)
    {
        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                script,
                scriptLength,
                NULL,
                0,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0)
        {
            return createTagString(
                        lang,
                        langLength,
                        script,
                        scriptLength,
                        NULL,
                        0,
                        variants,
                        variantsLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    /**
     * If we got here, we need to returned the mazimized code.  It
     * will be the same as the value that we got in the localeID
     * parameter.
     **/
    uprv_memcpy(
        minimizedLocaleID,
        maximizedTagBuffer,
        maximizedTagBufferLength <= minimizedLocaleIDCapacity ? 
            maximizedTagBufferLength : minimizedLocaleIDCapacity);

    return u_terminateChars(
                minimizedLocaleID,
                minimizedLocaleIDCapacity,
                maximizedTagBufferLength,
                err);

error:

    if (!U_FAILURE(*err))
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

/*eof*/
