#include "BitConverter.cpp"
#include "t85playdefs.cpp"
#include "Gd3Parser.cpp"
#include "soundioutils.cpp"

// t85 format is VGM-inspired:

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

void emulationTick(std::ifstream & file) {
	totalSmpCount--;
	if (waitTimeCounter) waitTimeCounter--;
	else {
		uint8_t cmd = file.get();
		// std::cout << (int)cmd << std::endl;
		switch(cmd) {
			case 0x41:
				file.read(buffer, 2);
				regWrites.push_back(BitConverter::readUint16(buffer));
				break;
			case 0x61:
				file.read(buffer, 2);
				waitTimeCounter = BitConverter::readUint16(buffer)-1;
				break;
			case 0x62:
				waitTimeCounter = 735-1;
				break;
			case 0x63:
				waitTimeCounter = 882-1;
				break;
			case 0x66:
			default:
				break;
		}
	}
}

int main (int argc, char** argv) {
	std::cout << "ATtiny85APU register dump player v0.1" << std::endl << "© alexmush, 2024" << std::endl << std::endl;
	for (int i = 1; i < argc; i++) {	// Check for the help command specifically
		if (!(
			memcmp(argv[i], "-h", 3) &&
			memcmp(argv[i], "--help", 2+4+1))) {
                  std::cout << 
R"(Command-line options:
-i <input file> - specify a .t85 register dump to read from
	= --input
-o <output file> - specify a .wav file to output to
	= --output
	If not specified, the audio will be played back live
-s <rate> - specify audio output sample rate in Hz
	= --sample-rate
	Default value: 44100
	Note: the emulation rate always remains at 44100Hz
-f - enable 15625Hz notch filter
	= --filter
-nf - disable 15625Hz notch filter
	= --no-filter
	Default value:
		Enabled when output mode == 1
		Disabled if output mode != 1
		Cannot be enabled if sample rate ≤ 31250Hz
-om <mode> - force output mode
	= --output-method
	Default value: whatever is specified in the register dump file
-h - show this help screen
	= --help
-c - export separate channel audio as well
	= --channels-separate)"
                            << std::endl;
                  std::exit(0);
		}
	}
	for (int i = 1; i < argc; i++) {
		if (! (
			memcmp(argv[i], "-i", 3) && 
			memcmp(argv[i], "--input", 8))) {
			if (inFileDefined) {
				std::cerr << "Cannot input more than 1 file at once" << std::endl;
			} else if (i+1 >= argc) {
				std::cerr << "File argument not supplied" << std::endl;
			} else if (argv[i+1][0] == char("-"[0])){
				std::cerr << "Invalid file argument: \"" << argv[i+1] << "\"" << std::endl;
				i--;
			} else {
				inFilePath = std::filesystem::path(argv[i+1]);
				if (!std::filesystem::exists(inFilePath)) {
					std::cerr << "File \"" << argv[i+1] << "\" does not exist" << std::endl;
				} else {
					inFileDefined = true;
				}
			}
			i++;
		} else if (! (
			memcmp(argv[i], "-o", 3) && 
			memcmp(argv[i], "--output", 9))) {
			if (outFileDefined) {
				std::cerr << "Cannot output to more than 1 file at once" << std::endl;
			} else if (i+1 >= argc) {
				std::cerr << "File argument not supplied" << std::endl;
			} else if (argv[i+1][0] == char("-"[0])){
				std::cerr << "Invalid file argument: \"" << argv[i+1] << "\"" << std::endl;
				i--;
			} else {
				outFilePath = std::filesystem::path(argv[i+1]);
				if (!std::filesystem::exists(outFilePath.parent_path())) {
					std::cerr << "Directory \"" << outFilePath.parent_path() << "\" does not exist" << std::endl;
				} else {
					outFileDefined = true;
				}
			}
			i++;
		} else if (!(
			memcmp(argv[i], "-s", 3) && 
			memcmp(argv[i], "--sample-rate", 2+6+1+4+1))) {
			// Sample rate
			if (i+1 >= argc) {
				std::cerr << "Sample rate argument not supplied" << std::endl;
			} else {
				char* p;
				auto tmp = strtol(argv[i+1], &p, 10);
				if (*p) {
					std::cerr << "Invalid sample rate argument: \"" << argv[i+1] << "\"" << std::endl;
				} else {
					sampleRate = tmp;
					sampleRateDefined = true;
				}
			}
			i++;
		} else if (!(
			memcmp(argv[i], "-om", 4) &&
			memcmp(argv[i], "--output-method", 2+6+1+6+1))) {
			// Override output mode
			if (i+1 >= argc) {
				std::cout << "Output method argument not supplied" << std::endl;
			} else {
				char* p;
				auto tmp = strtol(argv[i+1], &p, 10);
				if (*p) {
					std::cerr << "Invalid output method argument: \"" << argv[i+1] << "\"" << std::endl;
				} else {
					outputMethod = tmp;
					outputMethodOverride = true;
				}
			}
			i++;
		} else if (!(
			memcmp(argv[i], "-f", 3) && 
			memcmp(argv[i], "--filter", 2+6+1))) {
			// Enable notch filter
			notchFilterEnabled = true;
		} else if (!(
			memcmp(argv[i], "-nf", 3) && 
			memcmp(argv[i], "--no-filter", 2+2+1+6+1))) {
			// Disable notch filter
			notchFilterEnabled = false;
		} else if (!(
			memcmp(argv[i], "-c", 3) && 
			memcmp(argv[i], "--channels-separate", 2+8+1+8))) {
			// Enable separate file output
			sepFileOutput = true;
		} 
	}
	if (!inFileDefined) {
		std::string filename;
		std::cout << "\tPlease input register dump file name: ";
		std::cin >> filename;
		inFilePath = std::filesystem::path(filename);
		if (!std::filesystem::exists(inFilePath)) {
			std::cerr << "File \"" << filename << "\" does not exist" << std::endl;
			std::exit(1);
		} else {
			inFileDefined = true;
		}
	}

	auto expectedFileSize = std::filesystem::file_size(inFilePath);
	regDumpFile.open(inFilePath, std::ios_base::binary|std::ios_base::in);

	// Read header
	regDumpFile.read(buffer, 4);
	if (memcmp(buffer, "t85!", 4)) {
		std::cerr << "File header does not match" << std::endl;
		std::exit(2);
	}

	// File size
	regDumpFile.read(buffer, 4);
	uint32_t fileSize = BitConverter::readUint32(buffer) + 0x04;
	if (fileSize > expectedFileSize) {
		std::cerr << "File seems to be cut off by " << fileSize - expectedFileSize << " bytes. " << std::endl;
		std::exit(3);
	} else if (fileSize < expectedFileSize) {
		std::cerr << "File seems to contain " << expectedFileSize - fileSize << " extra bytes of data. " << std::endl;
	}
	if (fileSize < 0x2C) {
		std::cerr << "File is too small for the t85 header" << std::endl;
		std::exit(2);
	}

	// File version
	regDumpFile.read(buffer, 4);
	uint32_t fileVersion = BitConverter::readUint32(buffer);
	if (fileVersion > currentFileVersion) {
		std::cerr << "File version (" << 
			(fileVersion>>16) << "." << 
			((fileVersion>>8) & 0xFF) << "." << 
			(fileVersion & 0xFF) << ") is newer than the player's maximum supported file version (" << 
			(currentFileVersion>>16) << "." << 
			((currentFileVersion>>8) & 0xFF) << "." << 
			(currentFileVersion & 0xFF) << "), please update the player at https://github.com/ADM228/ATtiny85APU" << std::endl;
		std::exit(4);
	}

	// APU clock speed
	regDumpFile.read(buffer, 4);
	apuClock = BitConverter::readUint32(buffer);
	if (apuClock == 0) {
		std::cout << "APU Clock speed not specified, assuming 8MHz." << std::endl;
		apuClock = 8000000;
	}

	// VGM data offset
	regDumpFile.read(buffer, 4);
	regDataLocation = BitConverter::readUint32(buffer);
	if (regDataLocation) {
		regDataLocation += 0x10;	// Offset after all
		if (regDataLocation > fileSize) {
			regDataLocation = 0;
			std::cerr << "Register dump pointer out of bounds." << std::endl;
		}
	}

	// GD3 data offset
	regDumpFile.read(buffer, 4);
	gd3DataLocation = BitConverter::readUint32(buffer);
	if (gd3DataLocation) {
		gd3DataLocation += 0x14;
		if (regDataLocation > fileSize) {
			gd3DataLocation = 0;
			std::cerr << "GD3 pointer out of bounds." << std::endl;
		}
	}

	// Total amount of samples
	regDumpFile.read(buffer, 4);
	totalSmpCount = BitConverter::readUint32(buffer);

	// Loop stuff
	regDumpFile.read(buffer, 4);
	loopOffset = BitConverter::readUint32(buffer);
	regDumpFile.read(buffer, 4);
	loopLength = BitConverter::readUint32(buffer);
	if (!loopOffset || !loopLength) {
		loopOffset = 0; loopLength = 0;
	} else {
		loopOffset += 0x1C;
		if (loopOffset > fileSize) {
			loopOffset = 0; loopLength = 0;
			std::cerr << "Loop pointer out of bounds, treating as no loop." << std::endl;
		} else if (loopLength > totalSmpCount) {
			loopOffset = 0; loopLength = 0;
			std::cerr << "Loop length out of bounds, treating as no loop." << std::endl;
		}
	}
	
	// Extra header
	regDumpFile.read(buffer, 4);
	extraHeaderOffset = BitConverter::readUint32(buffer);
	if (extraHeaderOffset) {
		extraHeaderOffset += 0x24;
		if (extraHeaderOffset > fileSize) {
			extraHeaderOffset = 0;
			std::cerr << "Extra header pointer out of bounds." << std::endl;
		}
	}

	// Output method
	regDumpFile.read(buffer, 1);
	if (!outputMethodOverride) {
		outputMethod = *buffer;
	}


	// Read GD3
	if (gd3DataLocation) {
		int errCode;

		gd3Data = readGd3Data(regDumpFile, fileSize, gd3DataLocation, errCode);

		if (gd3Data != nullptr && !errCode) {

			if (gd3Data->trackNameEnglish.size() + gd3Data->trackNameOG.size() > 0) {
				std::cout << "Track name: " <<
					gd3Data->trackNameEnglish << 
					(gd3Data->trackNameEnglish.size() && gd3Data->trackNameOG.size() ? " / " : "") <<
					gd3Data->trackNameOG << std::endl;
			}
			if (gd3Data->gameNameEnglish.size() + gd3Data->gameNameOG.size() > 0) {
				std::cout << "Game name: " <<
					gd3Data->gameNameEnglish << 
					(gd3Data->gameNameEnglish.size() && gd3Data->gameNameOG.size() ? " / " : "") <<
					gd3Data->gameNameOG << std::endl;
			}
			if (gd3Data->systemNameEnglish.size() + gd3Data->systemNameOG.size() > 0) {
				std::cout << "System: " <<
					gd3Data->systemNameEnglish << 
					(gd3Data->systemNameEnglish.size() && gd3Data->systemNameOG.size() ? " / " : "") <<
					gd3Data->systemNameOG << std::endl;
			}
			if (gd3Data->authorNameEnglish.size() + gd3Data->authorNameOG.size() > 0) {
				std::cout << "Author: " <<
					gd3Data->authorNameEnglish << 
					(gd3Data->authorNameEnglish.size() && gd3Data->authorNameOG.size() ? " / " : "") <<
					gd3Data->authorNameOG << std::endl;
			}
			if (gd3Data->releaseDate.size())
				std::cout << "Release date: " << gd3Data->releaseDate << std::endl;
			if (gd3Data->vgmDumper.size())
				std::cout << "T85 by: " << gd3Data->vgmDumper << std::endl;
			if (gd3Data->notes.size())
				std::cout << "Notes: \n===\n" << gd3Data->notes << "\n===" << std::endl;

		} else {
			std::cerr << gd3Errors[errCode] << std::endl;
		}

	}

	// Extra header (TODO)

	// Read the actual data

	if (!totalSmpCount) {
		std::cerr << "This file specifies its length as 0." << std::endl;
		std::exit(0);
	} else if (!regDataLocation) {
		std::cerr << "This file contains no register dump data." << std::endl;
		std::exit(0);
	}
	// There actually is data, let's go emulate
	apu = t85APU_new(apuClock, sampleRate, outputMethod);

	regDumpFile.seekg(regDataLocation);

	if (notchFilterEnabled) notchFilterEnabled = sampleRate >= apuClock/256.0;
	if (notchFilterEnabled) {
		setNotchFilterParams(notchFilter, (double)sampleRate, apuClock/512.0, 0.01);
		// Figure out the harmonics??????? it gives out 12850Hz and 18400Hz noises at 44100Hz, wtf 
		// setNotchFilterParams(notchFilter, (double)sampleRate, 12850, 0.01);
		resetFilter(notchFilter);
	}

	if (outFileDefined) {
		edfic * extraData;
		ticksPerSample = (double)sampleRate / 44100.0;
		SndfileHandle outFile(outFilePath, SFM_WRITE, SF_FORMAT_WAV|SF_FORMAT_PCM_16, 1, sampleRate);
		auto audioBuffer = new int16_t[sampleRate]; 
		if (sepFileOutput) {
			extraData = new edfic;
			std::string basepath = (outFilePath.parent_path() / outFilePath.stem()).string() + "_"; 
			for (int i = 0; i < 5; i++) {
				std::string name = std::to_string(i) + outFilePath.extension().string();
				extraData->buffers[i] = new int16_t[sampleRate];
				extraData->handles[i] = SndfileHandle(basepath + name, SFM_WRITE, SF_FORMAT_WAV|SF_FORMAT_PCM_16, 1, sampleRate);
			}
		}
		size_t idx = 0;

		while (totalSmpCount) {
			emulationTick(regDumpFile);
			sampleTickCounter += ticksPerSample;
			while (sampleTickCounter >= 1.0) {
				// std::cout << sampleTickCounter << "   " << regWrites.size() << std::endl;
				sampleTickCounter -= 1.0;
				if (!apu.shiftRegisterPending() && regWrites.size()) {
					apu.writeReg(regWrites.front()&0xFF, regWrites.front()>>8);
					std::cout << totalSmpCount << " - WR: " << std::hex << (regWrites.front()>>8) << "->" << (regWrites.front()&0xFF) << std::dec << std::endl;
					regWrites.pop_front();
				} 
				BitConverter::writeBytes(audioBuffer+(idx), (uint16_t)((apu.calc()<<(15-apu().outputBitdepth)) - (1<<14)));
				if (sepFileOutput) {
					for (int i = 0; i < 5; i++) {
						BitConverter::writeBytes(extraData->buffers[i]+idx, (apu().channelOutput[i]));
					}
				}
				idx++;
				if (idx >= sampleRate) {
					if (notchFilterEnabled) BandFilter::filterBuffer(notchFilter, audioBuffer, sampleRate);
					idx = 0;
					outFile.write(audioBuffer, sampleRate);
					if (sepFileOutput) {
						for (int i = 0; i < 5; i++) {
							extraData->handles[i].write(extraData->buffers[i], sampleRate);
						}
					}
				}
			}
		} 
		if (idx) {
			if (notchFilterEnabled) BandFilter::filterBuffer(notchFilter, audioBuffer, idx);
			outFile.write(audioBuffer, idx);
			if (sepFileOutput) {
				for (int i = 0; i < 5; i++) {
					extraData->handles[i].write(extraData->buffers[i], idx);
				}
			}
		}
		// outfile wil close on destruction, but the pointer won't
		delete[] audioBuffer;
		if (sepFileOutput) {
			for (auto & i : extraData->buffers) delete[] i;
			delete extraData;
		}
	} else {
		ticksPerSample =  44100.0 / (double)sampleRate;
		// Fucking live playback
		struct SoundIo *soundio = soundio_create();
		if (!soundio) {
			fprintf(stderr, "out of memory\n");
			return 1;
		}
		soundio->app_name = "t85play v0.1";
		int err = soundio_connect(soundio);
		if (err) {
			fprintf(stderr, "Unable to connect to backend: %s\n", soundio_strerror(err));
			return 1;
		}
		fprintf(stderr, "Backend: %s\n", soundio_backend_name(soundio->current_backend));
    	soundio_flush_events(soundio);
		int selected_device_index = soundio_default_output_device_index(soundio);
		if (selected_device_index < 0) {
			fprintf(stderr, "Output device not found\n");
			return 1;
		}
		struct SoundIoDevice *device = soundio_get_output_device(soundio, selected_device_index);
		if (!device) {
			fprintf(stderr, "out of memory\n");
			return 1;
		}
		fprintf(stderr, "Output device: %s\n", device->name);
		if (device->probe_error) {
			fprintf(stderr, "Cannot probe device: %s\n", soundio_strerror(device->probe_error));
			return 1;
		}
		struct SoundIoOutStream *outstream = soundio_outstream_create(device);
		if (!outstream) {
			fprintf(stderr, "out of memory\n");
			return 1;
		}
		outstream->write_callback = write_callback;
		std::string sound_name = (gd3Data->gameNameEnglish.size() ? gd3Data->gameNameEnglish : gd3Data->gameNameOG) + " : " + (gd3Data->trackNameEnglish.size() ? gd3Data->trackNameEnglish : gd3Data->trackNameOG);
		outstream->name = sound_name.c_str();
		outstream->sample_rate = sampleRate;
		if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {
       		outstream->format = SoundIoFormatS16NE;
		} else {
			fprintf(stderr, "No suitable device format available.\n");
			return 1;
		}
		if ((err = soundio_outstream_open(outstream))) {
			fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
			return 1;
		}
		if (outstream->layout_error)
			fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));
		if ((err = soundio_outstream_start(outstream))) {
			fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
			return 1;
		}
		static uint64_t invoked = 0;
		// while (!ended) {soundio_flush_events(soundio);}
		// std::this_thread::sleep_for(std::chrono::seconds(6));
		while (getc(stdin) != "q"[0]) {}
		soundio_outstream_destroy(outstream);
		soundio_device_unref(device);
		soundio_destroy(soundio);


	}

	return 0;
}