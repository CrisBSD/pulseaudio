#ifndef footranscodehfoo
#define footranscodehfoo

#include <pulsecore/core-format.h>
#include <pulsecore/modargs.h>
#include <pulsecore/proplist-util.h>
#include <pulsecore/mutex.h>

/*
 * Bitrate and frame size are used externally, in module-tunnel-sink-new
 * We consider these the universal parameters that are going to apply to
 * any other codec that might be implemented in the future.
 */
#define PA_PROP_COMPRESSION_ALGORITHM           "compression.algorithm"
#define PA_PROP_COMPRESSION_BITRATE             "compression.bitrate"
#define PA_PROP_COMPRESSION_FRAME_SIZE          "compression.frame_size"

/* The rest of the parameters are Opus specific. */
#define PA_PROP_COMPRESSION_OPUS_COMPLEXITY     "compression.opus.complexity"
#define PA_PROP_COMPRESSION_OPUS_MAX_BANDWIDTH  "compression.opus.max_bandwidth"
#define PA_PROP_COMPRESSION_OPUS_SIGNAL         "compression.opus.signal"
#define PA_PROP_COMPRESSION_OPUS_VBR            "compression.opus.vbr"
#define PA_PROP_COMPRESSION_OPUS_VBR_CONSTRAINT "compression.opus.vbr_constraint"
#define PA_PROP_COMPRESSION_OPUS_APPLICATION    "compression.opus.application"
#define PA_PROP_COMPRESSION_OPUS_LSB_DEPTH      "compression.opus.lsb_depth"
#define PA_PROP_COMPRESSION_OPUS_DTX            "compression.opus.dtx"

typedef enum pa_transcode_flags {
    PA_TRANSCODE_DECODER = (1 << 0),
    PA_TRANSCODE_ENCODER = (1 << 1)
} pa_transcode_flags_t;

typedef struct pa_transcode {
    int32_t encoding;
    uint8_t channels;
    uint32_t max_frame_size;
    uint32_t sample_size;
    uint32_t rate;

    pa_proplist *proplist;

    pa_transcode_flags_t flags;
    pa_mutex *codec_mutex;

    union {
        void *decoder;
        void *encoder;
    };
} pa_transcode;

void pa_transcode_copy_arguments(pa_transcode * transcode, pa_modargs * ma, pa_proplist * prop);
void pa_transcode_verify_proplist(pa_transcode * transcode);
void pa_transcode_update_encoder_options(pa_transcode * transcode);
int pa_transcode_get_param(pa_transcode * transcode, const char *key);

bool pa_transcode_supported(pa_encoding_t encoding);
bool pa_transcode_supported_byname(const char *name);
pa_encoding_t pa_transcode_encoding_byname(const char *name);


void pa_transcode_set_format_info(pa_transcode * transcode, pa_format_info * f);
void pa_transcode_init(pa_transcode * transcode, pa_encoding_t encoding, pa_transcode_flags_t flags,
                       pa_format_info * transcode_format_info, pa_sample_spec * transcode_sink_spec, pa_modargs * modargs,
                       pa_proplist * proplist);
void pa_transcode_free(pa_transcode * transcode);
int32_t pa_transcode_encode(pa_transcode * transcode, unsigned char *pcm_input, unsigned char **compressed_output);
int32_t pa_transcode_decode(pa_transcode * transcode, unsigned char *compressed_input, int input_length,
                            unsigned char *pcm_output);

#endif
