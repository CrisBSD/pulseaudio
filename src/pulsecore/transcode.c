
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <pulsecore/macro.h>
#include <pulsecore/core-format.h>


#include "transcode.h"



#ifdef HAVE_OPUS

#include <opus.h>

#define OPUS_DEFAULT_SAMPLE_RATE 48000
#define OPUS_DEFAULT_SAMPLE_SIZE 2
#define OPUS_MAX_FRAME_SIZE 2880



OpusEncoder *tc_opus_create_encoder(pa_transcode * params);
OpusDecoder *tc_opus_create_decoder(int sample_rate, int channels);
int tc_opus_encode(OpusEncoder * encoder, unsigned char *pcm_bytes, unsigned char *cbits, int channels, int frame_size);
int tc_opus_decode(OpusDecoder * decoder, unsigned char *cbits,
                   int nbBytes, unsigned char *pcm_bytes, int channels, int max_frame_size);

#endif

/* Conversion of string names to constant values */
typedef struct tc_str_to_const {
    const char *key;
    int value;
} tc_str_to_const_t;

/* Flags for the parameter table */
typedef enum tc_flags {
    TC_NONE = 0x00,
    TC_FIXED = 0x01             /* Parameter can only be set on module load */
} tc_flags_t;

/* Parameter table
 *
 * name      : name of a property of the sink. PA_PROP_COMPRESSION constants.
 * const_data: structure containing string to constant value mappings. Optional.
 * min, max  : valid range for the parameter if const_data is NULL
 */

typedef struct tc_prop_data {
    const char *name;
    const char *default_value;
    tc_str_to_const_t *const_data;
    int min;
    int max;
    tc_flags_t flags;
} tc_prop_data_t;

/*
 * tables are terminated with a NULL key with a value that will be interpreted
 * as the default.
 *
 * This table lists the know algorithms. It is checked by pa_transcode_supported,
 * so if something is listed here, it will be reported as being supported.
 */


static tc_str_to_const_t algo_map[] = {
#ifdef HAVE_OPUS
    { "opus", PA_ENCODING_OPUS    },
#endif
    { "none", PA_ENCODING_INVALID },
    { NULL  , PA_ENCODING_INVALID }
};

#ifdef HAVE_OPUS
static tc_str_to_const_t opus_application_map[] = {
    { "voip"               , OPUS_APPLICATION_VOIP                },
    { "audio"              , OPUS_APPLICATION_AUDIO               },
    { "restricted_lowdelay", OPUS_APPLICATION_RESTRICTED_LOWDELAY },
    { NULL                 , OPUS_APPLICATION_AUDIO               }
};

static tc_str_to_const_t opus_bandwidth_map[] = {
    { "narrowband"   , OPUS_BANDWIDTH_NARROWBAND    },
    { "mediumband"   , OPUS_BANDWIDTH_MEDIUMBAND    },
    { "wideband"     , OPUS_BANDWIDTH_WIDEBAND      },
    { "superwideband", OPUS_BANDWIDTH_SUPERWIDEBAND },
    { "fullband"     , OPUS_BANDWIDTH_FULLBAND      },
    { NULL           , OPUS_BANDWIDTH_FULLBAND      }
};

static tc_str_to_const_t opus_signal_map[] = {
    { "auto" , OPUS_AUTO         },
    { "voice", OPUS_SIGNAL_VOICE },
    { "music", OPUS_SIGNAL_MUSIC },
    { NULL   , OPUS_AUTO         }
};
#endif

