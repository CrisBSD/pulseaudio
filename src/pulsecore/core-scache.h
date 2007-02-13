#ifndef foocorescachehfoo
#define foocorescachehfoo

/* $Id$ */

/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering
  Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <pulsecore/core.h>
#include <pulsecore/memchunk.h>
#include <pulsecore/sink.h>

#define PA_SCACHE_ENTRY_SIZE_MAX (1024*1024*2)

typedef struct pa_scache_entry {
    pa_core *core;
    uint32_t index;
    char *name;

    pa_cvolume volume;
    pa_sample_spec sample_spec;
    pa_channel_map channel_map;
    pa_memchunk memchunk;

    char *filename;

    int lazy;
    time_t last_used_time;
} pa_scache_entry;

int pa_scache_add_item(pa_core *c, const char *name, const pa_sample_spec *ss, const pa_channel_map *map, const pa_memchunk *chunk, uint32_t *idx);
int pa_scache_add_file(pa_core *c, const char *name, const char *filename, uint32_t *idx);
int pa_scache_add_file_lazy(pa_core *c, const char *name, const char *filename, uint32_t *idx);

int pa_scache_add_directory_lazy(pa_core *c, const char *pathname);

int pa_scache_remove_item(pa_core *c, const char *name);
int pa_scache_play_item(pa_core *c, const char *name, pa_sink *sink, pa_volume_t volume);
void pa_scache_free(pa_core *c);

const char *pa_scache_get_name_by_id(pa_core *c, uint32_t id);
uint32_t pa_scache_get_id_by_name(pa_core *c, const char *name);

uint32_t pa_scache_total_size(pa_core *c);

void pa_scache_unload_unused(pa_core *c);

#endif
