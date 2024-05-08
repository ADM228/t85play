/*	 _______________________________________________________________________
	|		|	0	|	1	|	2	|	3	|	4	|	5	|	6	|	7	|
	|=======|=======|=======|=======|=======|=======|=======|=======|=======|
	|  0x00 |		"t85!" ID string		|		End of file offset	 	|
	|  0x08	|		  File version			|		APU clock speed			|
	|  0x10	|		VGM data offset			|			GD3 offset			|
	|  0x18	|	Total number of samples		|			Loop offset			|
	|  0x20	|	Amount of looping samples	|	  Extra header offset		|
	|  0x28	|OutMthd|	**   RESERVED   **	|_______________________________|
	|_______|_______________________________|

*/

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <vector>
#include <fstream>

#include "../build/_deps/t85apu_emu-src/emu/libt85apu/t85apu_regdefines.h"
#define BITCONVERTER_ARRAY_CONVS
#include "../BitConverter.cpp"

#pragma region noteDefines

#define C0 0
#define BS0 0
#define CS0 1
#define DB0 1
#define D0 2
#define DS0 3
#define EB0 3
#define E0 4
#define FB0 4
#define F0 5
#define ES0 5
#define FS0 6
#define GB0 6
#define G0 7
#define GS0 8
#define AB0 8
#define A0 9
#define AS0 10
#define BB0 10
#define B0 11
#define CB0 11
#define C1 12
#define BS1 12
#define CS1 13
#define DB1 13
#define D1 14
#define DS1 15
#define EB1 15
#define E1 16
#define FB1 16
#define F1 17
#define ES1 17
#define FS1 18
#define GB1 18
#define G1 19
#define GS1 20
#define AB1 20
#define A1 21
#define AS1 22
#define BB1 22
#define B1 23
#define CB1 23
#define C2 24
#define BS2 24
#define CS2 25
#define DB2 25
#define D2 26
#define DS2 27
#define EB2 27
#define E2 28
#define FB2 28
#define F2 29
#define ES2 29
#define FS2 30
#define GB2 30
#define G2 31
#define GS2 32
#define AB2 32
#define A2 33
#define AS2 34
#define BB2 34
#define B2 35
#define CB2 35
#define C3 36
#define BS3 36
#define CS3 37
#define DB3 37
#define D3 38
#define DS3 39
#define EB3 39
#define E3 40
#define FB3 40
#define F3 41
#define ES3 41
#define FS3 42
#define GB3 42
#define G3 43
#define GS3 44
#define AB3 44
#define A3 45
#define AS3 46
#define BB3 46
#define B3 47
#define CB3 47
#define C4 48
#define BS4 48
#define CS4 49
#define DB4 49
#define D4 50
#define DS4 51
#define EB4 51
#define E4 52
#define FB4 52
#define F4 53
#define ES4 53
#define FS4 54
#define GB4 54
#define G4 55
#define GS4 56
#define AB4 56
#define A4 57
#define AS4 58
#define BB4 58
#define B4 59
#define CB4 59
#define C5 60
#define BS5 60
#define CS5 61
#define DB5 61
#define D5 62
#define DS5 63
#define EB5 63
#define E5 64
#define FB5 64
#define F5 65
#define ES5 65
#define FS5 66
#define GB5 66
#define G5 67
#define GS5 68
#define AB5 68
#define A5 69
#define AS5 70
#define BB5 70
#define B5 71
#define CB5 71
#define C6 72
#define BS6 72
#define CS6 73
#define DB6 73
#define D6 74
#define DS6 75
#define EB6 75
#define E6 76
#define FB6 76
#define F6 77
#define ES6 77
#define FS6 78
#define GB6 78
#define G6 79
#define GS6 80
#define AB6 80
#define A6 81
#define AS6 82
#define BB6 82
#define B6 83
#define CB6 83
#define C7 84
#define BS7 84
#define CS7 85
#define DB7 85
#define D7 86
#define DS7 87
#define EB7 87
#define E7 88
#define FB7 88
#define F7 89
#define ES7 89
#define FS7 90
#define GB7 90
#define G7 91
#define GS7 92
#define AB7 92
#define A7 93
#define AS7 94
#define BB7 94
#define B7 95
#define CB7 95

