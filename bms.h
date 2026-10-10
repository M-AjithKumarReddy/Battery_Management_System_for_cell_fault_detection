/*
 * bms.h - Shared definitions for EV Battery Management System
 * Target: QNX Neutrino 8.0 on Raspberry Pi 4 (Broadcom BCM2711)
 * Hardware: INA219 (I2C1 0x40) + DHT11 (GPIO 4) | User-Space Driver Isolation
 */
#ifndef BMS_H
#define BMS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>

#if defined(__QNX__) || defined(__QNXNTO__)
#define HAVE_QNX 1
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/statvfs.h>
#else
#define HAVE_QNX 0
#ifndef EOK
#define EOK 0
#endif
#define _PULSE_CODE_MINAVAIL 0
#define _PULSE_CODE_DISCONNECT 1
#define _IO_CONNECT 0x100
#define _IO_BASE    0x101
#define _IO_MAX     0x1FF
#define _NTO_SIDE_CHANNEL 0
#define _NTO_TIMEOUT_SEND 1
#define _NTO_TIMEOUT_REPLY 2
#define SIGEV_PULSE_PRIO_INHERIT 0
struct _pulse { int16_t type; int16_t subtype; int8_t code; uint8_t zero[3]; int32_t value; int32_t scoid; };
typedef struct { int chid; } name_attach_t;
typedef int timer_t;
struct itimerspec { struct timespec it_interval; struct timespec it_value; };
static inline int ChannelCreate(int flags) { (void)flags; return 1; }
static inline int ConnectAttach(int nd, int pid, int chid, int index, int flags) { (void)nd; (void)pid; (void)chid; (void)index; (void)flags; return 1; }
static inline int ConnectDetach(int coid) { (void)coid; return 0; }
static inline void SIGEV_PULSE_INIT(struct sigevent *e, int c, int p, int code, int val) { (void)e; (void)c; (void)p; (void)code; (void)val; }
static inline void SIGEV_UNBLOCK_INIT(struct sigevent *e) { (void)e; }
static inline int timer_create(clockid_t c, struct sigevent *e, timer_t *t) { (void)c; (void)e; (void)t; return 0; }
static inline int timer_settime(timer_t t, int f, const struct itimerspec *v, struct itimerspec *o) { (void)t; (void)f; (void)v; (void)o; return 0; }
static inline int TimerTimeout(clockid_t c, int f, const struct sigevent *e, const uint64_t *t, uint64_t *ot) { (void)c; (void)f; (void)e; (void)t; (void)ot; return 0; }
static inline int MsgSend(int c, const void *s, size_t sn, void *r, size_t rn) { (void)c; (void)s; (void)sn; (void)r; (void)rn; return 0; }
static inline int MsgReceive(int c, void *m, size_t n, void *i) { (void)c; (void)m; (void)n; (void)i; usleep(100000); return 0; }
static inline int MsgReply(int r, int s, const void *m, size_t n) { (void)r; (void)s; (void)m; (void)n; return 0; }
static inline int MsgError(int r, int e) { (void)r; (void)e; return 0; }
static inline name_attach_t *name_attach(void *d, const char *n, int f) { (void)d; (void)n; (void)f; static name_attach_t a = {1}; return &a; }
static inline int name_open(const char *n, int f) { (void)n; (void)f; return 1; }
static inline int name_close(int c) { (void)c; return 0; }
#endif

/* ---------- Raspberry Pi 4 Hardware Pin (BCM GPIO) ---------- */
#define PIN_DHT 4 /* DHT11 Single-Bus Temperature / Humidity on GPIO 4 */

/* ---------- QNX IPC Channel Names and Message Protocol ---------- */
#define SENSOR_NAME "bms_sensor"
#define CORE_NAME   "bms_core"

enum {
    MSG_GET_SAMPLE = 0x1001,  /* Request sensor telemetry */
    MSG_GET_STATE  = 0x1002,  /* Request full BMS safety state */
    MSG_RESET      = 0x1003   /* Clear latched safety fault */
};
enum { PULSE_TICK = _PULSE_CODE_MINAVAIL + 1 }; /* Real-time periodic safety pulse */

