#ifndef _H_ENC28J60_DEFS_H
#define _H_ENC28J60_DEFS_H

#define E28J_REVISION_ID              0x05

#define OPCODE_READ_CONTROL_REG       0
#define OPCODE_READ_BUFFER_MEMORY     1
#define OPCODE_WRITE_CONTROL_REG      2
#define OPCODE_WRITE_BUFFER_MEMORY    3
#define OPCODE_BIT_FIELD_SET          4
#define OPCODE_BIT_FIELD_CLEAR        5

// Registers
#define REG_ERDPTL                    0x00
#define REG_ERDPTH                    0x01
#define REG_EWRPTL                    0x02
#define REG_EWRPTH                    0x03
#define REG_ETXSTL                    0x04
#define REG_ETXSTH                    0x05
#define REG_ETXNDL                    0x06
#define REG_ETXNDH                    0x07
#define REG_ERXSTL                    0x08
#define REG_ERXSTH                    0x09
#define REG_ERXNDL                    0x0a
#define REG_ERXNDH                    0x0b
#define REG_ERXRDPTL                  0x0c
#define REG_ERXRDPTH                  0x0d
#define REG_ERXWRPTL                  0x0e
#define REG_ERXWRPTH                  0x0f
#define REG_EDMASTL                   0x10
#define REG_EDMASTH                   0x11
#define REG_EDMANDL                   0x12
#define REG_EDMANDH                   0x13
#define REG_EDMADSTL                  0x14
#define REG_EDMADSTH                  0x15
#define REG_EDMACSL                   0x16
#define REG_EDMACSH                   0x17
#define REG_EIE                       0x1b
#define REG_EIR                       0x1c
#define REG_ESTAT                     0x1d
#define REG_ECON2                     0x1e
#define REG_ECON1                     0x1f

#define REG_EHT0                      0x20
#define REG_EHT1                      0x21
#define REG_EHT2                      0x22
#define REG_EHT3                      0x23
#define REG_EHT4                      0x24
#define REG_EHT5                      0x25
#define REG_EHT6                      0x26
#define REG_EHT7                      0x27
#define REG_EPMM0                     0x28
#define REG_EPMM1                     0x29
#define REG_EPMM2                     0x2a
#define REG_EPMM3                     0x2b
#define REG_EPMM4                     0x2c
#define REG_EPMM5                     0x2d
#define REG_EPMM6                     0x2e
#define REG_EPMM7                     0x2f
#define REG_EPMCSL                    0x30
#define REG_EPMCSH                    0x31
#define REG_EPMOL                     0x34
#define REG_EPMOH                     0x35
#define REG_EWOLIE                    0x36
#define REG_EWOLIR                    0x37
#define REG_ERXFCON                   0x38
#define REG_EPKTCNT                   0x39

#define REG_MACON1                    0x40
#define REG_MACON2                    0x41
#define REG_MACON3                    0x42
#define REG_MACON4                    0x43
#define REG_MABBIPG                   0x44
#define REG_MAIPGL                    0x46
#define REG_MAIPGH                    0x47
#define REG_MACLCON1                  0x48
#define REG_MACLCON2                  0x49
#define REG_MAMXFLL                   0x4a
#define REG_MAMXFLH                   0x4b
#define REG_MAPHSUP                   0x4d
#define REG_MICON                     0x51
#define REG_MICMD                     0x52
#define REG_MIREGADR                  0x54
#define REG_MIWRL                     0x56
#define REG_MIWRH                     0x57
#define REG_MIRDL                     0x58
#define REG_MIRDH                     0x59

#define REG_MAADR1                    0x60
#define REG_MAADR0                    0x61
#define REG_MAADR3                    0x62
#define REG_MAADR2                    0x63
#define REG_MAADR5                    0x64
#define REG_MAADR4                    0x65
#define REG_EBSTSD                    0x66
#define REG_EBSTCON                   0x67
#define REG_EBSTCSL                   0x68
#define REG_EBSTCSH                   0x69
#define REG_MISTAT                    0x6a
#define REG_EREVID                    0x72
#define REG_ECOCON                    0x75
#define REG_EFLOCON                   0x77
#define REG_EPAUSL                    0x78
#define REG_EPAUSH                    0x79

// PHY registers
#define REG_PHCON1                    0x00
#define REG_PHSTAT1                   0x01
#define REG_PHID1                     0x02
#define REG_PHID2                     0x03
#define REG_PHCON2                    0x10
#define REG_PHSTAT2                   0x11
#define REG_PHIE                      0x12
#define REG_PHIR                      0x13
#define REG_PHLCON                    0x14

// Register bits

