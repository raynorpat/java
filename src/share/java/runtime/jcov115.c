/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Reconstructed from Sun 1.1.5 javai_g.dll 10039D90--1003C3xx.
 * See docs/superpowers/plans/2026-11-06-jdk115-jcov-evidence.md. */
#include "tree.h"
#include "interpreter.h"
#include "sys_api.h"
#include "monitor.h"
#include "jcov115.h"
#ifdef JCOV
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "io_md.h"

/* File output uses CRT stdio; only diagnostics go through the VM hook. */
#undef fprintf

char *cov_file;
char *testname;

static struct covtable *coverage_last(JavaFrame *frame, unsigned char *pc)
{
    struct methodblock *mb = frame->current_method;
    unsigned long *index, entry;
    if (!cov_file || !mb || !mb->coverage_table) return NULL;
    index = (unsigned long *)(mb->coverage_table + mb->coverage_table_length);
    entry = index[pc - mb->code];
    return entry ? mb->coverage_table + entry - 1 : NULL;
}

void coverage_instruction(JavaFrame *frame, unsigned char *pc)
{
    struct covtable *entry = coverage_last(frame, pc);
    unsigned long offset;
    if (!entry) return;
    offset = entry->pc;
    for (;;) {
        if (entry->type < CT_CASE) ++entry->count;
        if (entry == frame->current_method->coverage_table) break;
        if ((--entry)->pc != offset) break;
    }
}

void coverage_if(int taken, JavaFrame *frame, unsigned char *pc)
{
    struct covtable *entry = coverage_last(frame, pc);
    /* deviation: malformed optional coverage metadata must not write before its slab. */
    if (entry && (!taken || entry > frame->current_method->coverage_table))
        ++(entry - (taken ? 1 : 0))->count;
}

void coverage_lookupswitch(JavaFrame *frame, unsigned char *pc, int npairs, int nodef)
{
    struct covtable *entry = coverage_last(frame, pc);
    int without_default;
    if (!entry) return;
    without_default = entry->type == CT_SWITH_WO_DEF;
    if (without_default) {
        if (entry == frame->current_method->coverage_table) return;
        --entry;
    }
    if (!nodef && npairs >= 0 && npairs <= entry - frame->current_method->coverage_table)
        ++(entry - npairs)->count;
    else if (without_default) ++(entry + 1)->count;
}

void coverage_switch(JavaFrame *frame, unsigned char *pc, int key, int low, int high)
{
    struct covtable *entry = coverage_last(frame, pc), *selected;
    int without_default;
    if (!entry) return;
    without_default = entry->type == CT_SWITH_WO_DEF;
    if (without_default) {
        if (entry == frame->current_method->coverage_table) return;
        --entry;
    }
    if (key >= 0 && key <= high - low) {
        if (high - low - key > entry - frame->current_method->coverage_table) return;
        selected = entry + low - high + key;
        if (selected->where) ++selected->count;
        else if (without_default) ++(entry + 1)->count;
    } else if (without_default) ++(entry + 1)->count;
}

static const char cov_header[] = "JCOV-DATA-FILE-VERSION: 1.0\n";
static const char columns[] = "#flag\tline\tposition";
static const char count_columns[] = "#flag\tline\tposition\tcount\n";
static const char test_columns[] = "#flag\tline\tposition\tcount\ttestcount\tscale\n";

static int read_line(FILE *input, char *line, int *number)
{
    ++*number;
    return fgets(line, 1023, input) != NULL;
}

static void trim_line(char *line)
{
    size_t length = strlen(line);
    if (length && line[length - 1] == '\n') line[length - 1] = 0;
}

static void write_scale(FILE *output, int tests, int covered)
{
    int i;
    fputc('#', output);
    for (i = 0; i < (tests - 1) / 4; ++i) fputc('0', output);
    fputc(covered ? "1842"[tests % 4] : '0', output);
}

