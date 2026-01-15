#pragma once

#include "Component.hpp"

#include <string>
#include <netinet/in.h>

#include <pcap/pcap.h>
#include <libnet.h>

using json = nlohmann::ordered_json;

// -----------------------------------------------------------------------------
// NET Registers (Canonical MPP Form)
// -----------------------------------------------------------------------------
struct NetRegisters
{
    // Identity
    int sba = 0;

    // Lifecycle
    bool libnet_create  = false;
    bool libnet_destroy = false;
    bool pcap_create    = false;
    bool pcap_destroy   = false;

    // TX / RX triggers
    bool tx_fire = false;
    bool rx_fire = false;

    // Devices
    std::string libnet_device = "eno1";
    std::string pcap_device   = "eno1";

    // PCAP configuration
    int snaplen = 65535;
    bool promisc = true;
    int timeout_ms = 10;
    std::string pcap_filter;
    bool pcap_set_filter = false;

    // Ethernet (libnet)
    bool eth_enabled = false;
    std::string eth_src_mac;
    std::string eth_dst_mac;
    uint16_t eth_type = ETHERTYPE_IP;

    // IPv4 (libnet)
    bool ip4_enabled = false;
    std::string ip4_src;
    std::string ip4_dst;
    uint8_t ip4_ttl = 64;

    // ICMPv4 (libnet)
    bool icmp4_enabled = false;
    uint8_t  icmp4_type = ICMP_ECHO;
    uint8_t  icmp4_code = 0;
    uint16_t icmp4_id   = 0x1234;
    uint16_t icmp4_seq  = 0;
    std::string icmp4_payload;

    // RX status (published)
    bool rx_done = false;
    uint32_t rx_len = 0;
    uint32_t rx_caplen = 0;

    // TX status (optional but recommended)
    bool tx_done = false;

    // Errors
    std::string last_error;
};

// -----------------------------------------------------------------------------
// NET Component
// -----------------------------------------------------------------------------
class Net : public mpp::Component<Net>
{
public:
    explicit Net(int sba);
    ~Net();

    // MPP interface
    json serialize_registers() const;
    void apply_snapshot(const json& j);
    void on_message(const json& j);
    void on_parse_error(const json::parse_error& e);
    void on_unknown_parse_error();

protected:
    const char* component_name() const override { return "NET"; }

private:
    // Handles
    libnet_t* libnet_ = nullptr;
    pcap_t*   pcap_   = nullptr;

    NetRegisters regs_;

    // Lifecycle helpers
    void do_libnet_create();
    void do_libnet_destroy();
    void do_pcap_create();
    void do_pcap_destroy();

    // Data plane
    void do_tx();
    void do_rx();

    // Errors
    void set_error(const std::string& msg);
};

#define NET_ERROR(obj, msg) \
    (obj).set_last_error((msg), __FILE__, __LINE__, __func__)

static bool parse_mac(const std::string& s, uint8_t mac[6])
{
    unsigned int b[6];
    if (sscanf(s.c_str(),
               "%x:%x:%x:%x:%x:%x",
               &b[0], &b[1], &b[2],
               &b[3], &b[4], &b[5]) != 6)
        return false;

    for (int i = 0; i < 6; ++i)
        mac[i] = static_cast<uint8_t>(b[i]);

    return true;
}

static std::string mac_to_string(const uint8_t mac[6])
{
    char buf[32];
    snprintf(buf, sizeof(buf),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    return buf;
}
