# GENERATED by gencmake.py
# 
# Copyright (C) 2011-2014, International Business Machines Corporation and others.
# All Rights Reserved.
# 
# (Obligatory) don't edit, just re-run gencmake.py

set(ICU_i18n
    # collation_builder
    collationfastlatinbuilder.cpp
    collationbasedatabuilder.cpp
    collationweights.cpp
    collationruleparser.cpp
    collationbuilder.cpp
    collationdatabuilder.cpp
    # collation
    utf8collationiterator.cpp
    bocsu.cpp
    collationcompare.cpp
    collationfastlatin.cpp
    collationsets.cpp
    collationtailoring.cpp
    ucoleitr.cpp
    collationrootelements.cpp
    coleitr.cpp
    collationsettings.cpp
    ucol_sit.cpp
    ucol.cpp
    collationkeys.cpp
    coll.cpp
    collation.cpp
    collationiterator.cpp
    uitercollationiterator.cpp
    collationdata.cpp
    utf16collationiterator.cpp
    rulebasedcollator.cpp
    collationfcd.cpp
    sortkey.cpp
    collationdatawriter.cpp
    ucol_res.cpp
    collationroot.cpp
    collationdatareader.cpp
    # uclean_i18n
    ucln_in.cpp
    # regex
    regexcmp.cpp
    regexst.cpp
    repattrn.cpp
    rematch.cpp
    uregex.cpp
    regextxt.cpp
    regeximp.cpp
    # formattable_cnv
    fmtable_cnv.cpp
    # formattable
    measure.cpp
    fmtable.cpp
    # digitlist
    digitlst.cpp
    decContext.c
    decNumber.c
    # spoof_detection
    uspoof_impl.cpp
    scriptset.cpp
    uspoof_wsconf.cpp
    uspoof_build.cpp
    identifier_info.cpp
    uspoof.cpp
    uspoof_conf.cpp
    # genderinfo
    gender.cpp
    # charset_detector
    csrecog.cpp
    csmatch.cpp
    csrutf8.cpp
    inputext.cpp
    csdetect.cpp
    csrsbcs.cpp
    csrmbcs.cpp
    csr2022.cpp
    ucsdet.cpp
    csrucode.cpp
    # region
    uregion.cpp
    region.cpp
    # formatting
    decimfmt.cpp
    decimalformatpattern.cpp
    reldatefmt.cpp
    tztrans.cpp
    indiancal.cpp
    plurfmt.cpp
    measunit.cpp
    curramt.cpp
    tznames.cpp
    unum.cpp
    dtitvfmt.cpp
    currfmt.cpp
    smpdtfmt.cpp
    nfrule.cpp
    nfsubs.cpp
    compactdecimalformat.cpp
    tmutfmt.cpp
    wintzimpl.cpp
    basictz.cpp
    datefmt.cpp
    nfrs.cpp
    udateintervalformat.cpp
    currunit.cpp
    unumsys.cpp
    decfmtst.cpp
    winnmfmt.cpp
    ucurr.cpp
    chnsecal.cpp
    udat.cpp
    timezone.cpp
    taiwncal.cpp
    tzfmt.cpp
    zonemeta.cpp
    udatpg.cpp
    smpdtfst.cpp
    islamcal.cpp
    currpinf.cpp
    gregoimp.cpp
    dangical.cpp
    dtfmtsym.cpp
    simpletz.cpp
    tznames_impl.cpp
    cecal.cpp
    dtrule.cpp
    choicfmt.cpp
    astro.cpp
    windtfmt.cpp
    vtzone.cpp
    locdspnm.cpp
    japancal.cpp
    selfmt.cpp
    quantityformatter.cpp
    scientificformathelper.cpp
    numfmt.cpp
    ethpccal.cpp
    tmunit.cpp
    tzrule.cpp
    dtptngen.cpp
    calendar.cpp
    hebrwcal.cpp
    buddhcal.cpp
    gregocal.cpp
    numsys.cpp
    measfmt.cpp
    ucal.cpp
    vzone.cpp
    rbnf.cpp
    rbtz.cpp
    tzgnames.cpp
    ztrans.cpp
    dcfmtsym.cpp
    zrule.cpp
    persncal.cpp
    tmutamt.cpp
    olsontz.cpp
    dtitvinf.cpp
    coptccal.cpp
    umsg.cpp
    msgfmt.cpp
    reldtfmt.cpp
    # pluralrules
    plurrule.cpp
    upluralrules.cpp
    # sharedbreakiterator
    sharedbreakiterator.cpp
    # format
    format.cpp
    fphdlimp.cpp
    fpositer.cpp
    # filteredbreakiterator
    filteredbrk.cpp
    # string_search
    stsearch.cpp
    search.cpp
    usearch.cpp
    # localedata
    ulocdata.c
    # regex_cnv
    uregexc.cpp
    # alphabetic_index
    alphaindex.cpp
    # translit
    quant.cpp
    utrans.cpp
    cpdtrans.cpp
    casetrn.cpp
    nultrans.cpp
    rbt_data.cpp
    toupptrn.cpp
    nortrans.cpp
    esctrn.cpp
    remtrans.cpp
    translit.cpp
    tridpars.cpp
    strrepl.cpp
    rbt_rule.cpp
    transreg.cpp
    name2uni.cpp
    anytrans.cpp
    rbt_pars.cpp
    strmatch.cpp
    unesctrn.cpp
    rbt_set.cpp
    titletrn.cpp
    rbt.cpp
    funcrepl.cpp
    brktrans.cpp
    tolowtrn.cpp
    uni2name.cpp
    # universal_time_scale
    utmscale.c
    # LIBRARY DEPENDENCIES: set([None, 'common'])
    )

