# *   Copyright (C) 1998-2008, International Business Machines
# *   Corporation and others.  All Rights Reserved.
RBNF_CLDR_VERSION = 1.6
# A list of txt's to build
# Note: 
#
#   If you are thinking of modifying this file, READ THIS. 
#
# Instead of changing this file [unless you want to check it back in],
# you should consider creating a 'rbnflocal.mk' file in this same directory.
# Then, you can have your local changes remain even if you upgrade or
# reconfigure ICU.
#
# Example 'rbnflocal.mk' files:
#
#  * To add an additional locale to the list: 
#    _____________________________________________________
#    |  RBNF_SOURCE_LOCAL =   myLocale.txt ...
#
#  * To REPLACE the default list and only build with a few
#     locale:
#    _____________________________________________________
#    |  RBNF_SOURCE = ar.txt ar_AE.txt en.txt de.txt zh.txt
#
#
# Generated by LDML2ICUConverter, from LDML source files. 

# Aliases which do not have a corresponding xx.xml file (see icu-config.xml & build.xml)
RBNF_SYNTHETIC_ALIAS =


# All aliases (to not be included under 'installed'), but not including root.
RBNF_ALIAS_SOURCE = $(RBNF_SYNTHETIC_ALIAS)


# Ordinary resources
RBNF_SOURCE = af.txt ca.txt cs.txt da.txt\
 de.txt el.txt en.txt eo.txt es.txt\
 et.txt fa.txt fa_AF.txt fi.txt fo.txt\
 fr.txt fr_BE.txt fr_CH.txt ga.txt he.txt\
 hu.txt hy.txt is.txt it.txt ja.txt\
 ka.txt kl.txt ko.txt lt.txt lv.txt\
 mt.txt nb.txt nl.txt nn.txt pl.txt\
 pt.txt pt_PT.txt ro.txt ru.txt sk.txt\
 sq.txt sv.txt th.txt tr.txt uk.txt\
 vi.txt zh.txt zh_Hant.txt