#pragma endregion

#define OFF 255
#define REL 254
#define CNT 253

#define CMP___T 1
#define CMP__N_ 2
#define CMP__NT 3
#define CMP_E__ 4
#define CMP_E_T 5
#define CMP_EN_ 6
#define CMP_ENT 7

#define INST_ST_NRM 0
#define INST_ST_REL 1
#define INST_ST_STP 2

#define cmdMacro(cmd, num) (cmd<<8)|num
#define Cinst(num) cmdMacro(1, num)
#define Cspeed(num) cmdMacro(2, num)
#define Cwait(wait) cmdMacro(3, wait)
#define Cnt_w(note, wait) cmdMacro(4, note), Cwait(wait)
#define Cvol(vol) cmdMacro(5, vol)

#define lo(num) (num & 0xFF)
#define hi(num) lo(num >> 8)
#define ntVal(incmt, octave) cmdMacro(octave, incmt)
#define byte uint8_t
typedef struct __ChEnv {
	const byte * const volEnv;
	const int volSz, volLp, volRl;
	const byte * const dutyEnv;
	const int dutySz, dutyLp, dutyRl;
} ChEnv;

typedef struct __inst {
	const ChEnv * inst;
	size_t volIdx = 0, dutyIdx = 0;
	byte state = INST_ST_NRM;
} Instrument;

typedef struct __regwrite {
	byte addr, data;
	size_t frame;
} regwrite;

#define empChArr(x) (char *)std::array<uint8_t, x>().data()
#define NTSC_TM 735
#define PAL_TM 882

const unsigned int notesA[] = {
	Cinst(0), Cspeed(8), Cvol(255),
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	Cvol(128), 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4,
	Cnt_w(F1, 8 * (6-1)), OFF, F1, CNT, OFF
};

const byte envVol0[] = {
	255, 255, 255, 255, 128, 128, 128, 128
};

const byte envDuty0[] = {
	0x80, 0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 
	0x40, 0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 12, 
	10, 9, 8, 9,
	10, 12, 16, 24, 32, 0x28, 0x30, 0x38,
	0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
};

const ChEnv instList[] = { 
	{
		envVol0, sizeof(envVol0), -1, -1,
		envDuty0, sizeof(envDuty0), 0, sizeof(envDuty0)
	}
};

uint_fast8_t * generateNoteTable(double clk = 8000000, double a4 = 440.0, int size = 96) {
	uint_fast8_t * output = (uint_fast8_t *)calloc(size * 2, sizeof(uint_fast8_t));
	static const int a4Pos = A4; 
	const double mfreq = clk / 512;
	for (int i = 0; i < size; i++) {
		double noteFreq = a4 * (pow(2, (i-a4Pos)/12.0));
		double fullIncrement = noteFreq / mfreq * (1<<UINT16_WIDTH) / 2;
		if (round(fullIncrement) > UINT16_MAX) fullIncrement = UINT16_MAX;
		byte octave = std::fmax(floor(std::log2(fullIncrement) - 8 + 1), 0);
		int increment = round(fullIncrement / std::pow(2, octave));
		if (increment > UINT8_MAX) {
			if (octave > 8 - 1) std::cerr << "excuse me what the fuck: " << octave << ", " << increment << " <- " << fullIncrement << " <-" << noteFreq << std::endl;
			increment = round(increment / 2.0); octave++;
		}
		octave &= 7; increment &= 0xFF;
		output[i] = increment;
		output[size+i] = octave;
		printf("#%02d: %d:%02X <- %lf <- %lfHz\n", i, octave, increment, fullIncrement * 2, noteFreq);
	}
	return output;
}