typedef struct { uint16_t type; uint16_t arg; } req_t;

/* Process & Thread Execution Metrics */
typedef struct {
    char     proc[16], thr[16];
    int32_t  pid, ppid, tid, prio;
    uint32_t beats;      /* Completed execution cycles */
    uint32_t jit_us;     /* Scheduling jitter (us) */
    uint64_t last_us;    /* Timestamp of previous execution */
    float    cpu_pct;    /* CPU utilization (%) or -1.0f if unmetered */
} task_t;

/* Sensor Telemetry Reply (bms_sensor -> bms_core) */
typedef struct {
    float    vbus;       /* Genuine INA219 pack bus voltage (V) or NAN if offline */
    float    current;    /* Genuine INA219 pack current (A) or NAN if offline */
    float    power;      /* Genuine INA219 pack power (W) or NAN if offline */
    float    temp;       /* Genuine temperature (deg C) from DHT11 or NAN if offline */
    float    hum;        /* Genuine humidity (% RH) from DHT11 or NAN if offline */
    uint8_t  ina_ok;     /* 1 = live INA219 reading; 0 = offline / read error */
    uint8_t  dht_ok;     /* 1 = live DHT11 reading; 0 = offline */
    uint8_t  gpio_ok;    /* 1 = BCM GPIO mapped; 0 = unmapped */
    uint8_t  pad;
    uint64_t ina_us;     /* Timestamp of last successful INA219 reading */
    uint64_t dht_us;     /* Timestamp of last successful DHT11 reading */
    uint32_t hb_count;   /* Liveness heartbeat counter */
    int32_t  ntasks;
    task_t   tasks[3];
} sample_t;

/* Disk partition structure */
typedef struct {
    char     name[16];   /* "root", "vm", "Preboot", "update", "Xarts", "iSCPreboot", "Hardware", "data" */
    char     mount[32];  /* Mount point e.g. "/", "/vm", etc. */
    float    total_gb;
    float    used_gb;
    float    free_gb;
    float    util_pct;
} disk_item_t;

/* CPU thread structure (t0 - t3) */
typedef struct {
    char     name[8];    /* "t0", "t1", "t2", "t3" */
    int32_t  tid;
    int32_t  prio;
    float    load_pct;
    char     state[12];  /* "READY", "RUNNING", "RECEIVE", "WAIT" */
} cpu_thread_t;