set(ICU_stubdata
    # LIBRARY DEPENDENCIES: set([])
    )

set(ICU_common
    # icudataver
    icudataver.c
    # resourcebundle
    uloc_keytype.cpp
    uresbund.cpp
    uloc_tag.c
    wintz.c
    uresdata.c
    resbund.cpp
    locbased.cpp
    locavailable.cpp
    locid.cpp
    uloc.cpp
    locmap.c
    # sort
    uarrsort.c
    # platform
    uinvchar.c
    ustrfmt.c
    cwchar.c
    putil.cpp
    cstring.c
    charstr.cpp
    udataswp.c
    utrace.c
    appendable.cpp
    umath.c
    uobject.cpp
    stringpiece.cpp
    umutex.cpp
    ustrtrns.cpp
    cmemory.c
    sharedobject.cpp
    unistr.cpp
    ustring.cpp
    utf_impl.c
    ucln_cmn.cpp
    # uvector
    uvector.cpp
    # udata
    udata.cpp
    ucmndata.c
    umapfile.c
    udatamem.c
    # uhash
    uhash.c
    # stringenumeration
    uenum.c
    ustrenum.cpp
    # ucol_swp
    ucol_swp.cpp
    # utrie2
    utrie2.cpp
    # simplepatternformatter
    simplepatternformatter.cpp
    # filterednormalizer2
    filterednormalizer2.cpp
    # normalizer2
    normalizer2impl.cpp
    normalizer2.cpp
    # utrie2_builder
    utrie2_builder.cpp
    # utrie
    utrie.cpp
    # uniset_core
    unifilt.cpp
    unisetspan.cpp
    uniset.cpp
    unifunct.cpp
    bmpset.cpp
    # icu_utility
    util.cpp
    # patternprops
    patternprops.cpp
    # uset_props
    uset_props.cpp
    # uniset_props
    ruleiter.cpp
    uniset_props.cpp
    # uprops
    uprops.cpp
    # ubidi_props
    ubidi_props.c
    # ucase
    ucase.cpp
    # uchar
    uchar.c
    # ustring_case
    ustrcase.cpp
    # loadednormalizer2
    loadednormalizer2impl.cpp
    # unistr_case
    unistr_case.cpp
    # unames
    unames.cpp
    # propname
    propname.cpp
    # bytestrie
    bytestrie.cpp
    # parsepos
    parsepos.cpp
    # uniset_closure
    uniset_closure.cpp
    # unistr_titlecase_brkiter
    unistr_titlecase_brkiter.cpp
    # ustr_titlecase_brkiter
    ustr_titlecase_brkiter.cpp
    # ustring_case_locale
    ustrcase_locale.cpp
    # breakiterator
    rbbirb.cpp
    rbbitblb.cpp
    brkiter.cpp
    brkeng.cpp
    rbbinode.cpp
    rbbisetb.cpp
    dictionarydata.cpp
    rbbi.cpp
    rbbistbl.cpp
    rbbiscan.cpp
    dictbe.cpp
    ubrk.cpp
    rbbidata.cpp
    # schriter
    uchriter.cpp
    schriter.cpp
    # chariter
    chariter.cpp
    # uvector32
    uvectr32.cpp
    # normlzr
    normlzr.cpp
    # ucharstrie
    ucharstrie.cpp
    # utext
    utext.cpp
    # service_registration
    servnotf.cpp
    servslkf.cpp
    servlkf.cpp
    locutil.cpp
    servrbf.cpp
    serv.cpp
    servlk.cpp
    servls.cpp
    # locale_display_names
    locdispnames.cpp
    # locresdata
    locresdata.cpp
    # hashtable
    uhash_us.cpp
    # ustack
    ustack.cpp
    # unistr_case_locale
    unistr_case_locale.cpp
    # uvector64
    uvectr64.cpp
    # ucnv_set
    ucnv_set.c
    # uset
    uset.cpp
    # ures_cnv
    ures_cnv.c
    # conversion
    ucnv_ct.c
    ucnv2022.cpp
    ucnvmbcs.cpp
    ucnv_ext.cpp
    ucnv_u8.c
    ucnv_cnv.c
    ucnvlat1.c
    ucnv_lmb.c
    ucnv_u32.c
    ucnvbocu.cpp
    ucnvscsu.c
    ustr_cnv.cpp
    ucnvisci.c
    ucnv_bld.cpp
    ucnv.c
    ucnv_u16.c
    ucnv_u7.c
    ucnvhz.c
    ucnv_err.c
    ucnv_cb.c
    # ucnv_io
    ucnv_io.cpp
    # unifiedcache
    unifiedcache.cpp
    # bytestriebuilder
    bytestriebuilder.cpp
    # stringtriebuilder
    stringtriebuilder.cpp
    # ucat
    ucat.c
    # messagepattern
    messagepattern.cpp
    # uts46
    uts46.cpp
    # punycode
    punycode.cpp
    # bytestream
    bytestream.cpp
    # ubidi
    ubidiln.c
    ubidiwrt.c
    ubidi.c
    # ucasemap_titlecase_brkiter
    ucasemap_titlecase_brkiter.cpp
    # ucasemap
    ucasemap.cpp
    # ucnvdisp
    ucnvdisp.c
    # resbund_cnv
    resbund_cnv.cpp
    # ucharstrieiterator
    ucharstrieiterator.cpp
    # bytestrieiterator
    bytestrieiterator.cpp
    # unistr_cnv
    unistr_cnv.cpp
    # usetiter
    usetiter.cpp
    # listformatter
    listformatter.cpp
    # canonical_iterator
    caniter.cpp
    # errorcode
    errorcode.cpp
    # utypes
    utypes.c
    # loclikely
    loclikely.cpp
    # uscript_props
    uscript_props.cpp
    # uinit
    uinit.cpp
    # icuplug
    icuplug.cpp
    # date_interval
    dtintrv.cpp
    # uiter
    uiter.cpp
    # converter_selector
    ucnvsel.cpp
    # propsvec
    propsvec.c
    # icu_utility_with_props
    util_props.cpp
    # ulist
    ulist.c
    # unistr_props
    unistr_props.cpp
    # unormcmp
    unormcmp.cpp
    # ustr_wcs
    ustr_wcs.cpp
    # idna2003
    uidna.cpp
    # stringprep
    usprep.cpp
    # unorm
    unorm.cpp
    # script_runs
    usc_impl.c
    # ucharstriebuilder
    ucharstriebuilder.cpp
    # ushape
    ushape.cpp
    # uscript
    uscript.c
    # LIBRARY DEPENDENCIES: set([None])
    )

set(ICU_io
    # ustream
    ustream.cpp
    # uclean_io
    ucln_io.cpp
    # ustdio
    uprintf.cpp
    uscanf.c
    ustdio.c
    uscanf_p.c
    uprntf_p.c
    sscanf.c
    ufile.c
    sprintf.c
    locbund.cpp
    ufmt_cmn.c
    # LIBRARY DEPENDENCIES: set(['i18n', None, 'common'])
    )

