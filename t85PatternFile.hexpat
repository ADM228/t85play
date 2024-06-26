#include <std/mem.pat>
#include <std/io.pat>

#pragma pattern_limit 16777216

struct vgmDataEntry {
    u8 type;
    if (type == 0x41) {
        u8 address, value;
    } else if (type == 0x61) {
        u16 length;
    } else if (type == 0x62 || type == 0x63) {
    } else if (type == 0x66) {
        break;
    }
};

u128 waitTime out;
waitTime = 0;

struct vgmDataMeasureEntry {
    vgmDataEntry entry;
    if (entry.type == 0x61) waitTime += length;
    else if (entry.type == 0x62) waitTime += (44100 / 60);
    else if (entry.type == 0x63) waitTime += (44100 / 50);
//    if (entry.type >= 0x61 && entry.type <= 0x63) std::print("{}", waitTime);
};

fn limitStr16BySize(u32 size, u128 startptr, u128 ptr) {
    if (startptr - ptr >= size) return 0;
    if (std::mem::read_unsigned(ptr, 2) == 0) return 0;
    return 1;
};

struct gd3Data {
    char gd3[4];
    u32 version;
    u32 size;
    char16 songNameEN[];
    char16 songNameOG[];
    char16 gameNameEN[];
    char16 gameNameOG[];
    char16 sysNameEN[];
    char16 sysNameOG[];
    char16 authorNameEN[];
    char16 authorNameOG[];
    char16 date[];
    char16 vgmBy[];
    char16 notes[];
};

struct t85Header {
    char t85[4];
    u32 eofOffset;
    u32 fileVersion;
    u32 clock;
    u32 vgmDataOffset;
    u32 gd3DataOffset;
    u32 totalSampleCount;
    u32 loopOffset;
    u32 loopLength;
    u32 extraHeaderOffset;
    u8 outMethod;
    if (fileVersion == 0){
        padding[3];
    }
};
t85Header header @ $;

u32 clock out;
u32 outMethod out;
u32 totalSampleCount out;
u32 loopLength out;
clock = header.clock;
outMethod = header.outMethod;
totalSampleCount = header.totalSampleCount;
loopLength = header.loopLength;

u32 vgmDataStart = header.vgmDataOffset + 4*4;
u32 gd3DataStart = header.gd3DataOffset + 5*4;

vgmDataMeasureEntry vgmData[while($ < gd3DataStart)] @ vgmDataStart;
std::print("Total wait count: {}", waitTime);

gd3Data gd3 @ gd3DataStart;