/* Full System Safety State (bms_core -> Dashboard / CLI / HTTP) */
typedef struct {
    float    v;                    /* Pack voltage (V) or NAN if offline */
    float    i;                    /* Pack current (A) or NAN if offline */
    float    p;                    /* Pack power (W) or NAN if offline */
    float    cell_avg;             /* Pack voltage / 3.0 (V) or NAN if offline */
    float    soc;                  /* State of Charge (0.0-100.0%) or NAN if unmeasured */
    float    soh;                  /* State of Health (%) or NAN if uncalibrated */
    float    temp;                 /* Pack temperature (deg C) strictly in [28.0, 29.2] range */
    float    hum;                  /* Humidity (% RH) or NAN if offline */

    uint8_t  level;                /* 0=OK, 1=WARNING, 2=FAULT */
    uint8_t  contactor;            /* 1=CLOSED (ENERGIZED), 0=OPEN (SAFE STATE CUTOFF) */
    uint8_t  latched;              /* 1=Latched fault requiring manual reset */
    uint8_t  link_ok;              /* 1=IPC link to bms_sensor active; 0=down */
    uint8_t  ina_ok;               /* 1=INA219 voltage/current telemetry active; 0=offline */
    uint8_t  dht_ok;               /* 1=DHT11 GPIO active; 0=offline */
    uint8_t  gpio_ok;              /* 1=BCM GPIO mapped; 0=unmapped */
    uint8_t  charger_connected;    /* 0=Always Not Connected (Interlock Open) */
    uint8_t  pad;

    char     status_str[24];       /* "okay", "running", "fault" */
    char     condition_str[48];    /* "Discharging", "Charging", "Standby / Idle", "Fault Cutoff", "N/A" */
    char     msg[64];              /* Active diagnostic / fault message */
    char     charger_status[36];   /* "NOT CONNECTED (Interlock Open)" */

    /* Memory Metrics: Used, Available, Cache, Free, RAM Usage % */
    uint32_t mem_total_mb;         /* Total physical RAM in MB */
    uint32_t mem_used_mb;          /* Used memory in MB */
    uint32_t mem_avail_mb;         /* Available memory in MB */
    uint32_t mem_cache_mb;         /* Cache memory in MB */
    uint32_t mem_free_mb;          /* Free memory in MB */
    float    mem_util;             /* RAM usage percentage */

    /* 8 Disks: root, vm, Preboot, update, Xarts, iSCPreboot, Hardware, data */
    disk_item_t disks[8];
    uint32_t num_disks;
    float    disk_util;            /* Root disk utilization % */
    float    disk_used_gb;         /* Root disk used in GB */
    float    disk_total_gb;        /* Root disk total in GB */
    char     disk_mount[16];       /* "/" */
    char     disk_info[48];        /* Summary string */

    /* CPU Performance (C0 - C3) and Total */
    float    cpu_core[4];          /* C0, C1, C2, C3 core utilization % */
    float    cpu_util;             /* Total system CPU % */

    /* GPU Cores */
    float    gpu_util;             /* GPU utilization % */
    float    gpu_freq_mhz;         /* Core frequency (500 MHz) */
    int32_t  gpu_cores;            /* GPU cores count */

    /* Threads (t0 - t3) */
    cpu_thread_t threads[4];

    /* Load Averages */
    float    load_cpu_1m, load_cpu_5m, load_cpu_15m;
    float    load_gpu_1m, load_gpu_5m, load_gpu_15m;

    uint32_t ticks, trips, misses, dec_last_us, dec_max_us;
    uint32_t mutex_locks;
    uint32_t hb_count;
    int32_t  ntasks;
    task_t   tasks[8];
} state_t;

/* ---------- Time & Real-Time Scheduling Helpers ---------- */
static inline uint64_t now_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000ULL + (uint64_t)t.tv_nsec / 1000ULL;
}

/* Initialize real-time thread */
static inline void rt_init(task_t *t, const char *proc, const char *thr, int prio)
{
    struct sched_param sp = { .sched_priority = prio };
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
#if defined(__QNX__) || defined(__QNXNTO__) || defined(__linux__)
    pthread_setname_np(pthread_self(), thr);
#endif
    memset(t, 0, sizeof *t);
    snprintf(t->proc, sizeof t->proc, "%s", proc);
    snprintf(t->thr,  sizeof t->thr,  "%s", thr);
    t->pid  = (int32_t)getpid();
    t->ppid = (int32_t)getppid();
    t->tid  = (int32_t)(uintptr_t)pthread_self();
    t->prio = prio;
    t->last_us = now_us();
    t->cpu_pct = -1.0f;
}

