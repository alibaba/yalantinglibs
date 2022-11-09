#pragma once
#include "config.hpp"
#include <iostream>

template<typename BaselineSample, typename ContenderSample>
void report_cmp_serialize(const std::string& tag, const BaselineSample& baseline, const ContenderSample& contender) {
  std::cout << tag << " ";
  std::cout << pretty_name(baseline.name()) << "   serialize is ";
  std::cout << pretty_float((double)get_avg(contender.serialize_cost_) / get_avg(baseline.serialize_cost_));
  std::cout << " times faster than " << pretty_name(contender.name());
  std::cout << std::endl;
}
template<typename BaselineSample, typename ContenderSample>
void report_cmp_deserialize(const std::string& tag, const BaselineSample& baseline, const ContenderSample& contender) {
  std::cout << tag << " ";
  std::cout << pretty_name(baseline.name()) << " deserialize is ";
  std::cout << pretty_float((double)get_avg(contender.deserialize_cost_) / get_avg(baseline.deserialize_cost_));
  std::cout << " times faster than " << pretty_name(contender.name());
  std::cout << std::endl;
}

template<typename BaselineSample, typename ...ContenderSamples>
void bench(std::string tag, BaselineSample baseline, ContenderSamples ...contenders) {
  std::cout << "------- start benchmark " << tag << " -------" << std::endl;
  baseline.clear_buffer();
  (contenders.clear_buffer(), ...);
  baseline.reserve_buffer();
  (contenders.reserve_buffer(), ...);
  for (int i = 0; i < RUN_COUNT; ++i) {
    baseline.clear_buffer();
    (contenders.clear_buffer(), ...);
    baseline.do_serialization(i);
    (contenders.do_serialization(i), ...);
  }
  for (int i = 0; i < RUN_COUNT; ++i) {
    baseline.clear_data();
    (contenders.clear_data(), ...);
    baseline.do_deserialization(i);
    (contenders.do_deserialization(i), ...);
  }
  baseline.report_metric(tag);
  (contenders.report_metric(tag), ...);
  (report_cmp_serialize(tag, baseline, contenders), ...);
  (report_cmp_deserialize(tag, baseline, contenders), ...);
  std::cout << "------- end benchmark   " << tag << " -------";
  std::cout << std::endl;
  std::cout << std::endl;
}