/* This table lists all the config values, their defaults, and limits */
static tc_prop_data_t prop_data[] = {
    { PA_PROP_COMPRESSION_ALGORITHM          , "none"    , algo_map            , 0  , 0     , TC_FIXED },
    { PA_PROP_COMPRESSION_BITRATE            , "64000"   , NULL                , 500, 512000, TC_NONE  },
    { PA_PROP_COMPRESSION_FRAME_SIZE         , "2880"    , NULL                , 0  , 5000  , TC_FIXED },
#ifdef HAVE_OPUS
    { PA_PROP_COMPRESSION_OPUS_COMPLEXITY    , "10"      , NULL                , 0  , 10    , TC_NONE  },
    { PA_PROP_COMPRESSION_OPUS_MAX_BANDWIDTH , "fullband", opus_bandwidth_map  , 0  , 0     , TC_NONE  },
    { PA_PROP_COMPRESSION_OPUS_SIGNAL        , "auto"    , opus_signal_map     , 0  , 0     , TC_NONE  },
    { PA_PROP_COMPRESSION_OPUS_VBR           , "0"       , NULL                , 0  , 1     , TC_NONE  },
    { PA_PROP_COMPRESSION_OPUS_VBR_CONSTRAINT, "0"       , NULL                , 0  , 1     , TC_NONE  },
    { PA_PROP_COMPRESSION_OPUS_APPLICATION   , "audio"   , opus_application_map, 0  , 0     , TC_FIXED },
    { PA_PROP_COMPRESSION_OPUS_LSB_DEPTH     , "24"      , NULL                , 1  , 24    , TC_NONE  },
    { PA_PROP_COMPRESSION_OPUS_DTX           , "0"       , NULL                , 0  , 1     , TC_NONE  },
#endif
    { NULL                                   , NULL      , NULL                , 0  , 0     , TC_NONE  }
};


int tc_string_to_const(tc_str_to_const_t map[], const char *key);
const char *tc_const_to_string(tc_str_to_const_t map[], int value);
void tc_copy_arg(pa_modargs * ma, const char *arg, pa_proplist * p, const char *name);



int tc_string_to_const(tc_str_to_const_t map[], const char *key) {
    tc_str_to_const_t *p = &map[0];

    while (p->key != NULL) {
        if (strcmp(p->key, key) == 0)
            return p->value;

        p++;
    }

    return p->value;
}

const char *tc_const_to_string(tc_str_to_const_t map[], int value) {
    tc_str_to_const_t *p = &map[0];

    while (p->key != NULL) {
        if (p->value == value)
            return p->key;

        p++;
    }

    return NULL;
}

void tc_copy_arg(pa_modargs * ma, const char *arg, pa_proplist * p, const char *name) {
    const char *val = NULL;
    val = pa_modargs_get_value(ma, arg, NULL);

    if (val)
        pa_proplist_sets(p, name, val);
}


/*
 *  This function returns the value of a parameter, in a numeric format
 *  If it's an enum type, it gets converted to its value via the lookup table.
 *  Otherwise it returns the numeric value as-is
 */
int pa_transcode_get_param(pa_transcode * transcode, const char *key) {
    tc_prop_data_t *p = &prop_data[0];

    while (p->name) {
        if (strcmp(p->name, key) == 0) {

            const char *value = pa_proplist_gets(transcode->proplist, key);

            if (p->const_data) {
                return tc_string_to_const(p->const_data, value);
            } else {
                return atoi(value);
            }
        }
        p++;
    }

    /* This should never happen */
    pa_log_error("BUG: Parameter '%s' not found", key);
    return 0;
}


