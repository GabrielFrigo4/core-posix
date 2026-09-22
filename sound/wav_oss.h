#ifndef WAV_OSS_H
#define WAV_OSS_H

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>

#define DEFAULT_DSP_DEVICE "/dev/dsp"
#define AUDIO_BUFFER_SIZE 4096

#pragma pack(push, 1)
typedef struct
{
	char riff_id[4];
	uint32_t riff_size;
	char wave_id[4];
	char fmt_id[4];
	uint32_t fmt_size;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	char data_id[4];
	uint32_t data_size;
} WavHeader;
#pragma pack(pop)

static inline WavHeader wav_header_init(uint32_t sample_rate, uint16_t channels, uint16_t bits)
{
	uint16_t bytes_per_sample = bits / 8;
	return (WavHeader){.riff_id = {'R', 'I', 'F', 'F'},
	                   .riff_size = 0,
	                   .wave_id = {'W', 'A', 'V', 'E'},
	                   .fmt_id = {'f', 'm', 't', ' '},
	                   .fmt_size = 16,
	                   .audio_format = 1,
	                   .num_channels = channels,
	                   .sample_rate = sample_rate,
	                   .byte_rate = sample_rate * channels * bytes_per_sample,
	                   .block_align = channels * bytes_per_sample,
	                   .bits_per_sample = bits,
	                   .data_id = {'d', 'a', 't', 'a'},
	                   .data_size = 0};
}

static inline int oss_open_device(
    const char *device, int mode, int format, int channels, int speed
)
{
	int fd = open(device, mode);
	if (fd < 0)
	{
		return -1;
	}

	if (ioctl(fd, SNDCTL_DSP_SETFMT, &format) == -1 ||
	    ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) == -1 ||
	    ioctl(fd, SNDCTL_DSP_SPEED, &speed) == -1)
	{
		close(fd);
		return -1;
	}

	return fd;
}

static inline int wav_bits_to_oss_format(uint16_t bits)
{
	switch (bits)
	{
	case 8:
		return AFMT_U8;
	case 16:
		return AFMT_S16_LE;
	case 32:
		return AFMT_S32_LE;
	default:
		return -1;
	}
}

#endif
