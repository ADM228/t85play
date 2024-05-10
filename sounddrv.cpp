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

#include <algorithm>
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

#include "build/_deps/t85apu_emu-src/emu/libt85apu/t85apu_regdefines.h"
#define BITCONVERTER_ARRAY_CONVS
#include "BitConverter.cpp"

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

#define ENV_COUNT 5

#define cmdMacro(cmd, num) (cmd<<8)|num
#define Cinst(num) cmdMacro(1, num)
#define Cspeed(num) cmdMacro(2, num)
#define Cwait(wait) cmdMacro(3, wait)
#define Cnt_w(note, wait) cmdMacro(4, note), Cwait(wait)
#define Cvol(vol) cmdMacro(5, vol)

#define i_volData(instData) ((const byte * const)instData.envData[0])
#define i_dutyData(instData) ((const byte * const)instData.envData[1])
#define i_compData(instData) ((const byte * const)instData.envData[2])
#define i_nOffData(instData) ((const char * const)instData.envData[3])
#define i_eOffData(instData) ((const char * const)instData.envData[4])

#define lo(num) (num & 0xFF)
#define hi(num) lo(num >> 8)
#define ntVal(incmt, octave) cmdMacro(octave, incmt)
#define byte uint8_t
typedef struct __ChEnv {
	const void * const envData[ENV_COUNT];
	const int envSz[ENV_COUNT], envLp[ENV_COUNT], envRl[ENV_COUNT];
} ChEnv;

#define env_oneshot(env) env, -1, -1
#define env_loopwhole(env) env, 0, sizeof(env)
#define env_null __envNull, -1, -1
#define env_snull __envSNull, -1, -1
#define env_custom(env, lp, rl) env, lp, rl

#define make_instrument(volEnv, volLp, volRl, dutyEnv, dutyLp, dutyRl, compEnv, compLp, compRl, nOffEnv, nOffLp, nOffRl, eOffEnv, eOffLp, eOffRl) \
{ \
	{volEnv, dutyEnv, compEnv, nOffEnv, eOffEnv}, \
	{sizeof(volEnv), sizeof(dutyEnv), sizeof(compEnv), sizeof(nOffEnv), sizeof(eOffEnv)}, \
	{volLp, dutyLp, compLp, nOffLp, eOffLp}, {volRl, dutyRl, compRl, nOffRl, eOffRl} \
}
#define make_instrument2(envA, envB, envC, envD, envE) make_instrument(envA,envB,envC,envD,envE)

typedef struct __inst {
	const ChEnv * inst;
	size_t envIdx[ENV_COUNT];
	byte state;
} Instrument;

typedef struct __regwrite {
	byte addr, data;
	size_t frame;
} regwrite;

typedef struct __chanH {
	const int8_t * arpTable;
	size_t noteIdx = 0, arpIdx = 0, runtime = 0;
	unsigned int speed = 1, instIdx = 0, volume = 255;
	Instrument instrument;
} chanHandler;

#define empChArr(x) (char *)std::array<uint8_t, x>().data()
#define NTSC_TM 735
#define PAL_TM 882

const unsigned int notesA[] = {
	Cinst(0), Cspeed(8), Cvol(255),
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4,
	Cnt_w(F1, 8 * (6-1)), OFF, F1, CNT, OFF, Cwait(0)
};

const unsigned int notesB[] = {
	Cinst(0), Cspeed(8), Cvol(128), Cwait(12),
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, C4, F4, BB3, F4, AB3, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, 
	F3, F4, EB4, F4, DB4, F4, C4, F4, OFF, Cwait(255), Cwait(255)
};

const unsigned int * const notes[] = {notesA, notesB};

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

const int8_t __envSNull[] = {0};
const byte * const __envNull = (const byte * const)__envSNull;

const byte envCompPulse[] = {CMP___T};

const ChEnv instList[] = { 
	make_instrument2(env_oneshot(envVol0), env_loopwhole(envDuty0), env_oneshot(envCompPulse), env_snull, env_snull)
};

uint_fast16_t * generateNoteTable(double clk = 8000000, double a4 = 440.0, int size = 96) {
	uint_fast16_t * output = (uint_fast16_t *)calloc(size, sizeof(uint_fast16_t));
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
		output[i] = ntVal(increment, octave);
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
	for (int env = 0; env < 3; env++) {
		inst.envIdx[env] = inst.inst->envLp[env] >= 0 ? inst.inst->envLp[env] : inst.inst->envSz[env] - 1;
	}
}

void instrument_reset(Instrument & inst) {
	memset(inst.envIdx, 0, sizeof(size_t)*3);
	inst.state = INST_ST_NRM;
}

