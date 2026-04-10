#include "Net.hpp"

#include <cstring>
#include <arpa/inet.h>

// -----------------------------------------------------------------------------
// Construction / Destruction
// -----------------------------------------------------------------------------
Net::Net(int sba)
    : mpp::Component<Net>(sba)
{
    regs_.sba_ = sba;
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
    j["sba"]       = regs_.sba_;

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
    j["icmp4_seq"] = regs_.icmp4_seq;

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
    // eof
    // --------------------
    if (j.contains("eof"))           regs_.eof           = j["eof"];

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

    // --------------------
    // Actions
    // --------------------
    if (j.contains("net_rx_enable"))
        regs_.net_rx_enable = j["net_rx_enable"];

    if (j.value("tx_fire", false)) {
        regs_.tx_fire = false; // consume tx_fire
        do_tx();
    }

    if (j.value("rx_fire", false)) {
        regs_.rx_fire = false; // consume rx_fire
        do_rx();
    }
    if (j.contains("net_rx_enable")) {
        bool en = j["net_rx_enable"];
        regs_.net_rx_enable = en;

        if (en && !rx_thread_running) {
            rx_thread_running = true;
            rx_thread_ = std::thread([this]() {
                while (rx_thread_running) {
                    do_rx();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }

        if (!en && rx_thread_running) {
            rx_thread_running = false;
            if (rx_thread_.joinable())
                rx_thread_.join();
        }
    }

    if (!j.contains("verb"))
        return;

    const std::string verb = j["verb"];

    if (verb == "GET") {
        reply_json(serialize_registers());
        return;
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

    pcap_setnonblock(pcap_, 1, errbuf);
    
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

    XfrHeader hdr;
    hdr.magic = htonl(0x58465230); // "XFR0"
    hdr.seq   = htonl(regs_.icmp4_seq);
    hdr.len   = htonl(regs_.icmp4_payload.size());
    hdr.flags = htonl(regs_.eof ? 0x01 : 0x00);

    std::vector<uint8_t> packet(sizeof(XfrHeader));
    memcpy(packet.data(), &hdr, sizeof(hdr));

    packet.insert(
        packet.end(),
        regs_.icmp4_payload.begin(),
        regs_.icmp4_payload.end()
    );

    const uint8_t* payload = packet.data();
    uint32_t payload_len   = packet.size();

    libnet_build_icmpv4_echo(
        regs_.icmp4_type,
        regs_.icmp4_code,
        0,
        regs_.icmp4_id,
        regs_.icmp4_seq,
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

    json j;
    j["component"] = "NET";
    j["tx_done"] = regs_.tx_done;

    send_json(j, regs_.fsm_sba_);

    regs_.tx_done = false; // consume tx_done
}

// -----------------------------------------------------------------------------
// RX (PCAP sample)
// -----------------------------------------------------------------------------
void Net::do_rx()
{
    if (!pcap_)
        return;

    struct pcap_pkthdr* pcap_hdr = nullptr;
    const u_char* data = nullptr;

    int rc = pcap_next_ex(pcap_, &pcap_hdr, &data);
    if (rc <= 0)
        return;

    constexpr int ETH_HDR  = 14;
    constexpr int IP_HDR   = 20;
    constexpr int ICMP_HDR = 8;

    int icmp_offset = ETH_HDR + IP_HDR;
    int payload_offset = icmp_offset + ICMP_HDR;

    if (pcap_hdr->caplen < payload_offset)
        return;

    const uint8_t* payload =
        (const uint8_t*)(data + payload_offset);

    int payload_len =
        pcap_hdr->caplen - payload_offset;

    if (payload_len < (int)sizeof(XfrHeader))
        return;

    XfrHeader xhdr;
    memcpy(&xhdr, payload, sizeof(xhdr));

    if (ntohl(xhdr.magic) != 0x58465230)
        return;

    uint32_t seq   = ntohl(xhdr.seq);
    uint32_t len   = ntohl(xhdr.len);
    uint32_t flags = ntohl(xhdr.flags);

    regs_.eof = (flags & 0x01) != 0;

    const char* data_ptr =
        reinterpret_cast<const char*>(payload + sizeof(XfrHeader));

    regs_.buffer.assign(data_ptr, len);
    regs_.rx_len = len;

    json out;
    out["component"] = "NET";
    out["rx_done"] = true;
    out["seq"] = seq;
    out["buffer"] = regs_.buffer;
    out["len"] = len;
    out["eof"] = regs_.eof;

    send_json(out, regs_.fsm_sba_);

    regs_.rx_done = false; // Consume rx_done
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



