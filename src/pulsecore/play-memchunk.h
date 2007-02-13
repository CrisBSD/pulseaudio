#ifndef fooplaychunkhfoo
#define fooplaychunkhfoo

/* $Id$ */

/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering

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

#include <pulsecore/sink.h>
#include <pulsecore/memchunk.h>

int pa_play_memchunk(
    pa_sink *sink,
    const char *name,
    const pa_sample_spec *ss,
    const pa_channel_map *map,
    const pa_memchunk *chunk,
    pa_cvolume *cvolume);

#endif
