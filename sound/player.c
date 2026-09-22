#include "wav_oss.h"
#include <stdbool.h>
#include <stdlib.h>

static bool is_valid_pcm_wav(const WavHeader *header) {
	return memcmp(header->riff_id, "RIFF", 4) == 0 &&
	memcmp(header->wave_id, "WAVE", 4) == 0 &&
	memcmp(header->fmt_id, "fmt ", 4) == 0 &&
	header->audio_format == 1 &&
	memcmp(header->data_id, "data", 4) == 0;
}

static void play_stream(FILE *file, int dsp_fd, uint32_t total_bytes) {
	uint8_t buffer[AUDIO_BUFFER_SIZE];
	uint32_t remaining = total_bytes;

	while (remaining > 0) {
		size_t chunk = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
		size_t bytes_read = fread(buffer, 1, chunk, file);
		if (bytes_read == 0) {
			break;
		}

		ssize_t bytes_written = write(dsp_fd, buffer, bytes_read);
		if (bytes_written <= 0) {
			break;
		}
		remaining -= bytes_written;
	}

	ioctl(dsp_fd, SNDCTL_DSP_SYNC, NULL);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Uso: %s <arquivo.wav> [dispositivo]\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *filename = argv[1];
	const char *device = (argc > 2) ? argv[2] : DEFAULT_DSP_DEVICE;

	FILE *wav_file = fopen(filename, "rb");
	if (!wav_file) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	WavHeader header;
	if (fread(&header, sizeof(WavHeader), 1, wav_file) != 1 || !is_valid_pcm_wav(&header)) {
		fprintf(stderr, "Erro: Arquivo WAV invalido ou formato incompativel.\n");
		fclose(wav_file);
		return EXIT_FAILURE;
	}

	int oss_format = wav_bits_to_oss_format(header.bits_per_sample);
	if (oss_format < 0) {
		fprintf(stderr, "Erro: Formato de %u bits nao suportado.\n", header.bits_per_sample);
		fclose(wav_file);
		return EXIT_FAILURE;
	}

	int dsp_fd = oss_open_device(device, O_WRONLY, oss_format, header.num_channels, header.sample_rate);
	if (dsp_fd < 0) {
		perror("oss_open_device");
		fclose(wav_file);
		return EXIT_FAILURE;
	}

	printf("Tocando '%s' (%u Hz, %u bits, %u canais)...\n",
		   filename, header.sample_rate, header.bits_per_sample, header.num_channels);

	play_stream(wav_file, dsp_fd, header.data_size);

	close(dsp_fd);
	fclose(wav_file);
	return EXIT_SUCCESS;
}
