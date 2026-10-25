// dx test harness: headless mGBA (libmgba) driven by a tiny script language.
//
//   harness <rom.gba> <save.sav|-> <symbols.txt> <script.txt> <outdir>
//
// Script commands (one per line, '#' comments):
//   wait N                       run N frames with no keys held
//   press KEY [HOLD] [GAP]       hold KEY for HOLD frames (default 4), then release for GAP (default 12)
//   hold KEY N                   hold KEY for N frames
//   shot NAME                    write outdir/NAME.png
//   read NAME EXPR SIZE          read SIZE (1/2/4) bytes at EXPR, log it as NAME
//   expect EXPR SIZE OP VALUE    read and compare (OP: == != < <= > >=), logs PASS/FAIL
//   waitfor EXPR SIZE OP VALUE MAXFRAMES   run frames until the comparison holds (FAIL on timeout)
//   mash KEY EXPR SIZE OP VALUE [MAXFRAMES]   tap KEY every 16 frames until the comparison holds
//   write EXPR SIZE VALUE        poke RAM (test setup only)
//   cmpreads NAME1 OP NAME2      compare two earlier `read` values (e.g. position changed)
//   calc NAME X OP Y             NAME = X OP Y (+ - * /) over `read` names or numbers, for cmpreads
//   pin EXPR SIZE VALUE / unpin  keep rewriting a value before every frame
//   mash? ...                    like mash, but running out of frames is not a failure
//   dump NAME                    write EWRAM+IWRAM to NAME.bin (diff two builds' RAM)
//   watch EXPR                   log PC/LR and stack return addresses of every write to EXPR
//   break SYM [r1=0]             log callers each time SYM runs (r1=0: only when r1 is 0, e.g. a divisor)
//   log TEXT                     echo TEXT into the results
// EXPR: number (0x.. ok) | symbol | symbol+off | *EXPR (dereference a 32-bit pointer) e.g. *gDungeon+0x644
//       | @symbol (the u32 stored at symbol, e.g. the debug ROM's gDxOff* struct offsets), @symbol!N adds N
// KEY: A B SELECT START RIGHT LEFT UP DOWN R L (join with + to press together)
#include <mgba/core/core.h>
#include <mgba/debugger/debugger.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/core/log.h>
#include <mgba/internal/arm/arm.h>
#include <mgba-util/vfs.h>
#include <png.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct mCore *core;
static color_t *video;
static unsigned vw, vh;
static FILE *results;
static const char *outdir;
static int failures;

// values of earlier `read` commands, by name, for `cmpreads`
struct readval { char name[128]; long v; };
static struct readval reads[256];
static int nreads;

static void remember(const char *name, long v)
{
    int i;
    for (i = 0; i < nreads; i++)
        if (!strcmp(reads[i].name, name)) { reads[i].v = v; return; }
    if (nreads < 256) {
        snprintf(reads[nreads].name, sizeof reads[nreads].name, "%s", name);
        reads[nreads].v = v;
        nreads++;
    }
}

static int recall(const char *name, long *v)
{
    int i;
    for (i = 0; i < nreads; i++)
        if (!strcmp(reads[i].name, name)) { *v = reads[i].v; return 1; }
    return 0;
}

struct sym { char name[96]; unsigned addr; };
static struct sym *syms;
static int nsyms;

static void quiet_log(struct mLogger *l, int cat, enum mLogLevel lvl, const char *fmt, va_list args)
{
    (void)l; (void)cat; (void)lvl; (void)fmt; (void)args;
}
static struct mLogger quiet_logger = { .log = quiet_log };

static void out(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(results, fmt, ap);
    va_end(ap);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(results);
}

static void load_syms(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256];
    if (!f) { perror(path); exit(2); }
    while (fgets(line, sizeof line, f)) {
        unsigned addr; char type; char name[96];
        if (sscanf(line, "%x %c %95s", &addr, &type, name) == 3) {
            syms = realloc(syms, (nsyms + 1) * sizeof *syms);
            syms[nsyms].addr = addr;
            strcpy(syms[nsyms].name, name);
            nsyms++;
        }
    }
    fclose(f);
}