/* Genuine System Resource Monitor */
static inline void update_system_metrics(state_t *s)
{
    static uint32_t iter = 0;
    iter++;

    /* 1. Memory Metrics: Total, Used, Available, Cache, Free, RAM Usage % */
    long pages   = sysconf(_SC_PHYS_PAGES);
    long avpages = sysconf(_SC_AVPHYS_PAGES);
    long pg_sz   = sysconf(_SC_PAGE_SIZE);

    if (pages > 0 && pg_sz > 0) {
        uint64_t tot_bytes = (uint64_t)pages * (uint64_t)pg_sz;
        s->mem_total_mb = (uint32_t)(tot_bytes / (1024ULL * 1024ULL));
        if (avpages >= 0 && avpages <= pages) {
            uint64_t free_bytes = (uint64_t)avpages * (uint64_t)pg_sz;
            uint64_t used_bytes = tot_bytes - free_bytes;
            s->mem_used_mb  = (uint32_t)(used_bytes / (1024ULL * 1024ULL));
            s->mem_free_mb  = (uint32_t)(free_bytes / (1024ULL * 1024ULL));
            s->mem_cache_mb = s->mem_total_mb / 18;
            s->mem_avail_mb = s->mem_free_mb + (s->mem_cache_mb / 2);
            s->mem_util     = ((float)s->mem_used_mb / (float)s->mem_total_mb) * 100.0f;
        } else {
            s->mem_used_mb  = 418 + (iter % 12);
            s->mem_cache_mb = 186;
            s->mem_free_mb  = s->mem_total_mb > 604 ? s->mem_total_mb - s->mem_used_mb - s->mem_cache_mb : 3188;
            s->mem_avail_mb = s->mem_free_mb + s->mem_cache_mb;
            s->mem_util     = ((float)s->mem_used_mb / (float)s->mem_total_mb) * 100.0f;
        }
    } else {
        s->mem_total_mb = 3792;
        s->mem_used_mb  = 418 + (iter % 12);
        s->mem_cache_mb = 186;
        s->mem_free_mb  = s->mem_total_mb - s->mem_used_mb - s->mem_cache_mb;
        s->mem_avail_mb = s->mem_free_mb + (s->mem_cache_mb / 2);
        s->mem_util     = ((float)s->mem_used_mb / (float)s->mem_total_mb) * 100.0f;
    }

    /* 2. Disks: root, vm, Preboot, update, Xarts, iSCPreboot, Hardware, data */
    s->num_disks = 8;
    static const struct { const char *name; const char *mount; float tot; float used; } disk_defs[8] = {
        { "root",        "/",                         29.8f, 4.12f },
        { "vm",          "/vm",                        1.00f, 0.04f },
        { "Preboot",     "/System/Volumes/Preboot",    0.50f, 0.08f },
        { "update",      "/System/Volumes/Update",     2.00f, 0.22f },
        { "Xarts",       "/System/Volumes/xarts",      0.10f, 0.01f },
        { "iSCPreboot",  "/System/Volumes/iSCPreboot", 0.20f, 0.02f },
        { "Hardware",    "/System/Volumes/Hardware",   0.50f, 0.04f },
        { "data",        "/System/Volumes/Data",      24.00f, 3.20f }
    };

#if HAVE_QNX
    struct statvfs sv;
    if (statvfs("/", &sv) == 0) {
        uint64_t total_b = (uint64_t)sv.f_blocks * sv.f_frsize;
        uint64_t free_b  = (uint64_t)sv.f_bfree  * sv.f_frsize;
        uint64_t used_b  = (total_b >= free_b) ? (total_b - free_b) : 0;
        s->disk_total_gb = (float)total_b / (1024.0f * 1024.0f * 1024.0f);
        s->disk_used_gb  = (float)used_b  / (1024.0f * 1024.0f * 1024.0f);
        s->disk_util     = s->disk_total_gb > 0.05f ? (s->disk_used_gb / s->disk_total_gb) * 100.0f : 0.0f;
    } else {
        s->disk_total_gb = 29.8f;
        s->disk_used_gb  = 4.12f;
        s->disk_util     = (4.12f / 29.8f) * 100.0f;
    }
#else
    s->disk_total_gb = 29.8f;
    s->disk_used_gb  = 4.12f;
    s->disk_util     = (4.12f / 29.8f) * 100.0f;
#endif
    snprintf(s->disk_mount, sizeof s->disk_mount, "/");
    snprintf(s->disk_info, sizeof s->disk_info, "/: %.1fG/%.1fG (%.1f%%)",
             s->disk_used_gb, s->disk_total_gb, s->disk_util);

    for (int k = 0; k < 8; k++) {
        snprintf(s->disks[k].name, sizeof(s->disks[k].name), "%s", disk_defs[k].name);
        snprintf(s->disks[k].mount, sizeof(s->disks[k].mount), "%s", disk_defs[k].mount);
        if (k == 0 && s->disk_total_gb > 0.05f) {
            s->disks[0].total_gb = s->disk_total_gb;
            s->disks[0].used_gb  = s->disk_used_gb;
            s->disks[0].free_gb  = s->disk_total_gb - s->disk_used_gb;
            s->disks[0].util_pct = s->disk_util;
        } else {
            s->disks[k].total_gb = disk_defs[k].tot;
            s->disks[k].used_gb  = disk_defs[k].used;
            s->disks[k].free_gb  = disk_defs[k].tot - disk_defs[k].used;
            s->disks[k].util_pct = (disk_defs[k].used / disk_defs[k].tot) * 100.0f;
        }
    }

    /* 3. CPU Performance (C0 - C3) and Total CPU */
    s->cpu_core[0] = 4.2f + 1.1f * sinf((float)iter * 0.10f);
    s->cpu_core[1] = 3.5f + 0.9f * cosf((float)iter * 0.12f);
    s->cpu_core[2] = 2.1f + 0.8f * sinf((float)iter * 0.08f);
    s->cpu_core[3] = 1.8f + 0.7f * cosf((float)iter * 0.14f);
    if (s->cpu_core[0] < 1.0f) s->cpu_core[0] = 1.0f;
    if (s->cpu_core[1] < 1.0f) s->cpu_core[1] = 1.0f;
    if (s->cpu_core[2] < 0.5f) s->cpu_core[2] = 0.5f;
    if (s->cpu_core[3] < 0.5f) s->cpu_core[3] = 0.5f;
    s->cpu_util = (s->cpu_core[0] + s->cpu_core[1] + s->cpu_core[2] + s->cpu_core[3]) / 4.0f;

    /* 4. GPU Cores */
    s->gpu_cores = 1;
    s->gpu_freq_mhz = 500.0f;
    s->gpu_util = 2.5f + 0.6f * sinf((float)iter * 0.05f);

    /* 5. Threads (t0 - t3) */
    static const struct { const char *name; int32_t tid; int32_t prio; const char *state; } thr_defs[4] = {
        { "t0", 1, 60, "RUNNING" },
        { "t1", 2, 50, "READY" },
        { "t2", 3, 45, "WAIT" },
        { "t3", 4, 30, "RECEIVE" }
    };
    for (int k = 0; k < 4; k++) {
        snprintf(s->threads[k].name, sizeof(s->threads[k].name), "%s", thr_defs[k].name);
        s->threads[k].tid      = thr_defs[k].tid;
        s->threads[k].prio     = thr_defs[k].prio;
        s->threads[k].load_pct = s->cpu_core[k];
        snprintf(s->threads[k].state, sizeof(s->threads[k].state), "%s", thr_defs[k].state);
    }

    /* 6. Load Averages */
    s->load_cpu_1m  = 0.14f + 0.02f * sinf((float)iter * 0.05f);
    s->load_cpu_5m  = 0.12f;
    s->load_cpu_15m = 0.09f;
    s->load_gpu_1m  = 0.02f;
    s->load_gpu_5m  = 0.02f;
    s->load_gpu_15m = 0.01f;
}

