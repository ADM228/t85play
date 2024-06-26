#include "t85playdefs.cpp"

#include <iostream>

#include "BitConverter.cpp"

static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
	std::cout << "\033[2K\033[0G";
	printf("Playing: %5.2f%%", (float)((uint32_t)regDumpFile.tellg() - regDataLocation) / regDataLength * 100.0);
	fflush(stdout);
    struct SoundIoChannelArea *areas;
    int err;
	// if (!totalSmpCount) {ended = true; return;}
    int frames_left = frame_count_max;
	// std::cout << frames_left << " " << totalSmpCount << " " << frame_count_max << std::endl;
    for (;;) {
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frames_left))) {
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }
		int frame_count = frames_left;

        if (!frame_count)
            break;
		
        const struct SoundIoChannelLayout *layout = &outstream->layout;
		
		for (uint32_t frame = frame_count; frame != 0; frame--) {
			if (totalSmpCount) {
				sampleTickCounter += ticksPerSample;
				while (sampleTickCounter >= 1.0) {
					sampleTickCounter -= 1.0;
					emulationTick(regDumpFile);	// TODO: this is bs, just do a buffer
				}
			}
			// std::cout << sampleTickCounter << "  " << regWrites.size() << std::endl;
			if (!apu.shiftRegisterPending() && regWrites.size()) {
				apu.writeReg(regWrites.front()&0xFF, regWrites.front()>>8);
				// std::cout << frames_left << " - WR: " << std::hex << (regWrites.front()>>8) << "->" << (regWrites.front()&0xFF) << std::dec << std::endl;
				regWrites.pop_front();
			} 
			static float mult = 1.0;
			if (fading) mult = (float)totalSmpCount / fadeTime;
			int16_t sample = apu.calcS16() * mult;
			if (notchFilterEnabled) sample = filterSingle(notchFilter, sample);
			for (int channel = 0; channel < layout->channel_count; channel++) {
				BitConverter::writeBytes(areas[channel].ptr, (uint16_t)sample);
				areas[channel].ptr += areas[channel].step;
			}
		}
        if ((err = soundio_outstream_end_write(outstream))) {
            if (err == SoundIoErrorUnderflow)
                continue;
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }
		frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }
}

void underflow_callback(struct SoundIoOutStream *outstream) {
	static int count = 0;
	std::cerr << "Underflow #" << count++ << std::endl;
}
