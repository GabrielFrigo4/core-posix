#include "wav_oss.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

static atomic_bool keep_recording = true;

static void *wait_for_enter(void *arg) {
	(void)arg;
	getchar();
	atomic_store(&keep_recording, false);
	return NULL;
}

static void update_wav_sizes(FILE *file, uint32_t pcm_bytes) {
	uint32_t riff_size = pcm_bytes + sizeof(WavHeader) - 8;
	fseek(file, 4, SEEK_SET);
	fwrite(&riff_size, sizeof(uint32_t), 1, file);
	fseek(file, 40, SEEK_SET);
	fwrite(&pcm_bytes, sizeof(uint32_t), 1, file);
}

static uint32_t capture_stream(int dsp_fd, FILE *file) {
	uint8_t buffer[AUDIO_BUFFER_SIZE];
	uint32_t total_bytes = 0;

	while (atomic_load(&keep_recording)) {
		ssize_t bytes_read = read(dsp_fd, buffer, sizeof(buffer));
		if (bytes_read <= 0) {
			break;
		}
		fwrite(buffer, 1, bytes_read, file);
		total_bytes += bytes_read;
	}

	return total_bytes;
}

int main(int argc, char **argv) {
	const char *filename = (argc > 1) ? argv[1] : "gravacao.wav";
	const char *device = (argc > 2) ? argv[2] : DEFAULT_DSP_DEVICE;

	const uint32_t sample_rate = 48000;
	const uint16_t channels = 2;
	const uint16_t bits = 16;

	int dsp_fd = oss_open_device(device, O_RDONLY, AFMT_S16_LE, channels, sample_rate);
	if (dsp_fd < 0) {
		perror("oss_open_device");
		return EXIT_FAILURE;
	}

	FILE *wav_file = fopen(filename, "wb+");
	if (!wav_file) {
		perror("fopen");
		close(dsp_fd);
		return EXIT_FAILURE;
	}

	WavHeader header = wav_header_init(sample_rate, channels, bits);
	fwrite(&header, sizeof(WavHeader), 1, wav_file);

	pthread_t thread;
	pthread_create(&thread, NULL, wait_for_enter, NULL);

	printf("Gravando em '%s'. Pressione ENTER para parar...\n", filename);
	uint32_t total_bytes = capture_stream(dsp_fd, wav_file);

	pthread_join(thread, NULL);
	close(dsp_fd);

	update_wav_sizes(wav_file, total_bytes);
	fclose(wav_file);

	printf("Salvo: %u bytes gravados.\n", total_bytes);
	return EXIT_SUCCESS;
}