// EIE
#define B_INTIE                       7
#define B_PKTIE                       6
#define B_DMAIE                       5
#define B_LINKIE                      4
#define B_TXIE                        3
#define B_WOLIE                       2
#define B_TXERIE                      1
#define B_RXERIE                      0
// EIR
#define B_PKTIF                       6
#define B_DMAIF                       5
#define B_LINKIF                      4
#define B_TXIF                        3
#define B_WOLIF                       2
#define B_TXERIF                      1
#define B_RXERIF                      0
// ESTAT
#define B_INT                         7
#define B_LATECOL                     4
#define B_RXBUSY                      2
#define B_TXABRT                      1
#define B_CLKRDY                      0
// ECON2
#define B_AUTOINC                     7
#define B_PKTDEC                      6
#define B_PWRSV                       5
#define B_VRPS                        3
// ECON1
#define B_TXRST                       7
#define B_RXRST                       6
#define B_DMAST                       5
#define B_CSUMEN                      4
#define B_TXRTS                       3
#define B_RXEN                        2
#define B_BSEL1                       1
#define B_BSEL0                       0
// EWOLIE
#define B_UCWOLIE                     7
#define B_AWOLIE                      6
#define B_PMWOLIE                     4
#define B_MPWOLIE                     3
#define B_HTWOLIE                     2
#define B_MCWOLIE                     1
#define B_BCWOLIE                     0
// EWOLIR
#define B_UCWOLIF                     7
#define B_AWOLIF                      6
#define B_PMWOLIF                     4
#define B_MPWOLIF                     3
#define B_HTWOLIF                     2
#define B_MCWOLIF                     1
#define B_BCWOLIF                     0
// ERXFCON
#define B_UCEN                        7
#define B_ANDOR                       6
#define B_CRCEN                       5
#define B_PMEN                        4
#define B_MPEN                        3
#define B_HTEN                        2
#define B_MCEN                        1
#define B_BCEN                        0
// MACON1
#define B_LOOPBK                      4
#define B_TXPAUS                      3
#define B_RXPAUS                      2
#define B_PASSALL                     1
#define B_MARXEN                      0
// MACON2
#define B_MARST                       7
#define B_RNDRST                      6
#define B_MARXRST                     3
#define B_RFUNRST                     2
#define B_MATXRST                     1
#define B_TFUNRST                     0
// MACON3
#define B_PADCFG2                     7
#define B_PADCFG1                     6
#define B_PADCFG0                     5
#define B_TXCRCEN                     4
#define B_PHDRLEN                     3
#define B_HFRMEN                      2
#define B_FRMLNEN                     1
#define B_FULDPX                      0
// MACON4
#define B_DEFER                       6
#define B_BPEN                        5
#define B_NOBKOFF                     4
#define B_LONGPRE                     1
#define B_PUREPRE                     0
// MAPHSUP
#define B_RSTINTFC                    7
#define B_RSTRMII                     3
// MICON
#define B_RSTMII                      7
// MICMD
#define B_MIISCAN                     1
#define B_MIIRD                       0
// EBSTCON
#define B_PSV2                        7
#define B_PSV1                        6
#define B_PSV0                        5
#define B_PSEL                        4
#define B_TMSEL1                      3
#define B_TMSEL0                      2
#define B_TME                         1
#define B_BISTST                      0
// MISTAT
#define B_NVALID                      2
#define B_SCAN                        1
#define B_BUSY                        0
// ECOCON
#define B_COCON2                      2
#define B_COCON1                      1
#define B_COCON0                      0
// EFLOCON
#define B_FULDPXS                     2
#define B_FCEN1                       1
#define B_FCEN0                       0

// PHCON1
#define B_PRST                       15
#define B_PLOOPBK                    14
#define B_PPWRSV                     11
#define B_PDPXMD                      8
// PHSTAT1
#define B_PFDPX                      12
#define B_PHDPX                      11
#define B_LLSTAT                      2
#define B_JBSTAT                      1
// PHCON2
#define B_FRCLNK                     14
#define B_TXDIS                      13
#define B_JABBER                     10
#define B_HDLDIS                      8
// PHSTAT2
#define B_TXSTAT                     13
#define B_RXSTAT                     12
#define B_COLSTAT                    11
#define B_LSTAT                      10
#define B_DPXSTAT                     9
#define B_PLRITY                      4
// PHIE
#define B_PNLKIE                      4
#define B_PGEIE                       1
// PHIR
#define B_PLNKIF                      4
#define B_PGIF                        2
// PHLCON
#define B_LACFG0                      8
#define B_LBCFG0                      4
#define B_LFRQ1                       3
#define B_LFRQ0                       2
#define B_STRCH                       1

// Transmit override bits
#define B_PHUGEEN                     3
#define B_PPADEN                      2
#define B_PCRCEN                      1
#define B_POVERRIDE                   0

