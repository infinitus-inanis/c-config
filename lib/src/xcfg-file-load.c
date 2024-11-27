#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-file.h"
#include "xcfg-impl.h"
#include "xcfg-data.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <memory.h>
#include <limits.h>

#include <ctype.h>

#define load_logi(path, line, fmt, args...) \
  logi("[loading:%s:%u] " fmt, (path), (xcfg_u32)(line), ## args)

xcfg_ret
xcfg_load_file(xcfg *cfg, const char *path)
{
  FILE *stream;

  char   rbuf[LINE_MAX];
  size_t rlen = sizeof(rbuf) - 1;
  size_t line = 0;

  char *t0, *t1;

  xcfg_tree *tree = &cfg->tree;

  xcfg_ret xret = XCFG_RET_FAILRUE;

  if (!path || !path[0])
    return XCFG_RET_INVALID;

  stream = fopen(path, "r");
  if (!stream)
    return XCFG_RET_INVALID;

  memset(rbuf, 0, sizeof rbuf);
  line = 0;

  fseek(stream, 0, SEEK_SET);
  { /* load header */
    xcfg_u32 type_size;

    if (line++, !fgets(rbuf, rlen, stream))
      goto out;

    t0 = rbuf;
    if (strncmp(t0, "0x", 2))
      goto out;
    t0 += 2;

    type_size = strtoul(t0, NULL, 16);
    if (type_size != tree->rtti->size) {
      load_logi(path, line, "type size is invalid: %u != %u", type_size, tree->rtti->size);
      goto out;
    }
    load_logi(path, line, "type size: %u", type_size);

    memset(rbuf, 0, sizeof rbuf);
    if (line++, !fgets(rbuf, rlen, stream))
      goto out;

    t0 = strchr(rbuf, '(');
    t1 = strchr(rbuf, ')');
    if (!t0 || !t1 || t0 > t1)
      goto out;

    t0++;
    while (t0 < t1 && isspace(*t0)) ++t0;
    if (t0 == t1) goto out;
    t1--;
    while (t1 > t0 && isspace(*t1)) --t1;
    if (t0 == t1) goto out;
    *(t1 + 1) = '\0';
    /* here `t0` is a valid type name string */

    if (strcmp(t0, tree->rtti->name)) {
      load_logi(path, line, "type name is invalid: '%s' != '%s'", t0, tree->rtti->name);
      goto out;
    }
    load_logi(path, line, "type name: %s", t0);
  }

out:
  fclose(stream);
  return xret;
}