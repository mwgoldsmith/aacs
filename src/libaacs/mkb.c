/*
 * This file is part of libaacs
 * Copyright (C) 2009-2010  Obliter0n
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "mkb.h"
#include "file/file.h"
#include "util/macro.h"
#include "util/logging.h"
#include "util/strutl.h"

#include <stdio.h>

struct mkb {
    size_t size;    // file size
    uint8_t *buf;   // file contents
};

static const uint8_t *_record(MKB *mkb, uint8_t type, size_t *rec_len)
{
    size_t pos = 0, len = 0;

    while (pos + 4 <= mkb->size) {
        len = MKINT_BE24(mkb->buf + pos + 1);

        if (rec_len) {
            *rec_len = len;
        }

        if (mkb->buf[pos] == type) {
            DEBUG(DBG_MKB, "Retrieved MKB record 0x%02x (%p)\n", type,
                  mkb->buf + pos);

            return mkb->buf + pos;
        }

        pos += len;
    }

    return NULL;
}

MKB *mkb_open(const char *path)
{
    AACS_FILE_H *fp = NULL;
    char   *f_name;
    MKB *mkb = malloc(sizeof(MKB));

    f_name = str_printf("%s/AACS/MKB_RO.inf", path);

    DEBUG(DBG_MKB, "Opening MKB %s... (%p)\n", f_name, mkb);
    fp = file_open(f_name, "rb");

    X_FREE(f_name);

    if (fp) {
        file_seek(fp, 0, SEEK_END);
        mkb->size = file_tell(fp);
        file_seek(fp, 0, SEEK_SET);

        mkb->buf = malloc(mkb->size);

        file_read(fp, mkb->buf, mkb->size);

        DEBUG(DBG_MKB, "MKB size: %zd (%p)\n", mkb->size, mkb);
        DEBUG(DBG_MKB, "MKB version: %d (%p)\n", mkb_version(mkb), mkb);

        file_close(fp);
        return mkb;
    }

    DEBUG(DBG_MKB, "Error opening MKB! (%p)\n", mkb);

    return NULL;
}

MKB *mkb_init(uint8_t *data, int len)
{
    MKB *mkb = malloc(sizeof(MKB));

    mkb->size = len;
    mkb->buf  = data;

    return mkb;
}

void mkb_close(MKB *mkb)
{
    if (mkb) {
        X_FREE(mkb->buf);
        X_FREE(mkb);
    }
}

const uint8_t *mkb_data(MKB *mkb)
{
    return mkb->buf;
}

size_t mkb_data_size(MKB *mkb)
{
    size_t pos = 0;

    while (pos + 4 <= mkb->size) {
        if (!mkb->buf[pos]) {
            break;
        }
        pos += MKINT_BE24(mkb->buf + pos + 1);
    }

    return pos;
}


uint8_t mkb_type(MKB *mkb)
{
    const uint8_t *rec = _record(mkb, 0x10, NULL);

    return MKINT_BE32(rec + 4);
}

uint32_t mkb_version(MKB *mkb)
{
    const uint8_t *rec = _record(mkb, 0x10, NULL);

    return MKINT_BE32(rec + 8);
}

const uint8_t *mkb_type_and_version_record(MKB *mkb)
{
    const uint8_t *rec = _record(mkb, 0x10, NULL);

    return rec;
}


const uint8_t *mkb_host_revokation_entries(MKB *mkb, size_t *len)
{
    const uint8_t *rec = _record(mkb, 0x21, len);
    *len -= 4;

    return rec + 4;
}

const uint8_t *mkb_drive_revokation_entries(MKB *mkb, size_t *len)
{
    const uint8_t *rec = _record(mkb, 0x20, len);
    *len -= 4;

    return rec + 4;
}

const uint8_t *mkb_subdiff_records(MKB *mkb, size_t *len)
{
    const uint8_t *rec = _record(mkb, 0x04, len) + 4;
    *len -= 4;

    return rec;
}

const uint8_t *mkb_cvalues(MKB *mkb, size_t *len)
{
    const uint8_t *rec = _record(mkb, 0x05, len) + 4;
    *len -= 4;

    return rec;
}

const uint8_t *mkb_mk_dv(MKB *mkb)
{
    return _record(mkb, 0x81, NULL) + 4;
}

const uint8_t *mkb_signature(MKB *mkb, size_t *len)
{
    const uint8_t *rec = _record(mkb, 0x02, len);
    *len -= 4;

    return rec + 4;

}