void optimizeWrites(std::vector<regwrite> & list) {
	std::vector<regwrite> output;
	byte internalState[32];
	std::memset(internalState, 0, sizeof(internalState));
	std::memset(internalState+CFG_A, 0xF, 5);
	internalState[NTPHI] = 0x24;

	for(auto & write : list) {
		if (write.data != internalState[write.addr]) {
			output.push_back(write);
			internalState[write.addr] = write.data & ((
				write.addr == PHIAB || 
				write.addr == PHICD ||
				write.addr == PHIEN ||
				write.addr == E_SHP) ? 0x77 : 0xFF);
		}
	}
	list = output;
}

void instrument_release(Instrument & inst) {
	inst.state = INST_ST_REL;
	inst.volIdx = inst.inst->volLp >= 0 ? inst.inst->volLp : inst.inst->volSz - 1;
	inst.dutyIdx = inst.inst->dutyLp >= 0 ? inst.inst->dutyLp : inst.inst->dutySz - 1;
}

int main() {
	auto ntTbl = generateNoteTable();
	uint_fast8_t * const ntLo = &ntTbl[0], * const ntHi = &ntTbl[96]; 
	size_t noteIdx[5], cmdIdx[5];
	unsigned int speed[5], instrumentIdx[5], channelVolume[5];
	size_t frame = 0;
	Instrument instruments[5];
	std::vector<regwrite> regWriteList;
	regWriteList.push_back({DUTYA, 0x80, 0});
	memset(noteIdx,	0,	sizeof(noteIdx));
	memset(cmdIdx,	0,	sizeof(cmdIdx));
	while (noteIdx[0] < 70) {
		for (int ch = 0; ch < 1; ch++) {
			auto curNote = notesA[noteIdx[ch]];
			auto & inst = (instruments[ch]);
			int len = 0;
			switch(hi(curNote)){
				case 1:
					instrumentIdx[ch] = lo(curNote);
					break;
				case 2:
					speed[ch] = lo(curNote);
					break;
				case 3:
					len = lo(curNote);
					break;
				case 4:
					curNote = lo(curNote);
					if (curNote >= C0 && curNote <= B7) {
						inst = Instrument();
						inst.inst = &(instList[instrumentIdx[ch]]);
						regWriteList.push_back({PHIAB, (byte)PitchHi_Sq_A(ntHi[curNote]), frame});
						regWriteList.push_back({PILOA, ntLo[curNote], frame});
					} else if (curNote == CNT) {
					} else if (curNote == OFF) {
						regWriteList.push_back({VOL_A, 0x00, frame});
						inst.state = INST_ST_STP;
					} else if (curNote == REL) {
						instrument_release(inst);
					}
					break;
				case 5:
					channelVolume[ch] = lo(curNote);
					break;
				case 0:
				default:
					if (curNote >= C0 && curNote <= B7) {
						inst = Instrument();
						inst.inst = &(instList[instrumentIdx[ch]]);
						regWriteList.push_back({PHIAB, (byte)PitchHi_Sq_A(ntHi[curNote]), frame});
						regWriteList.push_back({PILOA, ntLo[curNote], frame});
					} else if (curNote == CNT) {
					} else if (curNote == OFF) {
						regWriteList.push_back({VOL_A, 0x00, frame});
						inst.state = INST_ST_STP;
					} else if (curNote == REL) {
						instrument_release(inst);
					}
					len = speed[ch];
					break;
			}
			auto & instData = *(inst.inst);
			
			for (int t = 0; t < len; t++, frame++) {
				if (inst.state != INST_ST_STP) {
					regWriteList.push_back({VOL_A, (byte)(instData.volEnv[inst.volIdx++] * (channelVolume[ch] / 255.0)), frame});
					regWriteList.push_back({DUTYA, instData.dutyEnv[inst.dutyIdx++], frame});
					if (inst.state == INST_ST_NRM) {
						if (inst.volIdx >= instData.volRl) inst.volIdx = instData.volLp;
						if (inst.dutyIdx >= instData.dutyRl) inst.dutyIdx = instData.dutyLp;
					} 
					if (inst.volIdx >= instData.volSz) inst.volIdx = instData.volSz - 1;
					if (inst.dutyIdx >= instData.dutySz) inst.dutyIdx = instData.dutySz - 1;
				}
			}
			noteIdx[0]++;
		}
	}

	// std::cout << "Old regiwrtes:" <<std::endl;
	// for (auto & regwrite : regWriteList) {
	// 	printf("%02X <- %02X on %ld\n", regwrite.addr, regwrite.data, regwrite.frame);
	// }
	// auto size = regWriteList.size();
	optimizeWrites(regWriteList);
	// std::cout << "New regiwrtes:" <<std::endl;

	// for (auto & regwrite : regWriteList) {
	// 	printf("%02X <- %02X on %ld\n", regwrite.addr, regwrite.data, regwrite.frame);
	// }
	// std::cout << "Old size: " << size << ", new size: " << regWriteList.size() << std::endl;
	size_t writeIdx = 0;
	frame = 0;
	std::vector<byte> output;
	while (writeIdx < regWriteList.size()) {
		auto & write = regWriteList[writeIdx];
		{
			size_t waitTime = 0;
			while (frame < write.frame) {
				waitTime += NTSC_TM;
				frame++;
			}
			while (waitTime) {
				if (waitTime == NTSC_TM) { output.push_back(0x62); waitTime -= NTSC_TM; }
				else if (waitTime == PAL_TM) { output.push_back(0x63); waitTime -= PAL_TM; }
				else if (waitTime == 2 * NTSC_TM) {
					output.push_back(0x62); waitTime -= NTSC_TM;
					output.push_back(0x62); waitTime -= NTSC_TM;
				} else if (waitTime == 2 * PAL_TM) {
					output.push_back(0x63); waitTime -= PAL_TM;
					output.push_back(0x63); waitTime -= PAL_TM;
				} else if (waitTime == NTSC_TM + PAL_TM) {
					output.push_back(0x62); waitTime -= NTSC_TM;
					output.push_back(0x63); waitTime -= PAL_TM;
				} else if (waitTime <= UINT16_MAX) { 
					output.push_back(0x61); output.push_back(lo(waitTime)); output.push_back(hi(waitTime)); waitTime = 0; 
				} else if (waitTime <= UINT16_MAX + PAL_TM) {
					output.push_back(0x63); waitTime -= PAL_TM;
					output.push_back(0x61); output.push_back(lo(waitTime)); output.push_back(hi(waitTime)); waitTime = 0;
				} else if (waitTime <= UINT16_MAX + PAL_TM + PAL_TM) {
					output.push_back(0x63); waitTime -= PAL_TM;
					output.push_back(0x63); waitTime -= PAL_TM;
					output.push_back(0x61); output.push_back(lo(waitTime)); output.push_back(hi(waitTime)); waitTime = 0;
				} else if (waitTime > UINT16_MAX + PAL_TM + PAL_TM) {
					output.push_back(0x61); output.push_back(0xFF); output.push_back(0xFF); waitTime -= UINT16_MAX;
				}
			}
		}
		output.push_back(0x41);
		output.push_back(write.addr);
		output.push_back(write.data);
		writeIdx++;
	}
	output.push_back(0x62), output.push_back(0x66);

	auto file = std::fstream("build/out.t85", std::ios_base::binary|std::ios_base::out);
	file.write("t85!", 4);
	file.write((char *)BitConverter::toByteArray(uint32_t(output.size()+0x28-4)).data(), 4);
	file.write(empChArr(4), 4); // file version 0
	file.write((char *)BitConverter::toByteArray(uint32_t(8000000)).data(), 4);
	file.write((char *)BitConverter::toByteArray(uint32_t(0x28-16)).data(), 4);
	file.write(empChArr(4), 4); // gd3 offset - none
	file.write((char *)BitConverter::toByteArray(uint32_t((frame+1) * 735)).data(), 4);
	file.write(empChArr(16), 16); // loop, loop, exheader, outmethod, reserved
	file.write((char *)output.data(), output.size());
	file.close();

	// for (auto & i : output) 
	// 	printf("%02X", i);

	return 0;
}