void pa_transcode_update_encoder_options(pa_transcode * transcode) {
#ifdef HAVE_OPUS
    int err;


    if (transcode->encoding == PA_ENCODING_OPUS) {
        err = opus_encoder_ctl(transcode->encoder,
                               OPUS_SET_BITRATE(pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_BITRATE)));
        if (err < 0)
            pa_log_error("failed to set bitrate: %s", opus_strerror(err));


        err = opus_encoder_ctl(transcode->encoder,
                               OPUS_SET_COMPLEXITY(pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_OPUS_COMPLEXITY)));
        if (err < 0)
            pa_log_error("failed to set complexity: %s", opus_strerror(err));


        err = opus_encoder_ctl(transcode->encoder,
                               OPUS_SET_DTX(pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_OPUS_DTX)));

        if (err < 0)
            pa_log_error("failed to set DTX: %s", opus_strerror(err));


        err = opus_encoder_ctl(transcode->encoder,
                               OPUS_SET_MAX_BANDWIDTH(pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_OPUS_MAX_BANDWIDTH)));

        if (err < 0)
            pa_log_error("failed to set max bandwidth: %s", opus_strerror(err));


        err = opus_encoder_ctl(transcode->encoder,
                               OPUS_SET_SIGNAL(pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_OPUS_SIGNAL)));
        if (err < 0)
            pa_log_error("failed to set signal: %s", opus_strerror(err));


        err = opus_encoder_ctl(transcode->encoder, OPUS_SET_VBR(pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_OPUS_VBR)));

        if (err < 0)
            pa_log_error("failed to set VBR: %s", opus_strerror(err));


        err = opus_encoder_ctl(transcode->encoder,
                               OPUS_SET_VBR_CONSTRAINT(pa_transcode_get_param
                                                     (transcode, PA_PROP_COMPRESSION_OPUS_VBR_CONSTRAINT)));
        if (err < 0)
            pa_log_error("failed to set VBR constraint: %s", opus_strerror(err));


        err = opus_encoder_ctl(transcode->encoder,
                               OPUS_SET_LSB_DEPTH(pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_OPUS_LSB_DEPTH)));

        if (err < 0)
            pa_log_error("failed to set LSB depth: %s", opus_strerror(err));

    }
#endif


}

#ifdef HAVE_OPUS
OpusDecoder *tc_opus_create_decoder(int sample_rate, int channels) {
    int err;

    OpusDecoder *decoder = opus_decoder_create(sample_rate, channels, &err);
    if (err < 0) {
        pa_log_error("failed to create decoder: %s", opus_strerror(err));
        return NULL;
    }

    return decoder;
}


OpusEncoder *tc_opus_create_encoder(pa_transcode * params) {
    int err;
    int application = pa_transcode_get_param(params,
                                             PA_PROP_COMPRESSION_OPUS_APPLICATION);


    OpusEncoder *encoder = opus_encoder_create(params->rate, params->channels, application,
                                               &err);
    if (err < 0) {
        pa_log_error("failed to create an encoder: %s", opus_strerror(err));
        return NULL;
    }

    return encoder;
}


int tc_opus_encode(OpusEncoder * encoder, unsigned char *pcm_bytes, unsigned char *cbits, int channels, int frame_size) {
    int i;
    int nbBytes;
    opus_int16 in[frame_size * channels];

    /* Convert from little-endian ordering. */
    for (i = 0; i < channels * frame_size; i++)
        in[i] = pcm_bytes[2 * i + 1] << 8 | pcm_bytes[2 * i];

    nbBytes = opus_encode(encoder, in, frame_size, cbits, frame_size * channels * 2);
    if (nbBytes < 0) {
        pa_log_error("encode failed: %s", opus_strerror(nbBytes));
        return EXIT_FAILURE;
    }

    return nbBytes;
}


int
tc_opus_decode(OpusDecoder * decoder, unsigned char *cbits, int nbBytes,
               unsigned char *pcm_bytes, int channels, int max_frame_size) {
    int err = 0, i;
    opus_int16 out[max_frame_size * channels];

    int frame_size = opus_decode(decoder, cbits, nbBytes, out, max_frame_size, 0);
    if (frame_size < 0) {
        pa_log_error("decoder failed: %s", opus_strerror(err));
        return EXIT_FAILURE;
    }

    /* Convert to little-endian ordering. */
    for (i = 0; i < channels * frame_size; i++) {
        pcm_bytes[2 * i] = out[i] & 0xFF;
        pcm_bytes[2 * i + 1] = (out[i] >> 8) & 0xFF;
    }

    return frame_size;
}

#endif



/*
 *  Copy module arguments into a property list.
 *  Actual verification is done in pa_transcode_verify_proplist
 */
