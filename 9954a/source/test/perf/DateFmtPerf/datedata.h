/*
***********************************************************************
* Copyright (C) 2016 and later: Unicode, Inc. and others.
* License & terms of use: http://www.unicode.org/copyright.html#License
***********************************************************************
***********************************************************************
* Copyright (c) 2002-2010,International Business Machines
* Corporation and others.  All Rights Reserved.
***********************************************************************
***********************************************************************
*/

int NUM_DATES = 250;

static int days[] = {
    21, 6, 6, 14, 2, 11, 18, 15, 10, 2, 19, 26, 3, 10, 5, 21, 5, 13, 18, 19, 13, 1, 14, 7, 8, 
    4, 1, 3, 27, 28, 9, 28, 16, 4, 26, 12, 17, 19, 9, 5, 17, 26, 12, 24, 6, 26, 8, 10, 6, 22, 
    18, 8, 19, 14, 28, 6, 2, 17, 11, 3, 14, 3, 6, 19, 5, 4, 18, 18, 15, 10, 26, 13, 6, 13, 16, 
    23, 1, 26, 25, 26, 27, 25, 16, 12, 21, 8, 5, 28, 21, 6, 17, 26, 2, 17, 24, 8, 15, 9, 4, 24, 
    19, 1, 25, 23, 7, 3, 1, 19, 28, 11, 5, 1, 13, 25, 7, 25, 12, 13, 15, 11, 5, 21, 24, 12, 27, 
    18, 23, 14, 8, 3, 8, 9, 22, 20, 10, 27, 11, 23, 10, 19, 9, 5, 28, 19, 6, 27, 28, 19, 12, 15, 
    24, 2, 22, 23, 19, 16, 3, 18, 15, 9, 19, 23, 8, 6, 11, 14, 28, 10, 2, 13, 20, 4, 15, 24, 17, 
    28, 20, 13, 9, 15, 24, 27, 28, 13, 20, 14, 22, 7, 15, 21, 15, 25, 12, 12, 6, 26, 14, 21, 20, 26, 
    16, 5, 9, 22, 4, 20, 21, 20, 9, 26, 4, 1, 4, 10, 21, 12, 19, 21, 25, 20, 5, 7, 24, 15, 27, 
    18, 5, 27, 9, 4, 18, 9, 25, 8, 20, 12, 16, 4, 8, 20, 26, 3, 24, 21, 5, 18, 13, 20, 15, 21
};


static int months[] = {
    0, 3, 8, 6, 5, 8, 7, 1, 8, 6, 0, 1, 5, 1, 9, 5, 4, 2, 3, 5, 5, 9, 11, 10, 1, 
    1, 1, 11, 5, 6, 7, 8, 0, 10, 9, 10, 0, 0, 10, 11, 10, 9, 0, 4, 7, 4, 4, 1, 8, 1, 
    7, 11, 2, 11, 0, 10, 11, 0, 8, 3, 1, 10, 2, 9, 0, 7, 8, 8, 0, 1, 11, 9, 5, 0, 1, 
    6, 2, 10, 6, 11, 9, 2, 6, 3, 1, 4, 2, 9, 0, 8, 10, 4, 0, 2, 5, 2, 8, 3, 5, 2, 
    0, 7, 10, 2, 5, 11, 2, 0, 7, 5, 7, 3, 2, 11, 9, 11, 4, 8, 3, 7, 0, 11, 5, 0, 0, 
    6, 1, 3, 5, 2, 6, 10, 1, 2, 11, 11, 6, 6, 9, 6, 2, 3, 7, 6, 8, 0, 7, 8, 9, 7, 
    9, 1, 4, 10, 1, 2, 7, 10, 10, 5, 6, 11, 6, 9, 7, 6, 10, 6, 1, 0, 6, 8, 3, 6, 10, 
    4, 9, 10, 4, 7, 0, 6, 11, 10, 3, 4, 9, 1, 10, 10, 0, 3, 4, 3, 3, 11, 10, 4, 7, 11, 
    9, 6, 2, 2, 5, 4, 9, 0, 8, 9, 5, 2, 3, 11, 0, 6, 9, 11, 6, 2, 5, 7, 8, 5, 5, 
    6, 1, 4, 5, 11, 10, 8, 3, 11, 8, 4, 6, 8, 2, 10, 9, 5, 6, 6, 4, 0, 2, 2, 6, 0
};


static int years[] = {
    1857, 1824, 1626, 1794, 1782, 1842, 1721, 1657, 1899, 1866, 1827, 1840, 1982, 1681, 1683, 1698, 1795, 1896, 1690, 1961, 1628, 1821, 1790, 1863, 1812, 
    1928, 1634, 1874, 1910, 1800, 1745, 1884, 1627, 1717, 1956, 1694, 1807, 1874, 1955, 1777, 1925, 1937, 1640, 1897, 1825, 1938, 1727, 1743, 1827, 1834, 
    1626, 1641, 2004, 1884, 1984, 1768, 1864, 1622, 1845, 1645, 1876, 1723, 1877, 1720, 1641, 2008, 1850, 1727, 1742, 1957, 1902, 1624, 1692, 1931, 1962, 
    1652, 1986, 1724, 1692, 1735, 1897, 1651, 1911, 1976, 1714, 1673, 1972, 1825, 1724, 1745, 1863, 1763, 1793, 1777, 1772, 1834, 1680, 1997, 1632, 1896, 
    1829, 1874, 1645, 2001, 1892, 1995, 1725, 1939, 1973, 1651, 1951, 1711, 1946, 1961, 2006, 1896, 1660, 1769, 1903, 1735, 1938, 1628, 1899, 1977, 1635, 
    1902, 1787, 1734, 1747, 1752, 1850, 1998, 1658, 1754, 1636, 1873, 1730, 1991, 1860, 1835, 1775, 1948, 1610, 1899, 1689, 1742, 1836, 1839, 1748, 1999, 
    1779, 1982, 1809, 1965, 2007, 1633, 1885, 1991, 1685, 1983, 1831, 1933, 1804, 1827, 1903, 1859, 1767, 1916, 1978, 1697, 1732, 1713, 1618, 1770, 1651, 
    1935, 1994, 1799, 1803, 1680, 1760, 1616, 1894, 1881, 1899, 1862, 1789, 1665, 1827, 1792, 1886, 1689, 1791, 1968, 1961, 1997, 1850, 1616, 1866, 1637, 
    1951, 1695, 1617, 1976, 1924, 1634, 1726, 1678, 1843, 1689, 1751, 1759, 1794, 1631, 1996, 1774, 1828, 1629, 1953, 1756, 1673, 1964, 1898, 1802, 2000, 
    1956, 1718, 1774, 1997, 1894, 1634, 1953, 1622, 1779, 1743, 1636, 1735, 1735, 1837, 1740, 1781, 1702, 1830, 1905, 1920, 1857, 1767, 1726, 1873, 1620
};


