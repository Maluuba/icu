/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 *
 *  FILE NAME: DOCMAIN.h
 *
 *   Date          Name        Description
 *   12/11/2000    Ram        Creation.
 */

/* This file contains documentation for Doxygen and doesnot have
 * any significance with respect to C or C++ API
 */

/*! \mainpage
 *
 * \section API API Reference Usage
 * 
 * <h3>C++ Programmers:</h3>
 * <p>Use  <a href="hierarchy.html">Class Hierarchy</a> or <a href="classes.html"> Alphabetical List </a>
 * or <a href="annotated.html"> Compound List</a>
 * to find the class you are interested in. For example, to find BreakIterator,
 * you can go to the <a href="classes.html"> Alphabetical List</a>, then click on
 * "BreakIterator". Once you are at the class, you will find an inheritance
 * chart, a list of the public members, a detailed description of the class,
 * then detailed member descriptions.</p>
 * 
 * <h3>C Programmers:</h3>
 * <p>Use <a href="#Module">Module List</a> or <a href="globals.html"> File Members</a>
 * to find a list of all the functions and constants.
 * For example, to find BreakIterator functions you would click on
 * <a href="files.html"> File List</a>,
 * then find "ubrk.h" and click on it. You will find descriptions of Defines,
 * Typedefs, Enumerations, and Functions, with detailed descriptions below.
 * If you want to find a specific function, such as ubrk_next(), then click
 * first on <a href="globals.html"> File Members</a>, then use your browser
 * Find dialog to search for "ubrk_next()".</p>
 *
 *
 * <h3>API References for Previous Releases</h3>
 * <p>The API References for the previous release are available
 *   <a href="http://oss.software.ibm.com/icu/apiref/previous/">online</a>.
 *   For each release there are also .zip and .tgz files for the API References
 *   on the <a href="http://oss.software.ibm.com/icu/download/">download page</a> of the release.</p>
 *
 * <hr>
 *
 * <h2>Architecture (User's Guide)</h2>
 * <ul>
 *   <li><a href="http://oss.software.ibm.com/icu/userguide/index.html">Introduction</a></li>
 *   <li><a href="http://oss.software.ibm.com/icu/userguide/i18n.html">Internationalization</a></li>
 *   <li><a href="http://oss.software.ibm.com/icu/userguide/design.html">Locale Model</a></li>
 *   <li><a href="http://oss.software.ibm.com/icu/userguide/design.html">Multithreading</a></li>
 *   <li><a href="http://oss.software.ibm.com/icu/userguide/conversion.html">Conversion</a></li>
 *   <li><a href="http://oss.software.ibm.com/icu/userguide/design.html">Error Handling</a></li>
 * </ul>
 *
 * <hr>
 *
 * <h2>Module List</h2>
 * <table border="1" cols="3" align="center">
 *   <tr>
 *     <td><strong>Module Name</strong></td>
 *     <td><strong>C</strong></td>
 *     <td><strong>C++</strong></td>
 *   </tr>
 *   <tr>
 *     <td>Basic Types and Constants</td>
 *     <td>utypes.h</td>
 *     <td>utypes.h</td>
 *   </tr>
 *   <tr>
 *     <td>Strings and Character Iteration</td>
 *     <td>ustring.h, utf.h</td>
 *     <td>UnicodeString, CharacterIterator</td>
 *   </tr>
 *   <tr>
 *     <td>Unicode Character<br>Properties and Names</td>
 *     <td>uchar.h</td>
 *     <td>uchar.h C API</td>
 *   </tr>
 *   <tr>
 *     <td>Codepage Conversion</td>
 *     <td>ucnv.h</td>
 *     <td>ucnv.h C API</td>
 *   </tr>
 *   <tr>
 *     <td>Unicode Text Compression</td>
 *     <td>ucnv.h <br> (encoding name "SCSU" or "BOCU-1")</td>
 *     <td>ucnv.h C API</td>
 *   </tr>
 *   <tr>
 *     <td>Locales </td>
 *     <td>uloc.h</a></td>
 *     <td>Locale</td>
 *   </tr>
 *   <tr>
 *     <td>Resource Bundles</td>
 *     <td>ures.h</td>
 *     <td>ResourceBundle</td>
 *   </tr>
 *   <tr>
 *     <td>Normalization</td>
 *     <td>unorm.h</td>
 *     <td>Normalizer</td>
 *   </tr>
 *   <tr>
 *     <td>Calendars</td>
 *     <td>ucal.h</td>
 *     <td>Calendar</td>
 *   </tr>
 *   <tr>
 *     <td>Date and Time Formatting</td>
 *     <td>udat.h</td>
 *     <td>DateFormat</td>
 *   </tr>
 *   <tr>
 *     <td>Message Formatting</td>
 *     <td>umsg.h</td>
 *     <td>MessageFormat</td>
 *   </tr>
 *   <tr>
 *     <td>Number Formatting</td>
 *     <td>unum.h</td>
 *     <td>NumberFormat</td>
 *   </tr>
 *   <tr>
 *     <td>Number Spellout <br> (Rule Based Number Formatting)</td>
 *     <td>unum.h <br> (use UNUM_SPELLOUT)</td>
 *     <td>RuleBasedNumberFormat</td>
 *   </tr>
 *   <tr>
 *     <td>Text Transformation <br> (Transliteration)</td>
 *     <td>utrans.h</td>
 *     <td>Transliterator</td>
 *   </tr>
 *   <tr>
 *     <td>Bidirectional Algorithm</td>
 *     <td>ubidi.h</td>
 *     <td>ubidi.h C API</td>
 *   </tr>
 *   <tr>
 *     <td>Arabic Shaping</td>
 *     <td>ushape.h</td>
 *     <td>ushape.h C API</td>
 *   </tr>
 *   <tr>
 *     <td>Collation</td>
 *     <td>ucol.h</td>
 *     <td>Collator</td>
 *   </tr>
 *   <tr>
 *     <td>String Searching</td>
 *     <td>usearch.h</td>
 *     <td>StringSearch</td>
 *   </tr>
 *   <tr>
 *     <td>Text Boundary Analysis <br> (Break Iteration)</td>
 *     <td>ubrk.h</td>
 *     <td>BreakIterator</td>
 *   </tr>
 *   <tr>
 *     <td>Unicode Set</td>
 *     <td>uset.h</td>
 *     <td>UnicodeSet</td>
 *   </tr>
 *   <tr>
 *     <td>Regular Expressions</td>
 *     <td>N/A</td>
 *     <td>RegexPattern, RegexMatcher</td>
 *   </tr>
 *   <tr>
 *     <td>StringPrep</td>
 *     <td>usprep.h</td>
 *     <td>usprep.h C API</td>
 *   </tr>
  *   <tr>
 *     <td>International Domain Names in Applications</td>
 *     <td>uidna.h</td>
 *     <td>uidna.h C API</td>
 *   </tr>
 * </table>
 */
