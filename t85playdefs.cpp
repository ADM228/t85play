#ifndef __T85PLAY_HPP_INCLUDED__
#define __T85PLAY_HPP_INCLUDED__

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cwchar>

#include <filesystem>
#include <fstream>

#include <list>
#include <array>

#include <sndfile.hh>
#include <soundio/soundio.h>

#include "t85apu.hpp"
#include "NotchFilter.cpp"

// typedefs go here
struct __gd3;
typedef __gd3 gd3;

// utility wrapper to adapt locale-bound facets for wstring/wbuffer convert
template<class Facet>
struct deletable_facet : Facet
{
    template<class... Args>
    deletable_facet(Args&&... args) : Facet(std::forward<Args>(args)...) {}
    ~deletable_facet() {}
};


// constants
static const uint32_t currentFileVersion = 0x00000000;	// 16.8.8 bits semantic versioning
static const uint32_t currentGd3Version = 0x00000100;

// global variables
t85APUHandle apu;

std::filesystem::path inFilePath;
bool inFileDefined = false;
std::ifstream regDumpFile;

std::filesystem::path outFilePath;
bool outFileDefined = false;
bool sepFileOutput = false;

uint32_t sampleRate = 44100;
bool sampleRateDefined = false;

BandFilter::BandFilter notchFilter;
bool notchFilterEnabled = false;

uint8_t outputMethod;
bool outputMethodOverride = false;

double ticksPerSample;

double sampleTickCounter = 0.0;

uint_fast16_t waitTimeCounter = 0;

uint32_t regDataLocation, gd3DataLocation, loopOffset, extraHeaderOffset;
uint32_t apuClock, totalSmpCount, loopLength;

char buffer[4];

bool ended = false;

std::list<uint16_t> regWrites;

std::wstring_convert<deletable_facet<std::codecvt<char16_t, char, std::mbstate_t>>, char16_t> utf16Converter;

gd3 * gd3Data;

typedef struct __extraDataForIndividualChannels {
    std::array<SndfileHandle, 5> handles;
    std::array<int16_t *, 5> buffers;
} edfic;

// functions
void emulationTick(std::ifstream & file);


#endif //__T85PLAY_HPP_INCLUDED__