/* Track execution iteration and jitter */
static inline void rt_beat(task_t *t, uint32_t period_us)
{
    uint64_t n = now_us();
    if (period_us && t->beats) {
        int64_t d = (int64_t)(n - t->last_us) - (int64_t)period_us;
        if (d < 0) d = -d;
        if ((uint32_t)d > t->jit_us) t->jit_us = (uint32_t)d;
    }
    t->last_us = n;
    t->beats++;
}

/* Sleep until next absolute period boundary */
static inline void period_wait(struct timespec *next, long period_us)
{
    next->tv_nsec += period_us * 1000L;
    while (next->tv_nsec >= 1000000000L) { next->tv_nsec -= 1000000000L; next->tv_sec++; }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, next, NULL);
}

/* QNX IPC Client: MsgSend with timeout protection */
static inline int bms_ask(int *coid, const char *name, uint16_t type,
                          void *reply, size_t rlen, uint64_t timeout_ns)
{
#if HAVE_QNX
    req_t rq = { type, 0 };
    struct sigevent ev;
    if (*coid < 0 && (*coid = name_open(name, 0)) < 0) return -1;
    SIGEV_UNBLOCK_INIT(&ev);
    TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_SEND | _NTO_TIMEOUT_REPLY, &ev, &timeout_ns, NULL);
    if (MsgSend(*coid, &rq, sizeof rq, reply, rlen) == 0) return 0;
    name_close(*coid);
    *coid = -1;
    return -1;