// Transmit status flags
#define B_TXSTAT_IS_VLAN                     51
#define B_TXSTAT_BACKPRESSURE_APPLIED        50
#define B_TXSTAT_IS_PAUSE_FRAME              49
#define B_TXSTAT_IS_CONTROL_FRAME            48
#define B_TXSTAT_UNDERRUN                    31
#define B_TXSTAT_IS_GIANT                    30
#define B_TXSTAT_LATE_COLLISION              29
#define B_TXSTAT_EXCESSIVE_COLLISION         28
#define B_TXSTAT_EXCESSIVE_DEFER             27
#define B_TXSTAT_DEFER                       26
#define B_TXSTAT_BROADCAST                   25
#define B_TXSTAT_MULTICAST                   24
#define B_TXSTAT_DONE                        23
#define B_TXSTAT_INVALID_LENGTH              22
#define B_TXSTAT_LENGTH_CHECK_ERROR          21
#define B_TXSTAT_CRC_ERROR                   20

// Receive status flags
#define B_RXSTAT_IS_VLAN                     30
#define B_RXSTAT_IS_UNSUPP_CONTROL           29
#define B_RXSTAT_IS_PAUSE                    28
#define B_RXSTAT_IS_CONTROL                  27
#define B_RXSTAT_DRIBBLE                     26
#define B_RXSTAT_BROADCAST                   25
#define B_RXSTAT_MULTICAST                   24
#define B_RXSTAT_OK                          23
#define B_RXSTAT_INVALID_LENGTH              22
#define B_RXSTAT_LENGTH_CHECK_ERROR          21
#define B_RXSTAT_CRC_ERROR                   20
#define B_RXSTAT_CARRIER_EVENT               18
#define B_RXSTAT_LONG_DROP_EVENT             16

static char const * const REG_NAMES[128] = {
    "ERDPTL", "ERDPTH", "EWRPTL", "EWRPTH", "ETXSTL", "ETXSTH", "ETXNDL", "ETXNDH",
    "ERXSTL", "ERXSTH", "ERXNDL", "ERXNDH", "ERXRDPTL", "ERXRDPTH", "ERXWRPTL", "ERXWRPTH",
    "EDMASTL", "EDMASTH", "EDMANDL", "EDMANDH", "EDMADSTL", "EDMADSTH", "EDMACSL", "EDMACSH",
    "(18H)", "(19H)", "(1AH)", "EIE", "EIR", "ESTAT", "ECON2", "ECON1",
    "EHT0", "EHT1", "EHT2", "EHT3", "EHT4", "EHT5", "EHT6", "EHT7",
    "EPMM0", "EPMM1", "EPMM2", "EPMM3", "EPMM4", "EPMM5", "EPMM6", "EPMM7",
    "EPMCSL", "EPMCSH", "(32H)", "(33H)", "EPMOL", "EPMOH", "EWOLIE", "EWOLIR",
    "ERXFCON", "EPKTCNT", "(3AH)", "EIE", "EIR", "ESTAT", "ECON2", "ECON1",
    "MACON1", "MACON2", "MACON3", "MACON4", "MABBIPG", "(45H)", "MAIPGL", "MAIPGH",
    "MACLCON1", "MACLCON2", "MAMXFLL", "MAMXFLH", "(4CH)", "MAPHSUP", "(4EH)", "(4FH)",
    "(50H)", "MICON", "MICMD", "(53H)", "MIREGADR", "(55H)", "MIWRL", "MIWRH",
    "MIRDL", "MIRDH", "(5AH)", "EIE", "EIR", "ESTAT", "ECON2", "ECON1",
    "MAADR1", "MAADR0", "MAADR3", "MAADR2", "MAADR5", "MAADR4", "EBSTSD", "EBSTCON",
    "EBSTCSL", "EBSTCSH", "MISTAT", "(6BH)", "(6CH)", "(6DH)", "(6EH)", "(6FH)",
    "(70H)", "(71H)", "EREVID", "(73H)", "(74H)", "ECOCON", "(76H)", "EFLOCON",
    "EPAUSL", "EPAUSH", "(7AH)", "EIE", "EIR", "ESTAT", "ECON2", "ECON1"
};

// Simulation events
#define SIM_EVENT_RECEIVE_FRAMES   0

static inline bool is_common_reg(uint8_t reg)
{
    return (reg & 0x1f) >= 0x1b;
}

static inline bool is_mac_reg(uint8_t reg)
{
    return
        ((reg >= REG_MACON1) && (reg <= REG_MAPHSUP)) ||
        ((reg >= REG_MAADR0) && (reg <= REG_MAADR4));
}

static inline bool is_mii_reg(uint8_t reg)
{
    return ((reg >= REG_MICON) && (reg <= REG_MIRDH)) || (reg == REG_MISTAT);
}

#endif
