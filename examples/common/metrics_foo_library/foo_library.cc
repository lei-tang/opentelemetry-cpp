// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "foo_library.h"
#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <vector>
#include "opentelemetry/context/context.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/nostd/shared_ptr.h"

namespace nostd       = opentelemetry::nostd;
namespace metrics_api = opentelemetry::metrics;

namespace
{

static nostd::shared_ptr<metrics_api::ObservableInstrument> double_observable_counter;

std::map<std::string, std::string> get_random_attr()
{
  const std::vector<std::pair<std::string, std::string>> labels = {{"key1", "value1"},
                                                                   {"key2", "value2"},
                                                                   {"key3", "value3"},
                                                                   {"key4", "value4"},
                                                                   {"key5", "value5"}};
  return std::map<std::string, std::string>{labels[rand() % (labels.size() - 1)],
                                            labels[rand() % (labels.size() - 1)]};
}

class MeasurementFetcher
{
public:
  static void Fetcher(opentelemetry::metrics::ObserverResult observer_result, void * /* state */)
  {
    if (nostd::holds_alternative<
            nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<double>>>(observer_result))
    {
      double random_incr = (rand() % 5) + 1.1;
      value_ += random_incr;
      std::map<std::string, std::string> labels = get_random_attr();
      nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<double>>>(
          observer_result)
          ->Observe(value_, labels);
    }
  }
  static double value_;
};
double MeasurementFetcher::value_ = 0.0;
}  // namespace

void foo_library::counter_example(const std::string &name)
{
  std::string counter_name                    = name + "_counter";
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter(name, "1.2.0");
  auto double_counter                         = meter->CreateDoubleCounter(counter_name);

  for (uint32_t i = 0; i < 20; ++i)
  {
    double val = (rand() % 700) + 1.1;
    double_counter->Add(val);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void foo_library::istio_request_count_example()
{
  std::string meter_name                      = "csm_metrics";
  std::string meter_version                   = "1.0.0";
  std::string request_count_metric_name       = "istio_request_count";
  std::string counter_name                    = request_count_metric_name + "_counter";
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter(meter_name, meter_version);
  auto double_counter                         = meter->CreateDoubleCounter(counter_name);

  std::vector<std::pair<std::string, std::string>> labels = {
      {"source_workload_name", "source_foo"},
      {"source_namespace_name", "source_ns_foo"},
      {"source_canonical_service_name", "source_canonical_service_foo"},
      {"destination_workload_name", "destination_bar"},
      {"destination_namespace_name", "destination_ns_bar"},
      {"destination_canonical_service_name", "destination_canonical_service_bar"},
  };
  auto labelkv = opentelemetry::common::KeyValueIterableView<decltype(labels)>{labels};

  for (uint32_t i = 0; i < 20; ++i)
  {
    // double val = (rand() % 700) + 1.1;
    double val = 1;
    double_counter->Add(val, labelkv);
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

void foo_library::observable_counter_example(const std::string &name)
{
  std::string counter_name                    = name + "_observable_counter";
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter(name, "1.2.0");
  double_observable_counter                   = meter->CreateDoubleObservableCounter(counter_name);
  double_observable_counter->AddCallback(MeasurementFetcher::Fetcher, nullptr);
  for (uint32_t i = 0; i < 20; ++i)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void foo_library::histogram_example(const std::string &name)
{
  std::string histogram_name                  = name + "_histogram";
  auto provider                               = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter(name, "1.2.0");
  auto histogram_counter = meter->CreateDoubleHistogram(histogram_name, "des", "unit");
  auto context           = opentelemetry::context::Context{};
  for (uint32_t i = 0; i < 20; ++i)
  {
    double val                                = (rand() % 700) + 1.1;
    std::map<std::string, std::string> labels = get_random_attr();
    auto labelkv = opentelemetry::common::KeyValueIterableView<decltype(labels)>{labels};
    histogram_counter->Record(val, labelkv, context);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
}