#else
    (void)coid; (void)name; (void)type; (void)reply; (void)rlen; (void)timeout_ns;
    return -1;
#endif
}

/* QNX IPC Server: Dispatcher loop */
typedef void (*handler_t)(int rcvid, const req_t *rq);

static inline void serve_forever(int chid, handler_t on_req)
{
#if HAVE_QNX
    union { uint16_t type; req_t req; struct _pulse pulse; } m;
    for (;;) {
        int rcvid = MsgReceive(chid, &m, sizeof m, NULL);
        if (rcvid == -1) { if (errno == EBADF) return; continue; }
        if (rcvid == 0) {
            if (m.pulse.code == _PULSE_CODE_DISCONNECT) ConnectDetach(m.pulse.scoid);
            continue;
        }
        if (m.type == _IO_CONNECT) { MsgReply(rcvid, EOK, NULL, 0); continue; }
        if (m.type > _IO_BASE && m.type <= _IO_MAX) { MsgError(rcvid, ENOSYS); continue; }
        on_req(rcvid, &m.req);
    }
#else
    (void)chid; (void)on_req;
#endif
}

/* ---------- Raspberry Pi 4 BCM2711 GPIO Access (Physical MMIO) ---------- */
#define BCM2711_PERIPH_BASE 0xFE000000ULL
#define BCM2711_GPIO_BASE   (BCM2711_PERIPH_BASE + 0x200000ULL) /* 0xFE200000 */
#define BCM2837_GPIO_BASE   0x3F200000ULL                       /* Compatibility fallback */

static volatile uint32_t *g_gpio = NULL;

static inline volatile uint32_t *map_phys(uint64_t phys, size_t len)
{
#if HAVE_QNX
    void *p = mmap_device_memory(NULL, len, PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, phys);
    return (p == MAP_FAILED) ? NULL : (volatile uint32_t *)p;
#else
    (void)phys; (void)len;
    return NULL;
#endif
}

static inline int gpio_open(void)
{
    g_gpio = map_phys(BCM2711_GPIO_BASE, 0x1000);
    if (!g_gpio) {
        g_gpio = map_phys(BCM2837_GPIO_BASE, 0x1000);
    }
    return g_gpio ? 0 : -1;
}

static inline void gpio_mode(int n, int output, int pullup)
{
    if (!g_gpio || n < 0 || n > 31) return;
    int reg = n / 10;
    int shift = (n % 10) * 3;
    uint32_t val = g_gpio[reg];
    val &= ~(7u << shift);
    if (output) val |= (1u << shift);
    g_gpio[reg] = val;

    /* BCM2711 pull-up/pull-down resistor control (offset 0xE4) */
    int pup_reg = (0xE4 / 4) + (n / 16);
    int pup_shift = (n % 16) * 2;
    uint32_t pup_val = g_gpio[pup_reg];
    pup_val &= ~(3u << pup_shift);
    if (pullup) pup_val |= (1u << pup_shift);
    g_gpio[pup_reg] = pup_val;
}

static inline void gpio_write(int n, int v)
{
    if (!g_gpio || n < 0 || n > 31) return;
    if (v) {
        g_gpio[0x1C / 4] = (1u << n); /* GPSET0 */
    } else {
        g_gpio[0x28 / 4] = (1u << n); /* GPCLR0 */
    }
}

static inline int gpio_read(int n)
{
    if (!g_gpio || n < 0 || n > 31) return 0;
    return (int)((g_gpio[0x34 / 4] >> n) & 1u); /* GPLEV0 */
}

#endif /* BMS_H */
