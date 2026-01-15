#include "Net.hpp"

#include <cstring>
#include <arpa/inet.h>

// -----------------------------------------------------------------------------
// Construction / Destruction
// -----------------------------------------------------------------------------
Net::Net(int sba)
    : mpp::Component<Net>(sba, /*publish_period_ms=*/0, /*listen_bus=*/true)
{
    regs_.sba = sba;
}

Net::~Net()
{
    do_pcap_destroy();
    do_libnet_destroy();
}

// -----------------------------------------------------------------------------
// Serialization (READ)
// -----------------------------------------------------------------------------
json Net::serialize_registers() const
{
    json j;
    j["component"] = "NET";
    j["sba"]       = regs_.sba;

    // Devices
    j["libnet_device"] = regs_.libnet_device;
    j["pcap_device"]   = regs_.pcap_device;

    // Live status
    j["libnet_live"] = (libnet_ != nullptr);
    j["pcap_live"]   = (pcap_ != nullptr);

    // RX / TX status
    j["tx_done"]   = regs_.tx_done;
    j["rx_done"]   = regs_.rx_done;
    j["rx_len"]    = regs_.rx_len;
    j["rx_caplen"] = regs_.rx_caplen;

    // Errors
    j["last_error"] = regs_.last_error;

    return j;
}

// -----------------------------------------------------------------------------
// Apply Snapshot (WRITE)
// -----------------------------------------------------------------------------
void Net::apply_snapshot(const json& j)
{
    // --------------------
    // Configuration
    // --------------------
    if (j.contains("libnet_device")) regs_.libnet_device = j["libnet_device"];
    if (j.contains("pcap_device"))   regs_.pcap_device   = j["pcap_device"];

    if (j.contains("snaplen"))       regs_.snaplen       = j["snaplen"];
    if (j.contains("promisc"))       regs_.promisc       = j["promisc"];
    if (j.contains("timeout_ms"))    regs_.timeout_ms    = j["timeout_ms"];
    if (j.contains("pcap_filter"))   regs_.pcap_filter   = j["pcap_filter"];

    if (j.contains("eth_enabled"))   regs_.eth_enabled   = j["eth_enabled"];
    if (j.contains("eth_src_mac"))   regs_.eth_src_mac   = j["eth_src_mac"];
    if (j.contains("eth_dst_mac"))   regs_.eth_dst_mac   = j["eth_dst_mac"];
    if (j.contains("eth_type"))      regs_.eth_type      = j["eth_type"];

    if (j.contains("ip4_enabled"))   regs_.ip4_enabled   = j["ip4_enabled"];
    if (j.contains("ip4_src"))       regs_.ip4_src       = j["ip4_src"];
    if (j.contains("ip4_dst"))       regs_.ip4_dst       = j["ip4_dst"];
    if (j.contains("ip4_ttl"))       regs_.ip4_ttl       = j["ip4_ttl"];

    if (j.contains("icmp4_enabled")) regs_.icmp4_enabled = j["icmp4_enabled"];
    if (j.contains("icmp4_type"))    regs_.icmp4_type    = j["icmp4_type"];
    if (j.contains("icmp4_code"))    regs_.icmp4_code    = j["icmp4_code"];
    if (j.contains("icmp4_id"))      regs_.icmp4_id      = j["icmp4_id"];
    if (j.contains("icmp4_seq"))     regs_.icmp4_seq     = j["icmp4_seq"];
    if (j.contains("icmp4_payload")) regs_.icmp4_payload = j["icmp4_payload"];

    // --------------------
    // Lifecycle
    // --------------------
    if (j.value("libnet_create", false))  do_libnet_create();
    if (j.value("libnet_destroy", false)) do_libnet_destroy();
    if (j.value("pcap_create", false))    do_pcap_create();
    if (j.value("pcap_destroy", false))   do_pcap_destroy();

    // --------------------
    // PCAP filter
    // --------------------
    if (j.value("pcap_set_filter", false) && pcap_) {
        if (!regs_.pcap_filter.empty()) {
            struct bpf_program fp{};
            if (pcap_compile(
                    pcap_, &fp,
                    regs_.pcap_filter.c_str(),
                    1, PCAP_NETMASK_UNKNOWN) == 0) {
                pcap_setfilter(pcap_, &fp);
                pcap_freecode(&fp);
            }
        }
    }

    if (j.value("read", false)) {
        reply_json(serialize_registers());
    }

    // --------------------
    // Actions
    // --------------------
    if (j.value("tx_fire", false)) {
        regs_.tx_done = false;
        do_tx();
    }

    if (j.value("rx_fire", false)) {
        regs_.rx_done = false;
        do_rx();
    }
    if (j.value("tick", false)) {
        do_rx();
    }
}

