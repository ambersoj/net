I'm not sure if I really was supposed to (according to your recommendations) but I added the line that commits the sequence number into the code:

    libnet_build_icmpv4_echo(
        regs_.icmp4_type,
        regs_.icmp4_code,
        0,
        regs_.icmp4_id,
        ++regs_.icmp4_seq,
        payload,
        payload_len,
        libnet_,
        0
    );

    // ***************** NEW ***********************
    commit("NET.icmp4_seq", true,
        {"icmp4_seq", regs_.icmp4_seq}
    );

So at least for now it is a commit in code, not a _commit in puml.

Also, I added icmp4_seq in the register dump.

Net::serialize_registers()
     .
     .
     .
    // RX / TX status
    j["tx_done"]   = regs_.tx_done;
    j["rx_done"]   = regs_.rx_done;
    j["rx_len"]    = regs_.rx_len;
    j["rx_caplen"] = regs_.rx_caplen;
    j["icmp4_seq"] = regs_.icmp4_seq;
     .
     .
     .

And when I ran start105 for a while and did a couple register dumps it looks as it should:

root@PRED:/usr/local/mpp/fsm# echo '{"verb":"GET"}' | nc -u -w1 127.0.0.1 5000
{"component":"NET","sba":5000,"libnet_device":"eno1","pcap_device":"eno1","libnet_live":true,"pcap_live":true,"tx_done":true,"rx_done":true,"rx_len":60,"rx_caplen":60,"icmp4_seq":19,"last_error":""}
root@PRED:/usr/local/mpp/fsm# echo '{"verb":"GET"}' | nc -u -w1 127.0.0.1 5000
{"component":"NET","sba":5000,"libnet_device":"eno1","pcap_device":"eno1","libnet_live":true,"pcap_live":true,"tx_done":true,"rx_done":false,"rx_len":60,"rx_caplen":60,"icmp4_seq":27,"last_error":""}

with the first one at ping number 19 and the next one at 27.


