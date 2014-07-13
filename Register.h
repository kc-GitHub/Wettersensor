#include <AskSinMain.h>

const uint8_t devParam[] PROGMEM = {
	0x10,                                    // The firmware version, 1 byte
	0xF1, 0x01,                              // The model-ID    0xF1 0x01 = DIY (HB-UW-Sen-THPL-I)
//	0xF1, 0x02,                              // The model-ID    0xF1 0x01 = DIY (HB-UW-Sen-THPL-O)
	'H','B','0','D','e','f','a','u','l','t', // The serial 10 bytes, needed for pairing   (Default for flash tool)
	0x70,                                    // Frame type 0x70 = WEATHER_EVENT
	0x03, 0x01, 0x00,                        // Device Info, 3 byte, describes device, not completely clear yet. includes amount of channels

	0xAB, 0xCD, 0xEF                         // The HM-ID 3 bytes, needed for pairing     (Default for flash tool)
};

HM::s_devParm dParm = {
	5,                                       // send retries, 1 byte, how often a string should be send out until we get an answer
	700,                                     // send timeout, 2 byte, time out for ACK handling
	devParam                                 // pointer to devParam, see above
};

HM::s_modtable modTbl[] = {
	{0,0,(s_mod_dlgt)NULL},
	{0,0,(s_mod_dlgt)NULL},
}; // 16 byte

// channel slice definition, 6 bytes
uint8_t sliceStr[] = {
	0x01, 0x02, 0x0a, 0x0b, 0x0c, 0x01,
};

// Channel device config
struct s_regDevL0 {
	// 0x01,0x02,0x0a,0x0b,0x0c,
	uint8_t  burstRx;        // 0x01, s:0, e:0
	uint8_t             :7;  //       l:0, s:7
	uint8_t  intKeyVisib:1;  // 0x02, s:7, e:8
	uint8_t  pairCentral[3]; // 0x0a, s:0, e:0
};

struct s_regChanL4 {
	// 0x01,
	uint8_t  peerNeedsBurst:1; // 0x01, s:0, e:1
	uint8_t                :7; //
};

struct s_regDev {
	s_regDevL0 l0;
};

struct s_regChan {
	s_regChanL4 l4;
};

struct s_regs {
	s_regDev ch0;
	s_regChan ch1;
} regs; // 11 byte


// channel device list table, 22 bytes
s_cnlDefType cnlDefType[] PROGMEM = {
	// cnl, lst, pMax, sIdx, sLen, pAddr, pPeer, *pRegs;	// pointer to regs structure
	{0, 0, 0, 0x00, 5, 0x0000, 0x0000, (void*)&regs.ch0.l0},
	{1, 4, 6, 0x05, 1, 0x0005, 0x0000, (void*)&regs.ch1.l4},
};


// handover to AskSin lib, 6 bytes
HM::s_devDef dDef = {
	1, 2, sliceStr, cnlDefType,
};

/**
 * eeprom definition, 16 bytes
 * define start address  and size in eeprom for magicNumber, peerDB, regsDB, userSpace
 */
HM::s_eeprom ee[] = {
	{0x0000, 0x0002, 0x001a, 0x0025,},
	{0x0002, 0x0018, 0x000b, 0x0000,},
};



// defaults definitions
const uint8_t regs01[] PROGMEM = {0x00,0x63,0x19,0x63};
const uint8_t regs03[] PROGMEM = {0x1f,0xa6,0x5c,0x06};
const uint8_t regs04[] PROGMEM = {0x1f,0xa6,0x5c,0x05};

s_defaultRegsTbl defaultRegsTbl[] = {
	// peer(0) or regs(1), channel, list, peer index, len, pointer to payload
	{  1,                  0,       0,    0,          5,   regs01},
};

HM::s_dtRegs dtRegs = {
	// amount of lines in defaultRegsTbl[], pointer to defaultRegsTbl[]
	0, defaultRegsTbl
};