static void write_class(FILE *output, ClassClass *cb, int tests)
{
    unsigned int m;
    unsigned long i;
    struct methodblock *mb;
    struct covtable *entry;
    int heading = 0;
    char timestamp[32], *p;
    for (m = 0, mb = cbMethods(cb); m < cbMethodsCount(cb); ++m, ++mb) {
        if ((mb->fb.access & ACC_NATIVE) || !mb->coverage_table) continue;
        if (!heading) {
            fputs("CLASS: ", output);
            /* deviation: print dotted names without mutating VM class identity. */
            for (p = cbName(cb); *p; ++p) fputc(*p == '/' ? '.' : *p, output);
            fprintf(output, "\nSRCFILE: %s\n", cbAbsoluteSourceName(cb) ? cbAbsoluteSourceName(cb) : "");
            ll2str(cbTimestamp(cb), timestamp, timestamp + sizeof(timestamp));
            fprintf(output, "TIMESTAMP: %s\n", timestamp);
            fputs(testname ? test_columns : count_columns, output);
            heading = 1;
        }
        for (i = 0, entry = mb->coverage_table; i < mb->coverage_table_length; ++i, ++entry) {
            if (entry->type == CT_CASE && !entry->where) continue;
            fprintf(output, "%lu\t%lu\t%lu\t\t%ld", entry->type,
                    entry->where >> 18, entry->where & 0x3ffff, (long)entry->count);
            if (testname) {
                fprintf(output, "\t%d\t\t", entry->count != 0);
                write_scale(output, tests, entry->count != 0);
            }
            fputc('\n', output);
        }
    }
}

/* Match primary class/source/timestamp identity without altering input rows. */
static int find_coverage_class(char *name, char *source, char *timestamp, unsigned char *used)
{
    int i;
    char value[32], *p;
    ClassClass *cb;
    for (p = name; *p; ++p) if (*p == '.') *p = '/';
    for (i = 0; i < nbinclasses; ++i) {
        cb = binclasses[i];
        if (!cb || used[i] || cbName(cb)[0] == '[' || strcmp(cbName(cb), name)) continue;
        if (!cbAbsoluteSourceName(cb) ||
            strncmp(source, cbAbsoluteSourceName(cb), strlen(cbAbsoluteSourceName(cb)))) continue;
        ll2str(cbTimestamp(cb), value, value + sizeof(value));
        if (!strcmp(timestamp, value)) return i;
    }
    return -1;
}

/* Return the original source diagnostic line for malformed input, or zero.
 * The caller preserves the existing file until a complete merge succeeds. */
