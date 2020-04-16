/*
 * Copyright 2014-2020 Real Logic Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <inttypes.h>

#include "aeronc.h"
#include "aeron_common.h"
#include "aeron_exclusive_publication.h"
#include "aeron_alloc.h"
#include "util/aeron_error.h"
#include "util/aeron_fileutil.h"
#include "concurrent/aeron_counters_manager.h"
#include "concurrent/aeron_term_appender.h"

int aeron_exclusive_publication_create(
    aeron_exclusive_publication_t **publication,
    aeron_client_conductor_t *conductor,
    const char *channel,
    int32_t stream_id,
    int32_t session_id,
    int32_t position_limit_id,
    int32_t channel_status_id,
    const char *log_file,
    int64_t original_registration_id,
    int64_t registration_id,
    bool pre_touch)
{
    aeron_exclusive_publication_t *_publication;

    *publication = NULL;
    if (aeron_alloc((void **)&_publication, sizeof(aeron_exclusive_publication_t)) < 0)
    {
        int errcode = errno;

        aeron_set_err(errcode, "aeron_exclusive_publication_create (%d): %s", errcode, strerror(errcode));
        return -1;
    }

    if (aeron_map_existing_log(&_publication->mapped_raw_log, log_file, pre_touch) < 0)
    {
        aeron_free(_publication);
        return -1;
    }

    _publication->log_meta_data = (aeron_logbuffer_metadata_t *)_publication->mapped_raw_log.log_meta_data.addr;

    _publication->conductor = conductor;
    _publication->channel = channel;
    _publication->registration_id = registration_id;
    _publication->original_registration_id = original_registration_id;
    _publication->stream_id = stream_id;
    _publication->session_id = session_id;
    _publication->is_closed = false;

    *publication = _publication;
    return -1;
}

int aeron_exclusive_publication_delete(aeron_exclusive_publication_t *publication)
{
    aeron_map_raw_log_close(&publication->mapped_raw_log, NULL);
    aeron_free((void *)publication->channel);
    aeron_free(publication);

    return 0;
}

int aeron_exclusive_publication_close(aeron_exclusive_publication_t *publication)
{
    return NULL != publication ?
        aeron_client_conductor_async_close_exclusive_publication(publication->conductor, publication) : 0;
}