void pa_transcode_copy_arguments(pa_transcode * transcode, pa_modargs * ma, pa_proplist * prop) {
    tc_copy_arg(ma, "compression", prop, PA_PROP_COMPRESSION_ALGORITHM);
    tc_copy_arg(ma, "compression-bitrate", prop, PA_PROP_COMPRESSION_BITRATE);
    tc_copy_arg(ma, "compression-frame_size", prop, PA_PROP_COMPRESSION_FRAME_SIZE);
    tc_copy_arg(ma, "compression-complexity", prop, PA_PROP_COMPRESSION_OPUS_COMPLEXITY);
    tc_copy_arg(ma, "compression-max_bandwidth", prop, PA_PROP_COMPRESSION_OPUS_MAX_BANDWIDTH);
    tc_copy_arg(ma, "compression-signal", prop, PA_PROP_COMPRESSION_OPUS_SIGNAL);
    tc_copy_arg(ma, "compression-vbr", prop, PA_PROP_COMPRESSION_OPUS_VBR);
    tc_copy_arg(ma, "compression-vbr_constraint", prop, PA_PROP_COMPRESSION_OPUS_VBR_CONSTRAINT);
    tc_copy_arg(ma, "compression-application", prop, PA_PROP_COMPRESSION_OPUS_APPLICATION);
    tc_copy_arg(ma, "compression-lsb_depth", prop, PA_PROP_COMPRESSION_OPUS_LSB_DEPTH);
    tc_copy_arg(ma, "compression-dtx", prop, PA_PROP_COMPRESSION_OPUS_DTX);

}

void pa_transcode_verify_proplist(pa_transcode * transcode) {

    tc_prop_data_t *p = &prop_data[0];
    const char *value = NULL;
    int intval = 0;
    bool found = false;
    tc_str_to_const_t *cp = NULL;

    while (p->name) {
        if (!pa_proplist_contains(transcode->proplist, p->name)) {
            pa_proplist_sets(transcode->proplist, p->name, p->default_value);

        } else {
            value = pa_proplist_gets(transcode->proplist, p->name);

            if (p->const_data) {
                /* Check if the value corresponds to one of the valid constants */
                cp = p->const_data;
                while (cp->key) {
                    if (strcmp(cp->key, value) == 0) {
                        found = true;
                        break;
                    }
                    cp++;
                }

                if (!found) {
                    /* We have the numeric default, so we look for the string name for it */
                    pa_log_error("Parameter '%s' has an invalid value '%s'. Set to default.", p->name, value);
                    pa_proplist_sets(transcode->proplist, p->name, tc_const_to_string(p->const_data, cp->value));
                }

            } else {
                intval = atoi(value);

                if (intval < p->min) {
                    pa_proplist_setf(transcode->proplist, p->name, "%i", p->min);
                    pa_log_error("Parameter '%s' out of range", p->name);
                }

                if (intval > p->max) {
                    pa_proplist_setf(transcode->proplist, p->name, "%i", p->max);
                    pa_log_error("Parameter '%s' out of range", p->name);
                }


            }
        }

        p++;
    }

}

bool pa_transcode_supported(pa_encoding_t encoding) {
    return (tc_const_to_string(algo_map, encoding) != NULL);
}


bool pa_transcode_supported_byname(const char *name) {
    return (tc_string_to_const(algo_map, name) != PA_ENCODING_INVALID);
}

pa_encoding_t pa_transcode_encoding_byname(const char *name) {
    return (pa_encoding_t) tc_string_to_const(algo_map, name);
}

void pa_transcode_set_format_info(pa_transcode * transcode, pa_format_info * f) {
    pa_format_info_set_prop_int(f, "frame_size", pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_FRAME_SIZE));
    pa_format_info_set_prop_int(f, "bitrate", pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_BITRATE));
}