static int merge_file(FILE *input, FILE *output, unsigned char *used, int *tests, int *number)
{
    char line[1024], name[1024], source[1024], timestamp[1024], scale[1024];
    int found, format, extended, have_line, flag, lineno, position, prior, runs;
    unsigned int m;
    unsigned long i;
    struct methodblock *mb;
    struct covtable *entry;
    size_t length;
    for (;;) {
        if (!read_line(input, line, number)) return 489;
        if (!strncmp(line, "CLASS: ", 7)) break;
        if (!strncmp(line, "TEST ", 5)) ++*tests;
        fputs(line, output);
    }
    if (testname) fprintf(output, "TEST %d: %s\n", ++*tests, testname);
    for (;;) {
        if (strncmp(line, "CLASS: ", 7)) return 515;
        fputs(line, output);
        strcpy(name, line + 7); trim_line(name);
        if (!read_line(input, line, number)) return 526;
        if (strncmp(line, "SRCFILE: ", 9)) return 527;
        strcpy(source, line + 9); fputs(line, output);
        if (!read_line(input, line, number)) return 531;
        if (strncmp(line, "TIMESTAMP: ", 11)) return 532;
        strcpy(timestamp, line + 11); trim_line(timestamp); fputs(line, output);
        found = find_coverage_class(name, source, timestamp, used);
        if (found < 0) {
            if (!read_line(input, line, number)) return 565;
            if (strncmp(line, columns, strlen(columns))) return 566;
            format = strlen(line) == strlen("#flag\tline\tposition\tcount");
            fputs(testname ? test_columns : line, output);
            while ((have_line = read_line(input, line, number)) != 0) {
                if (!strncmp(line, "CLASS: ", 7)) break;
                if (!testname) fputs(line, output);
                else {
                    trim_line(line);
                    fputs(line, output);
                    if (strchr(line, '#')) {
                        if (*tests % 4 == 1) fputc('0', output);
                    } else if (format) fputs("\t0\t\t#0", output);
                    else {
                        fputs("\t\t0\t0\t\t#", output);
                        for (i = 0; i < (unsigned long)((*tests + 3) / 4); ++i) fputc('0', output);
                    }
                    fputc('\n', output);
                }
            }
            if (!have_line) return 0;
            continue;
        }
        used[found] = 1;
        if (!read_line(input, line, number)) return 604;
        if (!strncmp(line, test_columns, strlen(test_columns) - 1)) format = 3;
        else if (!strncmp(line, count_columns, strlen(count_columns) - 1)) format = 2;
        else if (!strncmp(line, columns, strlen(columns))) format = 1;
        else return 615;
        extended = format == 3 || *tests > 0;
        fputs(extended ? test_columns : count_columns, output);
        for (m = 0, mb = cbMethods(binclasses[found]); m < cbMethodsCount(binclasses[found]); ++m, ++mb) {
            if ((mb->fb.access & ACC_NATIVE) || !mb->coverage_table) continue;
            for (i = 0, entry = mb->coverage_table; i < mb->coverage_table_length; ++i, ++entry) {
                if (entry->type == CT_CASE && !entry->where) continue;
                if (!read_line(input, line, number)) return 651;
                if (!strncmp(line, "CLASS: ", 7)) return 653;
                if (format == 1) {
                    trim_line(line);
                    fprintf(output, "%s\t\t%ld", line, (long)entry->count);
                    if (extended) fprintf(output, "\t%d\t\t%s", entry->count != 0, entry->count ? "#8" : "#0");
                } else {
                    if (format == 2) {
                        if (sscanf(line, "%d\t%d\t%d\t\t%d", &flag, &lineno, &position, &prior) != 4) return 663;
                    } else if (sscanf(line, "%d\t%d\t%d\t\t%d\t%d\t\t%1023s", &flag, &lineno, &position, &prior, &runs, scale) != 6) return 672;
                    fprintf(output, "%d\t%d\t%d\t\t%ld", flag, lineno, position,
                            (long)(int32_t)((uint32_t)prior + (uint32_t)entry->count));
                    if (format == 2 && extended)
                        fprintf(output, "\t%d\t\t%s", entry->count != 0, entry->count ? "#8" : "#0");
                    else if (format == 3) {
                        if (testname) {
                            length = strlen(scale);
                            if (*tests % 4 == 1) {
                                /* deviation: bound the original fixed input buffer. */
                                if (length + 1 >= sizeof(scale)) return 672;
                                scale[length] = entry->count ? '8' : '0'; scale[length + 1] = 0;
                            } else if (entry->count && length) {
                                unsigned int digit;
                                if (sscanf(scale + length - 1, "%x", &digit) != 1) return 672;
                                digit += 1 << ((4 - *tests % 4) % 4);
                                scale[length - 1] = "0123456789abcdef"[digit & 15];
                            }
                            if (entry->count) ++runs;
                        }
                        fprintf(output, "\t%d\t\t%s", runs, scale);
                    }
                }
                fputc('\n', output);
            }
        }
        if (!read_line(input, line, number)) return 0;
        if (strncmp(line, "CLASS: ", 7)) return 702;
    }
}

