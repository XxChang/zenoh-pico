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

#include <Arduino.h>
#include <WiFi.h>
#include <zenoh-pico.h>

#if Z_FEATURE_QUERYABLE == 1
// WiFi-specific parameters
#define SSID "SSID"
#define PASS "PASS"

#define CLIENT_OR_PEER 0  // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define CONNECT ""  // If empty, it will scout
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define CONNECT "udp/224.0.0.225:7447#iface=en0"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/zenoh-pico-queryable"
#define VALUE "[ARDUINO]{ESP32} Queryable from Zenoh-Pico!"

void query_handler(const z_loaned_query_t *query, void *arg) {
    z_owned_str_t keystr;
    z_keyexpr_to_string(z_query_keyexpr(query), &keystr);

    Serial.print(" >> [Queryable handler] Replying Data ('");
    Serial.print(z_str_loan(&keystr));
    Serial.print("': '");
    Serial.print(VALUE);
    Serial.println("')");

    z_query_reply(query, z_keyexpr(KEYEXPR), (const unsigned char *)VALUE, strlen(VALUE), NULL);

    z_str_drop(z_str_move(&keystr));
}

void setup() {
    // Initialize Serial for debug
    Serial.begin(115200);
    while (!Serial) {
        delay(1000);
    }

    // Set WiFi in STA mode and trigger attachment
    Serial.print("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    Serial.println("OK");

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, CONNECT);
    }

    // Open Zenoh session
    Serial.print("Opening Zenoh Session...");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config)) < 0) {
        Serial.println("Unable to open session!");
        while (1) {
            ;
        }
    }
    Serial.println("OK");

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_session_loan_mut(&s), NULL);
    zp_start_lease_task(z_session_loan_mut(&s), NULL);

    // Declare Zenoh queryable
    Serial.print("Declaring Queryable on ");
    Serial.print(KEYEXPR);
    Serial.println(" ...");
    z_owned_closure_query_t callback;
    z_closure_query(&callback, query_handler, NULL, NULL);
    z_owned_queryable_t qable;
    if (z_declare_queryable(&qable, z_session_loan(&s), z_keyexpr(KEYEXPR), z_closure_query_move(&callback), NULL) <
        0) {
        Serial.println("Unable to declare queryable.");
        while (1) {
            ;
        }
    }
    Serial.println("OK");
    Serial.println("Zenoh setup finished!");

    delay(300);
}

void loop() { delay(1000); }

#else
void setup() {
    Serial.println("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.");
    return;
}
void loop() {}
#endif
