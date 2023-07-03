#include "zenoh-pico/protocol/core.h"

uint8_t _z_id_len(_z_id_t id) {
    uint8_t len = 16;
    while (len > 0) {
        --len;
        if (id.id[len] != 0) {
            break;
        }
    }
    return len;
}
_Bool _z_id_check(_z_id_t id) {
    _Bool ret = false;
    for (int i = 0; !ret && i < 16; i++) {
        ret |= id.id[i] != 0;
    }
    return ret;
}
_z_id_t _z_id_empty() {
    return (_z_id_t){.id = {
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                     }};
}
_z_source_info_t _z_source_info_null() {
    return (_z_source_info_t){._source_sn = 0, ._entity_id = 0, ._id = _z_id_empty()};
}
_z_timestamp_t _z_timestamp_null() { return (_z_timestamp_t){.id = _z_id_empty(), .time = 0}; }
