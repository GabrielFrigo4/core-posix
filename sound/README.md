# 🔊 oss-audio

> Utilitários minimalistas em C puro para captura e reprodução de áudio digital via API nativa do OSS (Open Sound System) no FreeBSD, sem intermediários e com dependência zero de bibliotecas externas.

---

## 1. Como Funciona a Arquitetura

O `oss-audio` opera tratando dispositivos de som sob a filosofia Unix fundamental: **áudio é apenas um fluxo sequencial de bytes em um arquivo de dispositivo** (`/dev/dsp`).

```
[Microfone / ADC]
│
▼
/dev/dsp  (Kernel OSS)
│  read() via ring buffer
▼
[recorder.c] ──── Thread CLI (Monitora tecla ENTER)
│
▼  fwrite()
[gravacao.wav] (Cabeçalho RIFF de 44 bytes + PCM linear bruto)
│
▼  fread()
[player.c]  (Validação canônica do cabeçalho)
│
▼  write() sincronizado com clock do DAC
/dev/dsp  (Kernel OSS Mixer / VCHANs)
│
▼
[Fone / Saída DAC]
```

### Decisões de Design

- **Zero Dependências Externas:** O código utiliza estritamente chamadas de sistema POSIX (`open`, `read`, `write`, `close`), a biblioteca de threads padrão (`libpthread`) e `sys/soundcard.h`. Nenhuma biblioteca de alto nível como SDL, ALSA, PulseAudio ou PipeWire é necessária.
- **Isolamento DRY em Header Único (`wav_oss.h`):** Toda a definição binária do contêiner RIFF/WAVE e as rotinas de configuração via `ioctl` de baixo nível residem em um único cabeçalho compartilhado entre o gravador e o reprodutor.
- **Mixagem Nativa via Kernel:** A concorrência entre fluxos e programas é delegada integralmente ao subsistema de canais virtuais (**VCHANs**) do FreeBSD.
- **Clean Code Estrito:** O código dispensa comentários expositivos, utilizando nomes expressivos e rotinas de responsabilidade única.

---

## 2. O Formato de Áudio e o Mínimo Viável (~44 Bytes)

Um arquivo `.wav` nada mais é do que os mesmos bytes brutos gravados da placa com um **cabeçalho de 44 bytes** prepensado aos dados:

```c
#pragma pack(push, 1)
typedef struct {
    char     riff_id[4];        /* "RIFF" */
    uint32_t riff_size;         /* Tamanho total do arquivo - 8 */
    char     wave_id[4];        /* "WAVE" */
    char     fmt_id[4];         /* "fmt " */
    uint32_t fmt_size;          /* 16 para PCM linear */
    uint16_t audio_format;      /* 1 = PCM sem compressão */
    uint16_t num_channels;      /* 1 = Mono, 2 = Estéreo */
    uint32_t sample_rate;       /* Frequência em Hz (ex: 48000) */
    uint32_t byte_rate;         /* sample_rate * channels * (bits / 8) */
    uint16_t block_align;       /* channels * (bits / 8) */
    uint16_t bits_per_sample;   /* Profundidade (16 bits) */
    char     data_id[4];        /* "data" */
    uint32_t data_size;         /* Contagem de bytes PCM brutos */
} WavHeader;
#pragma pack(pop)
```

Ao abrir o arquivo para reprodução:

1. O programa valida os marcadores literais (`RIFF`, `WAVE`, `fmt ` e `data`).
2. Configura a placa física no kernel via `ioctl(fd, SNDCTL_DSP_*, ...)` com a taxa e os canais exatos lidos do cabeçalho.
3. Transmite o restante dos bytes do fluxo diretamente para o descritor de arquivo.

---

## 3. Fundamentos do Subsistema de Som do FreeBSD

### Semântica de Bloqueio de I/O de Áudio

Diferente de sistemas com servidores em userland que exigem loops de eventos assíncronos e callbacks complexos, a chamada `write()` no descritor `/dev/dsp` bloqueia naturalmente quando o buffer DMA do hardware atinge a capacidade. O próprio clock físico do DAC dita a taxa de consumo da thread.

### Canais Virtuais (VCHANs)

O kernel do FreeBSD possui um mixer em software integrado:

- A sysctl `dev.pcm.X.play.vchans` controla a clonagem dinâmica de descritores de som.
- Quando habilitado (`vchans: 1`), múltiplas instâncias de `player`, navegadores e ferramentas de terminal podem abrir `/dev/dsp` concomitantemente sem gerar erros de `EBUSY` (_Device busy_).
- O kernel soma as matrizes de inteiros das amostras no ring buffer do mixer mestre e envia o sinal consolidado à controladora física.