static unsigned eval(const char *e)
{
    char name[96];
    const char *plus;
    unsigned base, off = 0;
    size_t n;
    int i;

    if (*e == '*') {
        // *EXPR+off : deref the part before the last '+', then add off
        char tmp[128];
        strncpy(tmp, e + 1, sizeof tmp - 1);
        tmp[sizeof tmp - 1] = 0;
        char *p = strrchr(tmp, '+');
        if (p) { *p = 0; off = (p[1] == '@') ? eval(p + 1) : strtoul(p + 1, NULL, 0); }
        // a null pointer (or one into BIOS/IO) stays 0, so writes through it can be skipped
        unsigned at = eval(tmp), v = at >= 0x02000000 ? core->busRead32(core, at) : 0;
        return v ? v + off : 0;
    }
    if (isdigit((unsigned char)*e))
        return strtoul(e, NULL, 0);
    if (*e == '@') {
        // @sym or @sym!N: the u32 stored at sym (plus N)
        char sym[96];
        const char *bang = strchr(e, '!');
        size_t len = bang ? (size_t)(bang - e - 1) : strlen(e + 1);
        if (len >= sizeof sym) len = sizeof sym - 1;
        memcpy(sym, e + 1, len);
        sym[len] = 0;
        return core->busRead32(core, eval(sym)) + (bang ? strtoul(bang + 1, NULL, 0) : 0);
    }

    plus = strchr(e, '+');
    n = plus ? (size_t)(plus - e) : strlen(e);
    if (n >= sizeof name) n = sizeof name - 1;
    memcpy(name, e, n);
    name[n] = 0;
    if (plus) off = (plus[1] == '@') ? eval(plus + 1) : strtoul(plus + 1, NULL, 0);
    for (i = 0; i < nsyms; i++)
        if (!strcmp(syms[i].name, name)) { base = syms[i].addr; return base + off; }
    out("ERROR unknown symbol %s\n", name);
    failures++;
    return 0;
}

static unsigned rd(unsigned addr, int size)
{
    if (addr < 0x02000000) // through a null pointer (BIOS/IO): read as 0
        return 0;
    if (size == 1) return core->busRead8(core, addr);
    if (size == 2) return core->busRead16(core, addr);
    return core->busRead32(core, addr);
}

static int cmp(long a, const char *op, long b)
{
    if (!strcmp(op, "==")) return a == b;
    if (!strcmp(op, "!=")) return a != b;
    if (!strcmp(op, "<")) return a < b;
    if (!strcmp(op, "<=")) return a <= b;
    if (!strcmp(op, ">")) return a > b;
    if (!strcmp(op, ">=")) return a >= b;
    if (!strcmp(op, "&")) return (a & b) != 0;   // any of the mask bits set
    if (!strcmp(op, "!&")) return (a & b) == 0;  // none of the mask bits set
    return 0;
}

static long sx(unsigned v, int size)
{
    if (size == 1) return (signed char)v;
    if (size == 2) return (short)v;
    return (int)v;
}

