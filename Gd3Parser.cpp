#include "t85playdefs.cpp"

#include <ios>
#include <iostream>
#include <string>

#include "BitConverter.cpp"


// The GD3 data
typedef struct __gd3 {
	std::string trackNameEnglish;
	std::string trackNameOG;
	std::string gameNameEnglish;
	std::string gameNameOG;
	std::string systemNameEnglish;
	std::string systemNameOG;
	std::string authorNameEnglish;
	std::string authorNameOG;
	std::string releaseDate;
	std::string vgmDumper;
	std::string notes;
} gd3;

static const std::string gd3Errors[] = {
	"",
	"Gd3 ID does not match",
	"Gd3 version too new",
	"Gd3 size too small",
	"Gd3 size outside of file bounds",
	"Too few strings"
};

gd3 * readGd3Data(std::ifstream & file, uint32_t fileSize, uint32_t gd3Offset, int & errCode) {
	errCode = 0;
	file.seekg(gd3Offset, std::ios_base::beg);
	file.read(buffer, 4);
	if (memcmp(buffer, "Gd3 ", 4)) { errCode = 1; return nullptr; }
	file.read(buffer, 4);
	if (BitConverter::readUint32(buffer) > currentGd3Version) { errCode = 2; return nullptr; }
	file.read(buffer, 4);
	uint32_t gd3Size = BitConverter::readUint32(buffer);
	if (gd3Size < 11 * 2) { errCode = 3; return nullptr; }
	if (gd3Size + gd3Offset + 0x0C > fileSize) { errCode = 4; return nullptr; }
	char * dataBuffer = new char[gd3Size];
	file.read(dataBuffer, gd3Size);
	// Check for enough spaces
	size_t amountOfNulls = 0;
	for (size_t i = 0; i < gd3Size>>1; i++) {
		if (*((char16_t *)dataBuffer+i) == 0) amountOfNulls++;
	}
	if (amountOfNulls < 11) { errCode = 5; return nullptr; }

	// Actually convert the mfs
	gd3 * output = new gd3();
	std::u16string bufString;
	char16_t * currentData = (char16_t *)dataBuffer;


	for (int i = 0; i < 11; i++) {
		bufString = std::u16string(currentData);
		currentData += bufString.size()+1;
		*(((std::string *)output)+i) = utf16Converter.to_bytes(bufString);
	}

	delete [] dataBuffer;

	return output;
}
