#pragma once
#include "hbridge.h"

struct HBridgeChannelId
{
public:

    HBridgeChannelId(u_int8_t device_id, HBridgeChannel channel) {
        this->device_id = device_id;
        this->channel = channel;
    }  


    static HBridgeChannelId ID_AB1() {
         return HBridgeChannelId(0, HBridgeChannel::AB);
    }

    static HBridgeChannelId ID_A1() {
         return HBridgeChannelId(0, HBridgeChannel::A);
    }

    static HBridgeChannelId ID_B1() {
         return HBridgeChannelId(0, HBridgeChannel::B);
    }

    static HBridgeChannelId ID_AB2() {
         return HBridgeChannelId(1, HBridgeChannel::AB);
    }

    static HBridgeChannelId ID_A2() {
         return HBridgeChannelId(1, HBridgeChannel::A);
    }

    static HBridgeChannelId ID_B2() {
         return HBridgeChannelId(1, HBridgeChannel::B);
    }

    HBridgeChannel channel;
    u_int8_t device_id;
};
