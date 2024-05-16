//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"

typedef struct {
    volatile unsigned long count;
    unsigned long curr_len;
    z_clock_t start;
} z_stats_t;

static z_stats_t test_stats;
static volatile bool test_end;

#if Z_FEATURE_SUBSCRIPTION == 1
void z_stats_stop(z_stats_t *stats) {
    // Ignore default value
    if (stats->curr_len == 0) {
        return;
    }
    // Print values
    unsigned long elapsed_ms = z_clock_elapsed_ms(&stats->start);
    printf("End test for pkt len: %lu, msg nb: %lu, time ms: %lu\n", stats->curr_len, stats->count, elapsed_ms);
    stats->count = 0;
}

void on_sample(const z_loaned_sample_t *sample, void *context) {
    z_stats_t *stats = (z_stats_t *)context;
    const z_loaned_bytes_t *payload = z_sample_payload(sample);

    if (stats->curr_len != payload->len) {
        // End previous measurement
        z_stats_stop(stats);
        // Check for end packet
        stats->curr_len = (unsigned long)payload->len;
        if (payload->len == 1) {
            test_end = true;
            return;
        }
        // Start new measurement
        printf("Starting test for pkt len: %lu\n", stats->curr_len);
        stats->start = z_clock_now();
    }
    stats->count++;
}

int main(int argc, char **argv) {
    char *keyexpr = "test/thr";
    const char *mode = NULL;
    char *llocator = NULL;
    char *clocator = NULL;
    (void)argv;

    // Check if peer or client mode
    if (argc > 1) {
        mode = "peer";
        llocator = "udp/224.0.0.224:7447#iface=lo";
    } else {
        mode = "client";
        clocator = "tcp/127.0.0.1:7447";
    }
    // Set config
    z_owned_config_t config;
    z_config_default(&config);
    if (mode != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, mode);
    }
    if (llocator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, llocator);
    }
    if (clocator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, clocator);
    }
    // Open session
    z_owned_session_t s;
    if (z_open(&s, z_move(config)) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        exit(-1);
    }
    // Declare Subscriber/resource
    z_owned_closure_sample_t callback;
    z_closure(&callback, on_sample, NULL, (void *)&test_stats);
    z_owned_subscriber_t sub;
    if (z_declare_subscriber(&sub, z_loan(s), z_keyexpr(keyexpr), z_move(callback), NULL) < 0) {
        printf("Unable to create subscriber.\n");
        exit(-1);
    }
    // Listen until stopped
    printf("Start listening.\n");
    while (!test_end) {
    }
    // Wait for everything to settle
    printf("End of test\n");
    z_sleep_s(1);
    // Clean up
    z_undeclare_subscriber(z_move(sub));
    zp_stop_read_task(z_loan_mut(s));
    zp_stop_lease_task(z_loan_mut(s));
    z_close(z_move(s));
    exit(0);
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this test requires it.\n");
    return -2;
}
#endif