int jcov_write_data(char *filename)
{
    FILE *output = NULL, *input = NULL, *final;
    char temporary[260], lock[260], line[1024], directory[1024];
    const char *separator;
    unsigned char *used = NULL;
    struct stat status;
    int descriptor, result = 2, line_number = 0, problem, tests = 0;
    int registry_locked = 0;
    long i;
#ifdef WIN32
    separator = strrchr(filename, '\\');
#else
    separator = strrchr(filename, '/');
#endif
    if (separator) {
        size_t length = separator - filename;
        if (length >= sizeof(directory)) goto too_long;
        memcpy(directory, filename, length); directory[length] = 0;
        if (sysStat(directory, &status)) {
            if (errno != ENOENT) {
                jio_fprintf(stderr, "jcov_exit: Unexpected Error on jcov directory (%s): %s\n", directory, strerror(errno));
                return 2;
            }
            if (sysMkdir(directory, 0777) && errno != EEXIST) {
                jio_fprintf(stderr, "jcov_exit: Error creating jcov directory (%s): %s\n", directory, strerror(errno));
                return 2;
            }
        }
    }
    if (strlen(filename) + 6 > sizeof(temporary)) goto too_long;
    sprintf(temporary, "%s.temp", filename);
    if (strlen(temporary) + 6 > sizeof(lock)) {
        filename = temporary;
        goto too_long;
    }
    sprintf(lock, "%s.lock", temporary);
    descriptor = sysOpen(lock, O_CREAT | O_EXCL, 0600);
    if (descriptor == -1) {
        if (errno == EEXIST) return 1;
        jio_fprintf(stderr, "jcov_exit: problem creating temp file [%s]: %s\n", lock, strerror(errno));
        return 2;
    }
    output = fopen(temporary, "w+b");
    if (!output) {
        sysClose(descriptor); sysUnlink(lock);
        jio_fprintf(stderr, "jcov_exit: problem opening temp file [%s]: %s\n", temporary, strerror(errno));
        return 2;
    }
    fputs(cov_header, output);
    if (sysAccess(filename, 0) != -1) {
        input = fopen(filename, "rb");
        if (!input) {
            jio_fprintf(stderr, "jcov_exit: Can't open a coverage file [%s]\n", filename);
            goto cleanup;
        }
        if (!fgets(line, 1023, input) || strcmp(line, cov_header)) {
            jio_fprintf(stderr, "jcov_warning: Not a coverage file [%s]\n", filename);
            fclose(input); input = NULL;
        }
    }
    /* Keep registry indexes and class metadata stable through the merge.
     * Only native allocation and CRT I/O occur while this lock is held. */
    BINCLASS_LOCK();
    registry_locked = 1;
    used = calloc(nbinclasses ? nbinclasses : 1, 1);
    if (!used) goto cleanup;
    if (input) {
        problem = merge_file(input, output, used, &tests, &line_number);
        if (problem) {
            BINCLASS_UNLOCK();
            registry_locked = 0;
            jio_fprintf(stderr, "jcov_exit: malformed coverage file (line %d %d). jcov aborted\n", line_number, problem);
            goto cleanup;
        }
    } else if (testname) fprintf(output, "TEST %d: %s\n", ++tests, testname);
    for (i = 0; i < nbinclasses; ++i)
        if (binclasses[i] && !used[i] && cbName(binclasses[i])[0] != '[')
            write_class(output, binclasses[i], tests);
    BINCLASS_UNLOCK();
    registry_locked = 0;
    final = fopen(filename, "wb");
    if (!final) {
        jio_fprintf(stderr, "jcov_exit: problem creating coverage file [%s]: %s\n", filename, strerror(errno));
        goto cleanup;
    }
    rewind(output);
    while (fgets(line, 1023, output)) fputs(line, final);
    fclose(final);
    result = 0;
cleanup:
    if (registry_locked) BINCLASS_UNLOCK();
    /* deviation: release temporary allocations on all error paths. */
    free(used);
    if (input) fclose(input);
    fclose(output);
    sysUnlink(temporary); sysClose(descriptor); sysUnlink(lock);
    return result;
too_long:
    jio_fprintf(stderr, "jcov_exit: pathname too long! [%s]\n", filename);
    return 2;
}

void java_cov_dump(void)
{
    int attempt;
    if (!cov_file) return;
    for (attempt = 0; attempt < 5; ++attempt)
        if (jcov_write_data(cov_file) != 1) return;
    jio_fprintf(stderr, "jcov_exit: temp file exists, is someone else running this executable?\n");
}
#endif
