#ifdef QUIC_PORT
#ifdef QUIC_PORT_LOCAL
#include "net/quic/core/congestion_control/pcc_sender.h"
#include "net/quic/core/congestion_control/rtt_stats.h"
#include "net/quic/core/quic_time.h"
#include "net/quic/platform/api/quic_str_cat.h"
#else
#include "third_party/pcc_quic/pcc_sender.h"
#include "/quic/src/core/congestion_control/rtt_stats.h"
#include "/quic/src/net/platform/api/quic_str_cat.h"
#include "base_commandlineflags.h"
#endif
#else
#include "pcc_sender.h"
#include "stdlib.h"
#include <random>
#endif

#ifdef QUIC_PORT
#define UDT_UNUSED
#else
#define UDT_UNUSED __attribute__((unused))
#endif

#include <algorithm>

#ifdef QUIC_PORT
#ifdef QUIC_PORT_LOCAL
namespace net {

#else
namespace gfe_quic {

#endif
#endif

namespace {
// Number of bits per Mbit.
const size_t kMegabit = 1024 * 1024;
// Minimum sending rate of the connection.
#ifdef QUIC_PORT
const QuicBandwidth kMinSendingRate = QuicBandwidth::FromKBitsPerSecond(2000);
// The smallest amount that the rate can be changed by at a time.
const QuicBandwidth kMinimumRateChange = QuicBandwidth::FromBitsPerSecond(
    static_cast<int64_t>(0.5f * kMegabit));
#else
const float kNumMicrosPerSecond = 1000000.0f;
// Default TCPMSS used in UDT only.
// const size_t kDefaultTCPMSS = 1420;
const size_t kDefaultTCPMSS = 1500;
// An inital RTT value to use (10ms)
const size_t kInitialRttMicroseconds = 1 * 1000;
#endif
// Number of bits per byte.
const size_t kBitsPerByte = 8;
// Duration of monitor intervals as a proportion of RTT.
// const float kMonitorIntervalDuration = 0.5f;
const float kMonitorIntervalDuration = 1.0f;
// const float kMonitorIntervalDuration = 1.5f;
// Minimum number of packets in a monitor interval.
const size_t kMinimumPacketsPerInterval = 5;
}  // namespace

#ifdef QUIC_PORT
QuicTime::Delta PccSender::ComputeMonitorDuration(
    QuicBandwidth sending_rate,
    QuicTime::Delta rtt) {

  return QuicTime::Delta::FromMicroseconds(
      std::max(1.5 * rtt.ToMicroseconds(),
               kNumMicrosPerSecond * kMinimumPacketsPerInterval * kBitsPerByte *
                   kDefaultTCPMSS / static_cast<float>(
                       sending_rate.ToBitsPerSecond())));
}
#else
QuicTime PccSender::ComputeMonitorDuration(
    QuicBandwidth sending_rate,
    QuicTime rtt, QuicTime prev_dur) {
    std::cerr << "ComputeMonitorDuration: rtt=" << rtt << ", 1.0 * RTT=" <<
        kMonitorIntervalDuration * rtt << ", dur of sending rate=" <<
               kNumMicrosPerSecond * kMinimumPacketsPerInterval * kBitsPerByte *
                   kDefaultTCPMSS / (float)sending_rate << ", prev_dur=" << prev_dur << std::endl;
    if (rtt == 0 && prev_dur == 0)
        return 10000;
        // return kNumMicrosPerSecond * kMinimumPacketsPerInterval * kBitsPerByte *
        //            kDefaultTCPMSS / (float)sending_rate;
    if (rtt == 0)
        return prev_dur;
    // return std::max(kMonitorIntervalDuration * rtt, kNumMicrosPerSecond * kMinimumPacketsPerInterval * kBitsPerByte *
    //                kDefaultTCPMSS / (float)sending_rate);
    return kMonitorIntervalDuration * rtt; // + kNumMicrosPerSecond * kBitsPerByte * kDefaultTCPMSS / (float)sending_rate;
}
#endif

#ifdef QUIC_PORT
PccSender::PccSender(const RttStats* rtt_stats,
                     QuicPacketCount initial_congestion_window,
                     QuicPacketCount max_congestion_window,
                     QuicRandom* random)
#else
PccSender::PccSender(QuicTime initial_rtt_us,
                     QuicPacketCount initial_congestion_window,
                     UDT_UNUSED QuicPacketCount max_congestion_window)
#endif
    :
#ifdef QUIC_PORT
      sending_rate_(QuicBandwidth::FromBitsPerSecond(
          initial_congestion_window * kDefaultTCPMSS * kBitsPerByte *
          kNumMicrosPerSecond / rtt_stats->initial_rtt_us())),
#else
      sending_rate_(1200000),
      // sending_rate_(
      //     initial_congestion_window * kDefaultTCPMSS * kBitsPerByte *
      //     kNumMicrosPerSecond / initial_rtt_us),
#endif
      interval_analysis_group_(3),
      #ifndef QUIC_PORT
      avg_rtt_(0)
      #endif
      {

  // std::cerr << "Starting sending rate = " << sending_rate_ << std::endl;
  #ifndef QUIC_PORT
  if (Options::Get("-log=") == NULL) {
    log = new PccEventLogger("pcc_log.txt");
  } else {
    log = new PccEventLogger(Options::Get("-log="));
  }
  #endif

  // CLARG: "--pcc-utility-calc=<utility_calculator>" See src/pcc/utility for more info.
  const char* uc_name = Options::Get("--pcc-utility-calc=");
  if (uc_name == NULL) {
      utility_calculator_ = PccUtilityCalculatorFactory::Create("", log);
  } else {
      utility_calculator_ = PccUtilityCalculatorFactory::Create(std::string(uc_name), log);
  }

  // We'll tell the rate controller how many times per RTT it is called so it can run aglorithms
  // like doubling every RTT fairly easily.
  double call_freq = 1.0 / kMonitorIntervalDuration;

  // CLARG: "--pcc-rate-control=<rate_controller>" See src/pcc/rate_controler for more info.
  const char* rc_name = Options::Get("--pcc-rate-control=");
  std::string rc_name_str = "";
  if (rc_name != NULL) {
      rc_name_str = std::string(rc_name);
  }
  rate_controller_ = PccRateControllerFactory::Create(rc_name_str, call_freq, log);
    rate_control_lock_ = new std::mutex();

    if (Options::Get("--save-dir=") != NULL) {
        std::string pkt_log_fn = std::string(Options::Get("--save-dir=")) +
            "/packet_log.csv";
        packet_log.open(pkt_log_fn);
    } else {
        std::cerr <<   "--save-dir= is NULL"  << std::endl;
    }
}

#ifndef QUIC_PORT
PccSender::~PccSender() {
    delete log;
    delete utility_calculator_;
    delete rate_controller_;
    delete rate_control_lock_;
    packet_log.close();
}

#endif
#if defined(QUIC_PORT) && defined(QUIC_PORT_LOCAL)
PccSender::~PccSender() {}


#endif
#ifndef QUIC_PORT
void PccSender::Reset() {
    rate_control_lock_->lock();
    rate_controller_->Reset();
    rate_control_lock_->unlock();
}
#endif

bool PccSender::ShouldCreateNewMonitorInterval(QuicTime sent_time) {
    return interval_queue_.Empty() ||
      interval_queue_.Current().AllPacketsSent(sent_time);
}

void PccSender::UpdateCurrentRttEstimate(QuicTime rtt) {
    #ifdef QUIC_PORT
    return;
    #else
    avg_rtt_ = rtt;
    #endif
}

QuicTime PccSender::GetCurrentRttEstimate(QuicTime sent_time) {
    #ifdef QUIC_PORT
    return rtt_stats_->smoothed_rtt();
    #else
    return avg_rtt_;
    #endif
}

QuicBandwidth PccSender::UpdateSendingRate(QuicTime event_time) {
    // rate_control_lock_->lock();
  sending_rate_ = rate_controller_->GetNextSendingRate(sending_rate_, event_time);
    // rate_control_lock_->unlock();
    // std::cerr << "PCC: rate = " << sending_rate_ << std::endl;
  return sending_rate_;
}

void PccSender::OnPacketSent(QuicTime sent_time,
                             UDT_UNUSED QuicByteCount bytes_in_flight,
                             QuicPacketNumber packet_number,
                             QuicByteCount bytes,
                             UDT_UNUSED HasRetransmittableData is_retransmittable) {

  rate_control_lock_->lock();
  if (ShouldCreateNewMonitorInterval(sent_time)) {
      QuicTime rtt_estimate = GetCurrentRttEstimate(sent_time);
      QuicTime start_time = sent_time;
      QuicTime prev_dur = 0;
      while (interval_queue_.HasFinishedInterval(sent_time)) {
        MonitorInterval mi = interval_queue_.Pop();
        start_time = mi.GetEndTime();
        rtt_estimate = mi.GetObsRtt();
        prev_dur = mi.GetEndTime() - mi.GetStartTime();
        // std::cerr << "MI " << mi.GetId() << " Finished, end_time: " << mi.GetEndTime() << ", cur_time: " << sent_time << ", gap: " << (mi.GetEndTime() - sent_time) / 1000000.0 << std::endl;
        mi.SetUtility(utility_calculator_->CalculateUtility(interval_analysis_group_, mi));
        // rate_control_lock_->lock();
        rate_controller_->MonitorIntervalFinished(mi);
        // rate_control_lock_->unlock();
      }
      // std:: cerr << "PccSender OnPacketSent: while loop  finishes" << std::endl;
    // Set the monitor duration to 1.5 of smoothed rtt.
    // QuicTime rtt_estimate = GetCurrentRttEstimate(sent_time);
    float sending_rate = UpdateSendingRate(sent_time);
    QuicTime monitor_duration = ComputeMonitorDuration(sending_rate, rtt_estimate, prev_dur);
    //std::cerr << "\tTime: " << sent_time << std::endl;
    //std::cerr << "\tPacket Number: " << packet_number << std::endl;
    // start_time = sent_time;
    MonitorInterval mi = MonitorInterval(sending_rate, start_time, start_time + monitor_duration, packet_log);
    std::cerr << "MI " << mi.GetId() << " create, Duration: " << monitor_duration/1000000.0 << "s" << ", rtt" << rtt_estimate/1000000.0 << std::endl;
    interval_queue_.Push(mi);
  }
  interval_queue_.OnPacketSent(sent_time, packet_number, bytes);
  rate_control_lock_->unlock();
}

void PccSender::OnCongestionEvent(UDT_UNUSED bool rtt_updated,
                                  UDT_UNUSED QuicByteCount bytes_in_flight,
                                  QuicTime event_time,
  #ifndef QUIC_PORT
                                  QuicTime rtt,
  #endif
                                  const AckedPacketVector& acked_packets,
                                  const LostPacketVector& lost_packets) {
  #ifndef QUIC_PORT
  if (rtt != 0) {
    UpdateCurrentRttEstimate(rtt);
  }
  #endif
  rate_control_lock_->lock();
  int64_t rtt_estimate = GetCurrentRttEstimate(event_time);
  // if (ShouldCreateNewMonitorInterval(event_time)) {
  //     QuicTime start_time = event_time;
  //     QuicTime prev_dur = 0;
  //     while (interval_queue_.HasFinishedInterval(event_time)) {
  //       MonitorInterval mi = interval_queue_.Pop();
  //       start_time = mi.GetEndTime();
  //       rtt_estimate = mi.GetObsRtt();
  //       prev_dur = mi.GetEndTime() - mi.GetStartTime();
  //       // std::cerr << "MI " << mi.GetId() << " Finished, end_time: " << mi.GetEndTime() << ", cur_time: " << event_time << ", gap: " << (mi.GetEndTime() - event_time) / 1000000.0 << std::endl;
  //       mi.SetUtility(utility_calculator_->CalculateUtility(interval_analysis_group_, mi));
  //       // rate_control_lock_->lock();
  //       rate_controller_->MonitorIntervalFinished(mi);
  //       // rate_control_lock_->unlock();
  //     }
  // //     // std:: cerr << "PccSender OnPacketSent: while loop  finishes" << std::endl;
  // //   // Set the monitor duration to 1.5 of smoothed rtt.
  // //   // QuicTime rtt_estimate = GetCurrentRttEstimate(sent_time);
  //   float sending_rate = UpdateSendingRate(event_time);
  //   QuicTime monitor_duration = ComputeMonitorDuration(sending_rate, rtt_estimate, prev_dur);
  //   //std::cerr << "\tTime: " << sent_time << std::endl;
  //   //std::cerr << "\tPacket Number: " << packet_number << std::endl;
  //   // start_time = event_time;
  //   MonitorInterval mi = MonitorInterval(sending_rate, start_time, start_time + monitor_duration, packet_log);
  //   interval_queue_.Push(mi);
  //   std::cerr << "MI " << mi.GetId() << " create, Duration: " << monitor_duration/1000000.0 << "s, start_time=" <<  start_time << ", end_time=" << start_time + monitor_duration << ", rtt" << rtt_estimate/1000000.0 << ", interval queue length:" << interval_queue_.Size() << std::endl;
  // }

  interval_queue_.OnCongestionEvent(acked_packets,
                                    lost_packets,
                                    rtt_estimate,
                                    event_time);
  // while (interval_queue_.HasFinishedInterval(event_time)) {
  //   MonitorInterval mi = interval_queue_.Pop();
  //   std::cerr << "MI " << mi.GetId() << " Finished, end_time: " << mi.GetEndTime() << ", cur_time: " << event_time << std::endl;
  //   mi.SetUtility(utility_calculator_->CalculateUtility(interval_analysis_group_, mi));
  //   rate_control_lock_->lock();
  //   rate_controller_->MonitorIntervalFinished(mi);
  //   rate_control_lock_->unlock();
  // }
  rate_control_lock_->unlock();
}

#ifdef QUIC_PORT
bool PccSender::CanSend(QuicByteCount bytes_in_flight) {
  return true;
}
#endif

QuicBandwidth PccSender::PacingRate(UDT_UNUSED QuicByteCount bytes_in_flight) const {
  QuicBandwidth result = interval_queue_.Empty() ? sending_rate_
                                 : interval_queue_.Current().GetTargetSendingRate();
  return result;
}

#ifdef QUIC_PORT
QuicBandwidth PccSender::BandwidthEstimate() const {
  return QuicBandwidth::Zero();
}

QuicByteCount PccSender::GetCongestionWindow() const {
  // Use smoothed_rtt to calculate expected congestion window except when it
  // equals 0, which happens when the connection just starts.
  int64_t rtt_us = rtt_stats_->smoothed_rtt().ToMicroseconds() == 0
                       ? rtt_stats_->initial_rtt_us()
                       : rtt_stats_->smoothed_rtt().ToMicroseconds();
  return static_cast<QuicByteCount>(sending_rate_.ToBytesPerSecond() * rtt_us /
                                    kNumMicrosPerSecond);
}

bool PccSender::InSlowStart() const { return false; }

bool PccSender::InRecovery() const { return false; }

bool PccSender::IsProbingForMoreBandwidth() const { return false; }

QuicByteCount PccSender::GetSlowStartThreshold() const { return 0; }

CongestionControlType PccSender::GetCongestionControlType() const {
  return kPCC;
}

#ifdef QUIC_PORT_LOCAL
std::string PccSender::GetDebugState() const {
#else
string PccSender::GetDebugState() const {
#endif
  if (interval_queue_.empty()) {
    return "pcc??";
  }

  const MonitorInterval& mi = interval_queue_.current();
  std::string msg = QuicStrCat(
      "[st=", mode_, ",", "r=", sending_rate_.ToKBitsPerSecond(), ",",
      "pu=", QuicStringPrintf("%.15g", latest_utility_info_.utility), ",",
      "dir=", direction_, ",", "round=", rounds_, ",",
      "num=", interval_queue_.num_useful_intervals(), "]",
      "[r=", mi.sending_rate.ToKBitsPerSecond(), ",", "use=", mi.is_useful, ",",
      "(", mi.first_packet_sent_time.ToDebuggingValue(), "-", ">",
      mi.last_packet_sent_time.ToDebuggingValue(), ")", "(",
      mi.first_packet_number, "-", ">", mi.last_packet_number, ")", "(",
      mi.bytes_sent, "/", mi.bytes_acked, "/", mi.bytes_lost, ")", "(",
      mi.rtt_on_monitor_start_us, "-", ">", mi.rtt_on_monitor_end_us, ")");
  return msg;
}
#endif

#ifdef QUIC_PORT
} // namespace gfe_quic
#endif