static uint32_t keymask(const char *k)
{
    static const char *names[] = { "A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L" };
    char buf[64], *tok, *save;
    uint32_t m = 0;
    strncpy(buf, k, sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    for (tok = strtok_r(buf, "+", &save); tok; tok = strtok_r(NULL, "+", &save)) {
        int i, found = 0;
        for (i = 0; i < 10; i++)
            if (!strcasecmp(tok, names[i])) { m |= 1u << i; found = 1; }
        if (!found) { out("ERROR unknown key %s\n", tok); failures++; }
    }
    return m;
}

// pinned RAM values, rewritten before every frame (pin/unpin commands)
struct pin { char expr[128]; int size; unsigned value; };
static struct pin pins[16];
static int npins;

static void poke(unsigned addr, int size, unsigned v)
{
    if (size == 1) core->busWrite8(core, addr, v);
    else if (size == 2) core->busWrite16(core, addr, v);
    else core->busWrite32(core, addr, v);
}

// watch EXPR: log the PC/LR of writes to one address (libmgba watchpoint), for hunting memory corruption
static struct mDebugger dbg;
static int watching, watchHits;

static int breakZeroR1, breakHits;
static void log_stack(void);

static void dbg_entered(struct mDebugger *d, enum mDebuggerEntryReason reason, struct mDebuggerEntryInfo *info)
{
    struct ARMCore *cpu = core->cpu;
    if (reason == DEBUGGER_ENTER_BREAKPOINT) {
        // break SYM [r1=0]: log where it was called from (optionally only when r1, e.g. a divisor, is 0)
        if ((!breakZeroR1 || cpu->gprs[1] == 0) && breakHits++ < 8) {
            out("BREAK pc=%08x lr=%08x r0=%08x r1=%08x frame=%u\n", cpu->gprs[15], cpu->gprs[14], cpu->gprs[0],
                cpu->gprs[1], core->frameCounter(core));
            log_stack();
        }
        d->state = DEBUGGER_RUNNING;
        return;
    }
    if (reason == DEBUGGER_ENTER_WATCHPOINT && info && watchHits++ < 40)
        out("WATCH @%08x %08x -> %08x pc=%08x lr=%08x frame=%u\n", info->address, info->type.wp.oldValue,
            info->type.wp.newValue, cpu->gprs[15], cpu->gprs[14], core->frameCounter(core));
    if (reason == DEBUGGER_ENTER_WATCHPOINT && watchHits <= 40) {
        // likely return addresses on the stack (for finding the caller)
        int k, shown = 0;
        for (k = 0; k < 64 && shown < 12; k++) {
            unsigned w = core->busRead32(core, cpu->gprs[13] + 4 * k);
            if ((w >> 24) == 0x08 && (w & 1)) { out("  stack+%d: %08x\n", 4 * k, w); shown++; }
        }
    }
    d->state = DEBUGGER_RUNNING;
}

static const char *sym_of(unsigned a, unsigned *base)
{
    const char *best = NULL;
    unsigned bestAddr = 0;
    int j;
    for (j = 0; j < nsyms; j++)
        if (syms[j].addr <= a && syms[j].addr > bestAddr && (syms[j].addr >> 24) == (a >> 24)) {
            bestAddr = syms[j].addr;
            best = syms[j].name;
        }
    *base = bestAddr;
    return best;
}

static void log_stack(void)
{
    struct ARMCore *cpu = core->cpu;
    unsigned base, k;
    const char *name = sym_of(cpu->gprs[14], &base);
    if (name)
        out("  lr: %s+0x%x\n", name, cpu->gprs[14] - base);
    for (k = 0; k < 64; k++) {
        unsigned w = core->busRead32(core, cpu->gprs[13] + 4 * k);
        if ((w >> 24) != 0x08 || !(w & 1))
            continue;
        name = sym_of(w, &base);
        if (name)
            out("  stack+%u: %s+0x%x\n", 4 * k, name, w - base);
    }
}

static void dbg_paused(struct mDebugger *d)
{
    d->state = DEBUGGER_RUNNING;
}

static void frames(int n, uint32_t keys)
{
    int i;
    core->setKeys(core, keys);
    while (n-- > 0) {
        for (i = 0; i < npins; i++) {
            unsigned addr = eval(pins[i].expr);
            if (addr >= 0x02000000) // pointer chains read 0 while their object doesn't exist
                poke(addr, pins[i].size, pins[i].value);
        }
        if (watching) mDebuggerRunFrame(&dbg);
        else core->runFrame(core);
    }
    core->setKeys(core, 0);
}

static void shot(const char *name)
{
    char path[1024];
    FILE *f;
    png_structp png;
    png_infop info;
    unsigned y, x;
    unsigned char *row = malloc(vw * 3);

    snprintf(path, sizeof path, "%s/%s.png", outdir, name);
    f = fopen(path, "wb");
    if (!f) { perror(path); failures++; free(row); return; }
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info = png_create_info_struct(png);
    png_init_io(png, f);
    png_set_IHDR(png, info, vw, vh, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    for (y = 0; y < vh; y++) {
        for (x = 0; x < vw; x++) {
            color_t c = video[y * vw + x];   // 32-bit: 0xXXBBGGRR
            row[x * 3 + 0] = c & 0xFF;
            row[x * 3 + 1] = (c >> 8) & 0xFF;
            row[x * 3 + 2] = (c >> 16) & 0xFF;
        }
        png_write_row(png, row);
    }
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);
    fclose(f);
    free(row);
    out("SHOT %s frame=%u\n", path, core->frameCounter(core));
}

int main(int argc, char **argv)
{
    FILE *script;
    char line[512], path[1024];
    int lineno = 0;

    if (argc != 6) {
        fprintf(stderr, "usage: %s rom.gba save.sav|- symbols.txt script.txt outdir\n", argv[0]);
        return 2;
    }
    outdir = argv[5];
    snprintf(path, sizeof path, "%s/results.txt", outdir);
    results = fopen(path, "w");
    if (!results) { perror(path); return 2; }

    mLogSetDefaultLogger(&quiet_logger);
    load_syms(argv[3]);

    core = mCoreFind(argv[1]);
    if (!core || !core->init(core)) { out("ERROR cannot create core\n"); return 2; }
    mCoreInitConfig(core, NULL);
    core->desiredVideoDimensions(core, &vw, &vh);
    video = calloc(vw * vh, sizeof(color_t));
    core->setVideoBuffer(core, video, vw);
    if (!mCoreLoadFile(core, argv[1])) { out("ERROR cannot load ROM\n"); return 2; }
    if (strcmp(argv[2], "-")) {
        struct VFile *sv = VFileOpen(argv[2], O_RDWR);
        if (!sv || !core->loadSave(core, sv)) { out("ERROR cannot load save %s\n", argv[2]); return 2; }
    }
    core->reset(core);

    script = fopen(argv[4], "r");
    if (!script) { perror(argv[4]); return 2; }
    while (fgets(line, sizeof line, script)) {
        char cmd[32], a[128], b[128], c[32], d[64], e[32];
        int n;
        lineno++;
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0] || line[0] == '#') continue;
        n = sscanf(line, "%31s %127s %127s %31s %63s %31s", cmd, a, b, c, d, e);
        if (!strcmp(cmd, "wait") && n >= 2) frames(atoi(a), 0);
        else if (!strcmp(cmd, "press") && n >= 2) {
            frames(n >= 3 ? atoi(b) : 4, keymask(a));
            frames(n >= 4 ? atoi(c) : 12, 0);
        }
        else if (!strcmp(cmd, "hold") && n >= 3) frames(atoi(b), keymask(a));
        else if (!strcmp(cmd, "shot") && n >= 2) shot(a);
        else if (!strcmp(cmd, "log")) out("LOG %s\n", line + 4);
        else if (!strcmp(cmd, "read") && n >= 4) {
            unsigned addr = eval(b), v = rd(addr, atoi(c));
            out("READ %s @%08x = %ld (0x%x)\n", a, addr, sx(v, atoi(c)), v);
            remember(a, sx(v, atoi(c)));
        }
        else if (!strcmp(cmd, "expect") && n >= 5) {
            int size = atoi(b);
            unsigned addr = eval(a);
            long v = sx(rd(addr, size), size), want = strtol(d, NULL, 0);
            int ok = cmp(v, c, want);
            if (!ok) failures++;
            out("%s line %d: %s = %ld, expected %s %ld\n", ok ? "PASS" : "FAIL", lineno, a, v, c, want);
        }
        else if (!strcmp(cmd, "waitfor") && n >= 6) {
            int size = atoi(b), max = atoi(e), i, ok = 0;
            long want = strtol(d, NULL, 0), v = 0;
            for (i = 0; i < max; i++) {
                v = sx(rd(eval(a), size), size);
                if (cmp(v, c, want)) { ok = 1; break; }
                frames(1, 0);
            }
            if (!ok) failures++;
            out("%s line %d: waitfor %s %s %ld -> %ld after %d frames\n", ok ? "PASS" : "FAIL", lineno, a, c, want, v, i);
        }
        else if (!strcmp(cmd, "cmpreads") && n >= 4) {
            // cmpreads NAME1 OP NAME2: compare two earlier `read` values
            long v1, v2;
            int ok = recall(a, &v1) && recall(c, &v2) && cmp(v1, b, v2);
            if (!ok) failures++;
            out("%s line %d: %s %s %s\n", ok ? "PASS" : "FAIL", lineno, a, b, c);
        }
        else if (!strcmp(cmd, "reset")) {
            // reset: power-cycle; the cartridge save survives
            core->reset(core);
            out("RESET\n");
        }
        else if (!strcmp(cmd, "regs")) {
            // regs: log pc/lr/sp with the nearest symbols (where is the game stuck?)
            struct ARMCore *cpu = core->cpu;
            unsigned r[3] = { (unsigned)cpu->gprs[15], (unsigned)cpu->gprs[14], (unsigned)cpu->gprs[13] };
            const char *names[3] = { "pc", "lr", "sp" };
            int k;
            for (k = 0; k < 3; k++) {
                const char *best = "?";
                unsigned bestAddr = 0;
                int j;
                for (j = 0; j < nsyms; j++)
                    if (syms[j].addr <= r[k] && syms[j].addr > bestAddr && (syms[j].addr >> 24) == (r[k] >> 24)) {
                        bestAddr = syms[j].addr;
                        best = syms[j].name;
                    }
                out("REG %s=%08x %s+0x%x\n", names[k], r[k], best, r[k] - bestAddr);
            }
            // likely return addresses on the stack (callers)
            for (k = 0; k < 96; k++) {
                unsigned w = core->busRead32(core, r[2] + 4 * k);
                const char *best = NULL;
                unsigned bestAddr = 0;
                int j;
                if ((w >> 24) != 0x08 || !(w & 1))
                    continue;
                for (j = 0; j < nsyms; j++)
                    if (syms[j].addr <= w && syms[j].addr > bestAddr && (syms[j].addr >> 24) == 0x08) {
                        bestAddr = syms[j].addr;
                        best = syms[j].name;
                    }
                if (best)
                    out("STACK +%d: %08x %s+0x%x\n", 4 * k, w, best, w - bestAddr);
            }
        }
        else if (!strcmp(cmd, "write") && n >= 4) {
            // write EXPR SIZE VALUE: poke RAM (test setup only)
            unsigned addr = eval(a), v = strtoul(c, NULL, 0);
            if (addr < 0x02000000) { out("WRITE skipped: %s is a null pointer\n", a); continue; }
            poke(addr, atoi(b), v);
            out("WRITE @%08x = %u (0x%x)\n", addr, v, v);
        }
        else if (!strcmp(cmd, "pin") && n >= 4 && npins < 16) {
            // pin EXPR SIZE VALUE: keep rewriting this value before every frame (EXPR re-evaluated each time)
            snprintf(pins[npins].expr, sizeof(pins[npins].expr), "%s", a);
            pins[npins].size = atoi(b);
            pins[npins].value = strtoul(c, NULL, 0);
            npins++;
            out("PIN %s = %s\n", a, c);
        }
        else if (!strcmp(cmd, "dump") && n >= 2) {
            // dump NAME: write EWRAM (256 KB) then IWRAM (32 KB) to <outdir>/NAME.bin (for diffing builds)
            char path[512];
            FILE *f;
            unsigned addr;
            snprintf(path, sizeof path, "%s/%s.bin", outdir, a);
            f = fopen(path, "wb");
            for (addr = 0x02000000; f && addr < 0x02040000; addr++) fputc(core->busRead8(core, addr), f);
            for (addr = 0x03000000; f && addr < 0x03008000; addr++) fputc(core->busRead8(core, addr), f);
            if (f) fclose(f);
            out("DUMP %s\n", path);
        }
        else if (!strcmp(cmd, "break") && n >= 2) {
            struct mBreakpoint bp = { 0 };
            if (!watching) {
                memset(&dbg, 0, sizeof dbg);
                dbg.type = DEBUGGER_CUSTOM;
                dbg.entered = dbg_entered;
                dbg.paused = dbg_paused;
                mDebuggerAttach(&dbg, core);
                dbg.state = DEBUGGER_RUNNING;
                watching = 1;
            }
            bp.address = eval(a) & ~1u;
            bp.segment = -1;
            bp.type = BREAKPOINT_HARDWARE;
            breakZeroR1 = (n >= 3 && !strcmp(b, "r1=0"));
            dbg.platform->setBreakpoint(dbg.platform, &bp);
            out("BREAKPOINT %s @%08x%s\n", a, bp.address, breakZeroR1 ? " when r1=0" : "");
        }
        else if (!strcmp(cmd, "watch") && n >= 2) {
            struct mWatchpoint wp = { 0 };
            if (!watching) {
                memset(&dbg, 0, sizeof dbg);
                dbg.type = DEBUGGER_CUSTOM;
                dbg.entered = dbg_entered;
                dbg.paused = dbg_paused;
                mDebuggerAttach(&dbg, core);
                dbg.state = DEBUGGER_RUNNING;
                watching = 1;
            }
            wp.address = eval(a);
            wp.segment = -1;
            wp.type = WATCHPOINT_WRITE;
            dbg.platform->setWatchpoint(dbg.platform, &wp);
            out("WATCHING %s @%08x\n", a, wp.address);
        }
        else if (!strcmp(cmd, "calc") && n >= 5) {
            long x, y, r = 0;
            int okx = isdigit((unsigned char)b[0]) || b[0] == '-' ? (x = strtol(b, NULL, 0), 1) : recall(b, &x);
            int oky = isdigit((unsigned char)d[0]) || d[0] == '-' ? (y = strtol(d, NULL, 0), 1) : recall(d, &y);
            if (!okx || !oky) { out("ERROR line %d: calc: unknown value\n", lineno); failures++; continue; }
            if (!strcmp(c, "+")) r = x + y;
            else if (!strcmp(c, "-")) r = x - y;
            else if (!strcmp(c, "*")) r = x * y;
            else if (!strcmp(c, "/") && y) r = x / y;
            remember(a, r);
            out("CALC %s = %ld %s %ld = %ld\n", a, x, c, y, r);
        }
        else if (!strcmp(cmd, "unpin")) {
            npins = 0;
            out("UNPIN\n");
        }
        else if ((!strcmp(cmd, "mash") || !strcmp(cmd, "mash?")) && n >= 6) {
            // mash KEY EXPR SIZE OP VALUE [MAXFRAMES=3000]: tap KEY every 16 frames until the comparison holds
            // (mash? is the same, but running out of frames is not a failure)
            int soft = cmd[4] == '?';
            char f[32] = "";
            int size = atoi(c), max = 3000, i = 0, ok = 0;
            long want = strtol(e, NULL, 0), v = 0;
            uint32_t k = keymask(a);
            sscanf(line, "%*s %*s %*s %*s %*s %*s %31s", f);
            if (f[0]) max = atoi(f);
            while (i < max) {
                v = sx(rd(eval(b), size), size);
                if (cmp(v, d, want)) { ok = 1; break; }
                frames(4, k);
                frames(12, 0);
                i += 16;
            }
            if (!ok && !soft) failures++;
            out("%s line %d: mash %s until %s %s %ld -> %ld after %d frames\n", ok ? "PASS" : soft ? "SKIP" : "FAIL", lineno, a, b, d, want, v, i);
        }
        else { out("ERROR line %d: bad command: %s\n", lineno, line); failures++; }
    }

    out("RESULT %s (%d failure%s) frames=%u\n", failures ? "FAIL" : "PASS", failures, failures == 1 ? "" : "s", core->frameCounter(core));
    fclose(results);
    core->deinit(core);
    return failures ? 1 : 0;
}
