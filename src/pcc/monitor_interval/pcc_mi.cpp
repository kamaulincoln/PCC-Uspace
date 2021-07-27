#ifdef QUIC_PORT
#ifdef QUIC_PORT_LOCAL
#include "net/quic/core/congestion_control/pcc_monitor_interval.h"
#else
#include "net/quic/core/congestion_control/pcc_monitor_interval.h"
#endif
#else
#include "pcc_mi.h"
#endif

#ifndef QUIC_PORT
typedef int32_t QuicPacketCount;
typedef int32_t QuicPacketNumber;
typedef int64_t QuicByteCount;
typedef int64_t QuicTime;
typedef double  QuicBandwidth;
#endif

#ifdef QUIC_PORT
#ifdef QUIC_PORT_LOCAL
namespace net {
#else
namespace gfe_quic {
#endif
using namespace net;
#endif

PacketRttSample::PacketRttSample() : packet_number(0),
#ifdef QUIC_PORT
                                     rtt(QuicTime::Delta::Zero()) {}
#else
                                     rtt(0) {}
#endif

PacketRttSample::PacketRttSample(QuicPacketNumber packet_number,
#ifdef QUIC_PORT
                                 QuicTime::Delta rtt)
#else
                                 QuicTime rtt)
#endif
    : packet_number(packet_number),
      rtt(rtt) {}

int MonitorInterval::next_id = 0;
QuicTime first_mi_first_packet_sent_time = 0;

MonitorInterval::MonitorInterval(QuicBandwidth sending_rate, QuicTime start_time, QuicTime end_time,
                                 std::ofstream& packet_log, QuicTime prev_ack_time) {
    this->target_sending_rate = sending_rate;
    this->start_time = start_time;
    this->end_time = end_time;
    bytes_sent = 0;
    bytes_acked = 0;
    bytes_lost = 0;
    n_packets_sent = 0;
    n_packets_accounted_for = 0;
    first_packet_ack_time = 0;
    last_packet_ack_time = 0;
    id = next_id;
    ++next_id;
    pkt_log = &packet_log;
    this->first_packet_ack_time = prev_ack_time;
}

void MonitorInterval::OnPacketSent(QuicTime cur_time, QuicPacketNumber packet_number, QuicByteCount packet_size) {
    if (n_packets_sent == 0) {
        first_packet_sent_time = cur_time;
        first_packet_number = packet_number;
        last_packet_number_accounted_for = first_packet_number - 1;
        //std::cerr << "MI " << id << " started with " << packet_number << ", dur " << (end_time - cur_time) << std::endl;
    }
    std::cerr << "MI " << id << " sent: " << packet_number << ", packet_size=" << packet_size << ", time: " << cur_time << std::endl;
    last_packet_sent_time = cur_time;
    last_packet_number = packet_number;
    ++n_packets_sent;
    bytes_sent += packet_size;
    // std::cerr << "bytes_send=" << bytes_sent << ", n_packets_sent=" << n_packets_sent << std::endl;
    if (first_mi_first_packet_sent_time == 0) {
        first_mi_first_packet_sent_time = cur_time;
    }


    if (pkt_log != NULL && pkt_log->is_open()) {
        *pkt_log << (cur_time - first_mi_first_packet_sent_time) / 1000000.0 << ","
                    << packet_number << ","
                    << "sent" << ","
                    << packet_size << ","
                    << (start_time - first_mi_first_packet_sent_time) / 1000000.0 << ","
                    << (end_time - first_mi_first_packet_sent_time) / 1000000.0 << std::endl;
    }

}

void MonitorInterval::OnPacketAcked(QuicTime cur_time, QuicPacketNumber packet_number, QuicByteCount packet_size, QuicTime rtt) {
    std::cerr << "MI " << id << " ack " << packet_number << std::endl;
    if (rtt != 0) {

    bytes_acked += packet_size;
    packet_rtt_samples.push_back(PacketRttSample(packet_number, rtt));
    if (first_packet_ack_time == 0) {
        // first_packet_ack_time = cur_time;
        std::cerr << "MI " << id << " first ack " << packet_number << std::endl;
        std::cerr << "\tAck time: " << first_packet_ack_time << std::endl;
    }
    last_packet_ack_time = cur_time;
    // if (ContainsPacket(packet_number) && packet_number > last_packet_number_accounted_for) {
    //     int skipped = (packet_number - last_packet_number_accounted_for) - 1;
    //     bytes_acked += packet_size;
    //     n_packets_accounted_for += skipped + 1;
    //     packet_rtt_samples.push_back(PacketRttSample(packet_number, rtt));
    //     last_packet_number_accounted_for = packet_number;
    //     last_packet_ack_time = cur_time;
    //     std::cerr << "MI " << id << " got ack " << packet_number << ", rtt=" << rtt/1000000.0 << "s" << std::endl; //std::cerr << "\tAck time: " << cur_time << std::endl;
    // } else if (packet_number > last_packet_number) {
    //     n_packets_accounted_for = n_packets_sent;
    //     last_packet_number_accounted_for = last_packet_number;
    // }
    // if (packet_number >= first_packet_number && first_packet_ack_time == 0) {
    //     first_packet_ack_time = cur_time;
    //     //std::cerr << "MI " << id << " first ack " << packet_number << std::endl;
    //     //std::cerr << "\tAck time: " << cur_time << std::endl;
    // }
    // if (packet_number >= last_packet_number && last_packet_ack_time == 0) {
    //     last_packet_ack_time = cur_time;
    //     //std::cerr << "MI " << id << " last ack " << packet_number << std::endl;
    //     //std::cerr << "\tAck time: " << cur_time << std::endl;
    // }
    if (pkt_log != NULL && pkt_log->is_open()) {
        *pkt_log << (cur_time - first_mi_first_packet_sent_time) / 1000000.0 << ","
                    << packet_number << ","
                    << "acked" << ","
                    << packet_size <<","
                    << (start_time - first_mi_first_packet_sent_time) / 1000000.0 << ","
                    << (end_time - first_mi_first_packet_sent_time) / 1000000.0 << ","
                    << rtt
                    << std::endl;
    }
    }
}

void MonitorInterval::OnPacketLost(QuicTime cur_time, QuicPacketNumber packet_number, QuicByteCount packet_size) {
    bytes_lost += packet_size;
    if (first_packet_ack_time == 0) {
        first_packet_ack_time = cur_time;
    }
    last_packet_ack_time = cur_time;
    // if (ContainsPacket(packet_number) && packet_number > last_packet_number_accounted_for) {
    //     int skipped = (packet_number - last_packet_number_accounted_for) - 1;
    //     bytes_lost += packet_size;
    //     n_packets_accounted_for += skipped + 1;
    //     last_packet_number_accounted_for = packet_number;
    // } else if (packet_number > last_packet_number) {
    //     n_packets_accounted_for = n_packets_sent;
    //     last_packet_number_accounted_for = last_packet_number;
    // }
    // if (packet_number >= first_packet_number && first_packet_ack_time == 0) {
    //     first_packet_ack_time = cur_time;
    // }
    // if (packet_number >= last_packet_number && last_packet_ack_time == 0) {
    //     last_packet_ack_time = cur_time;
    // }
    if (pkt_log!=NULL && pkt_log->is_open()) {
        *pkt_log << (cur_time - first_mi_first_packet_sent_time) / 1000000.0 << ","
                    << packet_number << ","
                    << "lost" << ","
                    << (start_time - first_mi_first_packet_sent_time) / 1000000.0 << ","
                    << (end_time - first_mi_first_packet_sent_time) / 1000000.0 << std::endl;
    }
}

bool MonitorInterval::AllPacketsSent(QuicTime cur_time) const {
    //std::cout << "Checking if all packets sent: " << cur_time << " >= " << end_time << std::endl;
    return (cur_time >= end_time); // && (packet_rtt_samples.size() > 0);
}

bool MonitorInterval::HasPacketAck() const {
  // return true;
  return bytes_acked > 0 || bytes_lost > 0;
}

bool MonitorInterval::AllPacketsAccountedFor(QuicTime cur_time) {
    bool stop = AllPacketsSent(cur_time) && HasPacketAck(); //  && (n_packets_accounted_for == n_packets_sent);
    end_time = cur_time;
    return stop;
}

QuicTime MonitorInterval::GetStartTime() const {
    // return first_packet_sent_time;
    return start_time;
}

QuicTime MonitorInterval::GetEndTime() const {
    return end_time;
}

QuicBandwidth MonitorInterval::GetTargetSendingRate() const {
    return target_sending_rate;
}

QuicBandwidth MonitorInterval::GetObsThroughput() const {
    float dur = GetObsRecvDur();
    //std::cout << "Num packets: " << n_packets_sent << std::endl;
    //std::cout << "Dur: " << dur << " Acked: " << bytes_acked << std::endl;
    if (dur == 0) {
        return 0;
    }
    return 8 * bytes_acked / (dur / 1000000.0);
}

QuicBandwidth MonitorInterval::GetObsSendingRate() const {
    float dur = GetObsSendDur();
    if (dur == 0) {
        return 0;
    }
    return 8 * bytes_sent / (dur / 1000000.0);
}

float MonitorInterval::GetObsSendDur() const {
    return (last_packet_sent_time - first_packet_sent_time);
}

float MonitorInterval::GetObsRecvDur() const {
    //std::cerr << "MI " << id << "\n";
    //std::cerr << "\tfirst ack " << first_packet_ack_time << "\n\tlast ack " << last_packet_ack_time << "\n";
    return (last_packet_ack_time - first_packet_ack_time);
}

float MonitorInterval::GetObsRtt() const {
    if (packet_rtt_samples.empty()) {
        return 0;
    }
    double rtt_sum = 0.0;
    for (const PacketRttSample& sample : packet_rtt_samples) {
        rtt_sum += sample.rtt;
    }
    return rtt_sum / packet_rtt_samples.size();
}

float MonitorInterval::GetObsRttInflation() const {
    if (packet_rtt_samples.size() < 2) {
        return 0;
    }
    float send_dur = GetObsSendDur();
    if (send_dur == 0) {
        return 0;
    }
    float first_half_rtt_sum = 0;
    float second_half_rtt_sum = 0;
    int half_count = packet_rtt_samples.size() / 2;
    for (int i = 0; i < 2 * half_count; ++i) {
        if (i < half_count) {
            first_half_rtt_sum += packet_rtt_samples[i].rtt;
        } else {
            second_half_rtt_sum += packet_rtt_samples[i].rtt;
        }
    }
    float rtt_inflation = (second_half_rtt_sum - first_half_rtt_sum) / (half_count * send_dur);
    return rtt_inflation;
}

float MonitorInterval::GetObsLossRate() const {
    return 1.0 - (bytes_acked / (float)bytes_sent);
}

void MonitorInterval::SetUtility(float new_utility) {
    utility = new_utility;
}

float MonitorInterval::GetObsUtility() const {
    return utility;
}

uint64_t MonitorInterval::GetFirstAckLatency() const {
    if (packet_rtt_samples.size() > 0) {
        return packet_rtt_samples.front().rtt;
    }
    return 0;
}

uint64_t MonitorInterval::GetLastAckLatency() const {
    if (packet_rtt_samples.size() > 0) {
        return packet_rtt_samples.back().rtt;
    }
    return 0;
}

bool MonitorInterval::ContainsPacket(QuicPacketNumber packet_number) {
    return (packet_number >= first_packet_number && packet_number <= last_packet_number);
}
const std::vector<PacketRttSample>& MonitorInterval::GetRTTSamples() const {
    return packet_rtt_samples;
}

#ifdef QUIC_PORT
} // namespace gfe_quic
#endif