int main() {
	auto ntTbl = generateNoteTable();
	
	std::array<chanHandler, 5> channels;
	uint8_t stopped = 0;
	uint_fast16_t notePitches[8], finalPitches[8];

	size_t frame = 0;
	std::vector<regwrite> regWriteList;
	std::for_each(channels.begin(), channels.end(), [](chanHandler & c){instrument_reset(c.instrument); c.instrument.state = INST_ST_STP;});
	// while (stopped != (1<<5)-1) {
	while (stopped == 0) {
		printf("Fr %zu, cmdIdx %zu, runtime %zu\n", frame, channels[0].noteIdx, channels[0].runtime); fflush(stdout);
		for (int __chidx = 0; __chidx < 2; __chidx++) {
			auto & c = channels[__chidx];
			auto & inst = (c.instrument);
			while (!c.runtime && !(stopped & 1<<__chidx)) {
				auto curNote = notes[__chidx][c.noteIdx];
				switch(hi(curNote)){
					case 1:
						c.instIdx = lo(curNote);
						inst.inst = &(instList[c.instIdx]);
						break;
					case 2:
						c.speed = lo(curNote);
						break;
					case 3:
						if (lo(curNote) == 0) stopped |= 1<<__chidx;
						c.runtime = lo(curNote);
						break;
					case 5:
						c.volume = lo(curNote);
						break;
					case 0:
					case 4:
					default:
						auto note = lo(curNote);
						if (note >= C0 && note <= B7) {
							instrument_reset(inst);
							notePitches[__chidx] = ntTbl[note];
						} else if (note == CNT) {
						} else if (note == OFF) {
							notePitches[__chidx] = 0;
							inst.state = INST_ST_STP;
						} else if (note == REL) {
							instrument_release(inst);
						}
						if (hi(curNote) == 0)
							c.runtime = c.speed;
						break;
				}
				c.noteIdx++;
			}
		}

		{
			uint8_t octave = 0;
			for (int idx = 0; idx < 8; idx++) {
				static const byte regtableLo[] = {
					PILOA, PILOB, PILOC, PILOD, PILOE, PILON, EPLOA, EPLOB
				};
				static const byte regTableHi[] = {PHIAB, PHICD, PHIEN, EPIHI};
				regWriteList.push_back({regtableLo[idx], (byte)lo(notePitches[idx]),frame});
				if (!(idx & 1)) octave = PitchHi_Env_A(hi(notePitches[idx]));
				else {
					octave |= PitchHi_Env_B(hi(notePitches[idx]));
					/* if (idx != 7) */ octave &= 0x77;
					regWriteList.push_back({regTableHi[idx>>1], octave, frame});
				}
			}
		}

		// for (auto & c : channels) {
		for (int __chidx = 0; __chidx < 2; __chidx++) { auto & c = channels[__chidx];
			auto & inst = c.instrument;
			auto & instData = *(inst.inst);
			
			if (inst.state != INST_ST_STP) {
				regWriteList.push_back({(byte)(VOL_A+__chidx), (byte)(i_volData(instData)[inst.envIdx[0]] * (c.volume / 255.0)), frame});
				regWriteList.push_back({(byte)(DUTYA+__chidx), i_dutyData(instData)[inst.envIdx[1]], frame});

				for (int env = 0; env < 3; env++) {
					
					auto & idx = inst.envIdx[env];
					auto & size = instData.envSz[env];
					auto & loop = instData.envLp[env];
					auto & rel = instData.envRl[env];

					idx++;
					if (inst.state == INST_ST_NRM) {
						if (idx >= rel) idx = loop;
					} 
					if (idx >= size) idx = size - 1;
				}
			}
			c.runtime--;
		}
		frame++;
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

	#define HEADER_SIZE 0x2C

	auto file = std::fstream("build/out.t85", std::ios_base::binary|std::ios_base::out);
	file.write("t85!", 4);
	file.write((char *)BitConverter::toByteArray(uint32_t(output.size()+HEADER_SIZE-4)).data(), 4);
	file.write(empChArr(4), 4); // file version 0
	file.write((char *)BitConverter::toByteArray(uint32_t(8000000)).data(), 4);
	file.write((char *)BitConverter::toByteArray(uint32_t(HEADER_SIZE-16)).data(), 4);
	file.write(empChArr(4), 4); // gd3 offset - none
	file.write((char *)BitConverter::toByteArray(uint32_t((frame+1) * 735)).data(), 4);
	file.write(empChArr(16), 16); // loop, loop, exheader, outmethod, reserved
	file.write((char *)output.data(), output.size());
	file.close();

	// for (auto & i : output) 
	// 	printf("%02X", i);

	return 0;
}