// -----------------------------------------------------------------------------
// BUS message handling
// -----------------------------------------------------------------------------
void Net::on_message(const json& j)
{

}

// -----------------------------------------------------------------------------
// libnet lifecycle
// -----------------------------------------------------------------------------
void Net::do_libnet_create()
{
    if (libnet_)
        return;

    char errbuf[LIBNET_ERRBUF_SIZE]{};

    libnet_ = libnet_init(
        LIBNET_LINK,
        regs_.libnet_device.c_str(),
        errbuf
    );

    if (!libnet_)
        set_error(errbuf);
}

void Net::do_libnet_destroy()
{
    if (!libnet_)
        return;

    libnet_destroy(libnet_);
    libnet_ = nullptr;
}

// -----------------------------------------------------------------------------
// pcap lifecycle
// -----------------------------------------------------------------------------
void Net::do_pcap_create()
{
    if (pcap_)
        return;

    char errbuf[PCAP_ERRBUF_SIZE]{};

    pcap_ = pcap_open_live(
        regs_.pcap_device.c_str(),
        regs_.snaplen,
        regs_.promisc,
        regs_.timeout_ms,
        errbuf
    );

    if (!pcap_) {
        set_error(errbuf);
        return;
    }
}

void Net::do_pcap_destroy()
{
    if (!pcap_)
        return;

    pcap_close(pcap_);
    pcap_ = nullptr;
}

// -----------------------------------------------------------------------------
// TX (ICMP Echo)
// -----------------------------------------------------------------------------
void Net::do_tx()
{
    if (!libnet_)
        return;

    if (!regs_.eth_enabled || !regs_.ip4_enabled || !regs_.icmp4_enabled)
        return;

    uint8_t eth_src[6], eth_dst[6];
    if (!parse_mac(regs_.eth_src_mac, eth_src) ||
        !parse_mac(regs_.eth_dst_mac, eth_dst)) {
        set_error("invalid MAC address format");
        return;
    }

    uint32_t src_ip =
        libnet_name2addr4(libnet_, (char*)regs_.ip4_src.c_str(), LIBNET_DONT_RESOLVE);
    uint32_t dst_ip =
        libnet_name2addr4(libnet_, (char *)regs_.ip4_dst.c_str(), LIBNET_DONT_RESOLVE);

    if (src_ip == 0 || dst_ip == 0) {
        set_error("invalid IP address");
        return;
    }

    const uint8_t* payload =
        reinterpret_cast<const uint8_t*>(regs_.icmp4_payload.data());
    uint32_t payload_len = regs_.icmp4_payload.size();

    libnet_build_icmpv4_echo(
        regs_.icmp4_type,
        regs_.icmp4_code,
        0,
        regs_.icmp4_id,
        regs_.icmp4_seq++,
        payload,
        payload_len,
        libnet_,
        0
    );

    libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_ICMPV4_ECHO_H + payload_len,
        0,
        libnet_get_prand(LIBNET_PRu16),
        0,
        regs_.ip4_ttl,
        IPPROTO_ICMP,
        0,
        src_ip,
        dst_ip,
        nullptr,
        0,
        libnet_,
        0
    );

    libnet_build_ethernet(
        eth_dst,
        eth_src,
        regs_.eth_type,
        nullptr,
        0,
        libnet_,
        0
    );

    if (libnet_write(libnet_) < 0) {
        set_error(libnet_geterror(libnet_));
        return;
    }

    libnet_clear_packet(libnet_);
    regs_.tx_done = true;

    commit("NET.tx_done", true);
}

// -----------------------------------------------------------------------------
// RX (PCAP sample)
// -----------------------------------------------------------------------------
void Net::do_rx()
{
    if (!pcap_)
        return;

    struct pcap_pkthdr* hdr = nullptr;
    const u_char* data = nullptr;

    int rc = pcap_next_ex(pcap_, &hdr, &data);
    if (rc <= 0)
        return;

    regs_.rx_done   = true;
    regs_.rx_len    = hdr->len;
    regs_.rx_caplen = hdr->caplen;

    commit("NET.rx_done", true, {
        {"rx_len", regs_.rx_len},
        {"rx_caplen", regs_.rx_caplen}
    });

    json j;
    j["component"] = "NET";
    j["rx_done"]   = true;
    j["rx_len"]    = regs_.rx_len;
    j["rx_caplen"] = regs_.rx_caplen;

}

// -----------------------------------------------------------------------------
// Errors
// -----------------------------------------------------------------------------
void Net::on_parse_error(const json::parse_error& e)
{
    set_error(e.what());
}

void Net::on_unknown_parse_error()
{
    set_error("unknown JSON parse error");
}

void Net::set_error(const std::string& msg)
{
    regs_.last_error = msg;
}