void
pa_transcode_init(pa_transcode * transcode, pa_encoding_t encoding,
                  pa_transcode_flags_t flags,
                  pa_format_info * transcode_format_info,
                  pa_sample_spec * transcode_sink_spec, pa_modargs * modargs, pa_proplist * proplist) {
    int frame_size;

    pa_assert((flags & PA_TRANSCODE_DECODER)
              || (flags & PA_TRANSCODE_ENCODER));

    transcode->flags = flags;
    transcode->proplist = proplist;
    transcode->codec_mutex = pa_mutex_new(false, false);


    if (modargs) {
        // Being called for creating an encoder, we receive parameters from a command line
        pa_transcode_copy_arguments(transcode, modargs, proplist);
        pa_transcode_verify_proplist(transcode);
        transcode->encoding = pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_ALGORITHM);
    } else {
        // Being called for creating a decoder, no arguments
        transcode->encoding = encoding;
    }

    switch (transcode->encoding) {
#ifdef HAVE_OPUS
    case PA_ENCODING_OPUS:

        transcode->sample_size = OPUS_DEFAULT_SAMPLE_SIZE;


        if (flags & PA_TRANSCODE_DECODER) {
            if (pa_format_info_get_prop_int(transcode_format_info, "max_frame_size", (int *) &transcode->max_frame_size) != 0)
                transcode->max_frame_size = OPUS_MAX_FRAME_SIZE;
            if (pa_format_info_get_prop_int(transcode_format_info, "frame_size", &frame_size) != 0)
                pa_proplist_setf(transcode->proplist, PA_PROP_COMPRESSION_FRAME_SIZE, "%i", frame_size);

            pa_format_info_get_rate(transcode_format_info, &transcode->rate);
            pa_format_info_get_channels(transcode_format_info, &transcode->channels);

            transcode->decoder = tc_opus_create_decoder(transcode->rate, transcode->channels);

        } else {
            transcode->channels = transcode_sink_spec->channels;
            transcode->rate = OPUS_DEFAULT_SAMPLE_RATE;
            transcode->encoder = tc_opus_create_encoder(transcode);


            transcode_sink_spec->rate = transcode->rate;
            transcode_sink_spec->format = PA_SAMPLE_S16LE;

            pa_transcode_update_encoder_options(transcode);

        }

        break;
#endif
    default:
        transcode->decoder = NULL;
        transcode->encoder = NULL;
    }

}

void pa_transcode_free(pa_transcode * transcode) {

    switch (transcode->encoding) {
#ifdef HAVE_OPUS
    case PA_ENCODING_OPUS:
        opus_decoder_destroy(transcode->decoder);
        break;
#endif
    default:
        transcode->decoder = NULL;
    }

}


int32_t pa_transcode_encode(pa_transcode * transcode, unsigned char *pcm_input, unsigned char **compressed_output) {
    int nbBytes = 0;
    int frame_size = pa_transcode_get_param(transcode, PA_PROP_COMPRESSION_FRAME_SIZE);

    pa_mutex_lock(transcode->codec_mutex);
    switch (transcode->encoding) {
#ifdef HAVE_OPUS
    case PA_ENCODING_OPUS:
        *compressed_output = malloc(frame_size * transcode->channels * transcode->sample_size);
        nbBytes = tc_opus_encode(transcode->encoder, pcm_input, *compressed_output, transcode->channels, frame_size);
        break;
#endif

    }

    pa_mutex_unlock(transcode->codec_mutex);

    return nbBytes;
}

int32_t
pa_transcode_decode(pa_transcode * transcode, unsigned char *compressed_input, int input_length, unsigned char *pcm_output) {
    int32_t frame_length = 0;

    pa_mutex_lock(transcode->codec_mutex);

    switch (transcode->encoding) {
#ifdef HAVE_OPUS
    case PA_ENCODING_OPUS:
        frame_length =
            tc_opus_decode(transcode->decoder, compressed_input,
                           input_length, pcm_output, transcode->channels, transcode->max_frame_size);
        break;
#endif

    }

    pa_mutex_unlock(transcode->codec_mutex);

    return frame_length;
}
