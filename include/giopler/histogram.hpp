// Copyright (c) 2022 Carlos Reyes
// This code is licensed under the permissive MIT License (MIT).
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#ifndef GIOPPLER_HISTOGRAM_HPP
#define GIOPPLER_HISTOGRAM_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ios>
#include <string>
#include <vector>

#include "giopler/contract.hpp"

// -----------------------------------------------------------------------------
namespace gioppler::histogram
{

// -----------------------------------------------------------------------------
/// observation index into histogram
using rank_t        = uint32_t;

/// the values we are tracking in the histogram
using observation_t = uint64_t;

// -----------------------------------------------------------------------------
/// bucket of observations for the histogram
// provides support for ranking (quantile) operations
// observations are uint64 values
// count value goes up to 16 million
// stores the minimum and maximum observation values for the bucket
// actually stores the minimum observation and a span (range) to save bits
// uses linear interpolation to estimate the observation for the requested rank
// Note: this is an internal class intended to be called only from Histogram
class Bucket
{
public:
  Bucket()
    : _observation_min{}, _observation_span{}, _count{}
  {
    dev::confirm(invariant());
  }

  explicit Bucket(const observation_t observation)
    : _observation_min{observation}, _observation_span{}, _count{1}
  {
    dev::confirm(invariant());
  }

  Bucket& operator +=(const Bucket& rhs)
  {
    dev::Invariant _{[this]{ return invariant(); }};
    const observation_t min = std::min(get_min(), rhs.get_min());
    const observation_t max = std::max(get_max(), rhs.get_max());
    _observation_min  = min;
    _observation_span = max-min;
    _count += rhs._count;
    assert(invariant());
    return *this;
  }

  observation_t get_min() const {
    return _observation_min;
  }

  observation_t get_max() const {
    return _observation_min+_observation_span;
  }

  rank_t get_count() const {
    return static_cast<rank_t>(_count);
  }

  // https://stackoverflow.com/questions/325933/determine-whether-two-date-ranges-overlap/325964#325964
  bool is_overlapping(const Bucket& b) const {
    return (get_min() <= b.get_max() && get_max() >= b.get_min());
  }

  bool contains_rank(const rank_t rank) const {
    return (rank >= 1 && rank <= _count);
  }

  /// get the observation contained using the rank to interpolate within the bucket
  // 1 .. _count
  double get_rank(const rank_t rank) const
  {
    assert(invariant());
    assert(contains_rank(rank));
    if (_count == 1) {
      return _observation_min;
    } else {
      return _observation_min + (rank-1)*_observation_span/(_count-1);
    }
  }

  friend constexpr bool
  operator <(const Bucket& l, const Bucket& r) {
    return (l._observation_min < r._observation_min);
  }

  std::ostream&
  write(std::ostream& os) const {
    os << '[' << get_min() << ',' << get_count() << ',' << get_max() << ']';
    return os;
  }

private:
  observation_t _observation_min;       // low end of range containing within this bucket
  observation_t _observation_span :40;  // about 10^12 (78 hours in ns)
  observation_t _count :24;             // about 10^7  (16,777,216)

  bool invariant() const {              // not a lot we can validate here
    assert(_count > 0);
    return (_count > 0);
  }
};

std::ostream&
operator <<(std::ostream& os, const Bucket& bucket) {
  return bucket.write(os);
}

// -----------------------------------------------------------------------------
/// histogram of uint64 values
// provides robust statistics computed using quantiles
// uses a vector to store the buckets with periodic compacting
// the vector is not resized once initially created
// number of buckets can handle millions of observations
// buckets have varying widths and are only allocated as needed
// compacting aims the level the number of observations per bucket
// https://en.wikipedia.org/wiki/Histogram
// https://en.wikipedia.org/wiki/Robust_statistics
// https://en.wikipedia.org/wiki/Quantile
class Histogram
{
public:
  Histogram()
    : _observations{}, _compacted(true)
  {
    _buckets.reserve(max_buckets);
    assert(invariant());
  }

  void add_observation(const observation_t observation)
  {
    assert(invariant());
    _buckets.emplace_back(Bucket(observation));
    ++_observations;
    _compacted = false;
    assert(invariant());

    if (_buckets.size() >= max_buckets) {
      compact_buckets();
    }
  }

  rank_t get_count() const {
    return _observations;
  }

  // 1 .. _observations
  observation_t get_by_rank(const rank_t rank) const
  {
    if (!_observations)   return 0;

    compact_buckets();
    assert(rank >= 1 && rank <= _observations);
    rank_t delta_rank = std::clamp(rank, static_cast<rank_t>(1), _observations);

    for (const auto& bucket : _buckets) {
      if (bucket.contains_rank(delta_rank)) {
        return bucket.get_rank(delta_rank);
      } else {
        delta_rank -= bucket.get_count();
      }
    }

    assert(false);   // should never get here
  }

  /// average of Q1+2*median+Q3
  // https://en.wikipedia.org/wiki/Trimean
  // https://en.wikipedia.org/wiki/Location_parameter
  // https://en.wikipedia.org/wiki/Trimmed_estimator
  observation_t get_trimedian() const
  {
    compact_buckets();
    if (_observations < 4) {
      if (!_observations) {
        return 0;
      } else if (_observations <= 2) {
        return get_by_rank(1);
      } else {   // 3
        return get_by_rank(2);
      }
    }

    const rank_t q1_rank       = round_div(_observations, 4);
    const rank_t q2_rank       = round_div(_observations, 2);
    const rank_t q3_rank       = q1_rank+q2_rank;
    const observation_t q1_obs = get_by_rank(q1_rank);
    const observation_t q2_obs = get_by_rank(q2_rank);
    const observation_t q3_obs = get_by_rank(q3_rank);
    return round_div(q1_obs+2*q2_obs+q3_obs, 4);
  }

  /// delta between 25th and 75th quantiles
  // https://en.wikipedia.org/wiki/Interquartile_range
  observation_t get_interquantile_range() const
  {
    compact_buckets();
    if (_observations < 4) {
      if (_observations <= 1) {
        return 0;
      } else if (_observations == 2) {
        return get_by_rank(2)-get_by_rank(1);
      } else {   // 3
        return get_by_rank(3)-get_by_rank(1);
      }
    }

    const rank_t q1_rank       = round_div(_observations, 4);
    const rank_t q2_rank       = round_div(_observations, 2);
    const rank_t q3_rank       = q1_rank+q2_rank;
    const observation_t q1_obs = get_by_rank(q1_rank);
    const observation_t q3_obs = get_by_rank(q3_rank);
    return (q3_obs-q1_obs);
  }

  /// an indication on how much noise is in the observations
  // 0=much noise, 99=very little noise
  // https://en.wikipedia.org/wiki/Coefficient_of_variation
  // https://en.wikipedia.org/wiki/Efficiency_(statistics)
  // https://en.wikipedia.org/wiki/Signal-to-noise_ratio
  // https://en.wikipedia.org/wiki/Logarithmic_scale
  // 0 .. 99
  int get_signal_to_noise_ratio() const
  {
    double trimedian = get_trimedian();
    trimedian = (trimedian == 0.0) ? 1.0 : trimedian;
    double std_dev   = get_standard_deviation();
    std_dev = (std_dev < 1) ? 1.0 : std_dev;
    const double snr = std::clamp(10*std::log10((trimedian*trimedian)/(std_dev*std_dev)), 0.0, 99.0);
    return std::lround(snr);
  }

  /// Do we have any outliers below or above most of the observations?
  // An outlier is a value six sigma away from the median
  //   in excess of the expectation from the standard deviation
  //   if we assume a normal distribution.
  // alternative method:   https://en.wikipedia.org/wiki/Outlier#Tukey's_fences
  std::pair<bool,bool> have_outliers() const
  {
    // https://en.wikipedia.org/wiki/68%E2%80%9395%E2%80%9399.7_rule
    static constexpr double stddev_6sigma = (1-0.999999998026825)/2;   // proportion outside interval
    const rank_t expected_outliers = std::lround(get_count()*stddev_6sigma);

    compact_buckets();
    const double trimedian          = get_trimedian();
    const double standard_deviation = get_standard_deviation();

    const double low_6sigma = trimedian-6*standard_deviation;
    rank_t low_outliers = 0;
    if (low_6sigma >= get_min()) {
      const rank_t low_values = count_low_values(std::lround(low_6sigma));
      if (low_values > expected_outliers) {
        low_outliers = low_values-expected_outliers;
      }
    }

    const double high_6sigma = trimedian+6*standard_deviation;
    rank_t high_outliers = 0;
    if (high_6sigma <= get_max()) {
      const rank_t high_values = count_high_values(std::lround(high_6sigma));
      if (high_values > expected_outliers) {
        high_outliers = high_values-expected_outliers;
      }
    }

    return std::make_pair<>(low_outliers, high_outliers);
  }

  /// A sparkline is a textual representation of a simple chart.
  // need to use reinterpret_cast to get around poor support for UTF-8 in C++20
  std::string get_sparkline(const observation_t width = 9) const
  {
    if (!_buckets.size())   return "";

    compact_buckets();
    static const std::vector<std::u8string> steps = {u8"▁", u8"▂", u8"▃", u8"▄", u8"▅", u8"▆", u8"▇", u8"█"};

    std::string sparkline;
    std::vector<rank_t> buckets(width);
    const observation_t min_value    = get_min();
    const observation_t max_value    = get_max();
    const observation_t range_value  = max_value-min_value;
    const double        bucket_width = static_cast<double>(range_value) / (width-1);

    for (const auto& bucket : _buckets) {
      for (rank_t rank = 1; rank <= bucket.get_count(); ++rank) {
        const rank_t index = static_cast<rank_t>((bucket.get_rank(rank)-min_value) / bucket_width);
        assert(index >= 0 && index < width);
        ++buckets[index];
      }
    }

    rank_t max_height = 0;
    for (const auto& bucket : buckets) {
      if (bucket > max_height) {
        max_height = bucket;
      }
    }

    for (const auto& bucket : buckets) {
      const size_t step = round_div(static_cast<rank_t>(bucket * (steps.size()-1)), max_height);
      assert(step >= 0 && step < steps.size());
      sparkline += reinterpret_cast<const char*>(steps[step].c_str());
    }

    return sparkline;
  }

  std::string get_statistics() const
  {
    auto [have_low_outliers, have_high_outliers] = have_outliers();
    std::stringstream oss;
    oss << '{' << "min:" << get_min()
        << ',' << "max:" << get_max()
        << ',' << "count:" << get_count()
        << ',' << "low_outliers:" << std::boolalpha << have_low_outliers
        << ',' << "high_outliers:" << std::boolalpha << have_high_outliers
        << ',' << "trimedian:" << get_trimedian()
        << ',' << "std_dev:" << get_standard_deviation()
        << ',' << "snr:" << get_signal_to_noise_ratio()
        << ',' << "sparkline:" << get_sparkline()
        << '}';
    return oss.str();
  }

  std::ostream& write(std::ostream& os) const
  {
    compact_buckets();
    os << '[';
    for (const auto& bucket : _buckets) {
      os << bucket;
    }
    os << ']';
    return os;
  }

private:
  rank_t _observations;                       // count of observations
  mutable bool _compacted;
  mutable std::vector<Bucket> _buckets;
  static constexpr rank_t max_buckets = 256;

  // https://stackoverflow.com/questions/2422712/rounding-integer-division-instead-of-truncating
  static uint64_t round_div(const uint64_t dividend, const uint64_t divisor)
  {
    return (dividend + (divisor / 2)) / divisor;
  }

  static uint32_t round_div(const uint32_t dividend, const uint32_t divisor)
  {
    return (dividend + (divisor / 2)) / divisor;
  }

  bool invariant() const
  {
    observation_t sum{};
    for (const auto& bucket : _buckets) {
      sum += bucket.get_count();
    }
    assert(sum == _observations);
    assert(_buckets.size() <= max_buckets);
    return (sum == _observations && _buckets.size() <= max_buckets);
  }

  observation_t get_min() const
  {
    compact_buckets();
    return _buckets.size() ? _buckets.front().get_rank(1) : 0;
  }

  observation_t get_max() const
  {
    compact_buckets();
    return _buckets.size() ? _buckets.back().get_rank(_buckets.back().get_count()) : 0;
  }

  /// ensure we have room to add another bucket
  // makes sure the buckets are sorted correctly and non-overlapping
  void compact_buckets() const
  {
    if (_compacted)   return;
    assert(invariant());
    std::ranges::sort(_buckets,
                      [](const Bucket& l, const Bucket& r) { return (l.get_min() < r.get_min()); });

    const rank_t target_bucket_size = 1+round_div(_observations, max_buckets);
    Bucket* p_last_bucket = nullptr;

    for (auto bucket_it = std::begin(_buckets); bucket_it != std::end(_buckets); ) {
      if (p_last_bucket &&
          (p_last_bucket->get_count() < target_bucket_size ||
           bucket_it->is_overlapping(*p_last_bucket))) {
        *p_last_bucket += *bucket_it;   // merge
        bucket_it = _buckets.erase(bucket_it);
      } else {
        p_last_bucket = &(*bucket_it);   // assuming iterator may not be a simple pointer
        ++bucket_it;
      }
    }

    _compacted = true;
    assert(invariant());
    assert(_buckets.size() < max_buckets);
  }

  // returns the number of values <= threshold
  rank_t count_low_values(const observation_t threshold) const
  {
    compact_buckets();
    rank_t low_count = 0;
    for (const auto& bucket : _buckets) {
      if (bucket.get_max() <= threshold) {
        low_count += bucket.get_count();   // full bucket
      } else {                             // partial bucket
        for (rank_t rank = 1; rank <= bucket.get_count(); ++rank) {
          if (bucket.get_rank(rank) <= threshold) {
            ++low_count;
          } else {
            break;
          }
        }
        break;
      }
    }
    return low_count;
  }

  // returns the number of values >= threshold
  rank_t count_high_values(const observation_t threshold) const
  {
    compact_buckets();
    rank_t high_count = 0;
    for (auto bucket_it = std::crbegin(_buckets); bucket_it != std::crend(_buckets); ++bucket_it) {
      if (bucket_it->get_min() >= threshold) {
        high_count += bucket_it->get_count();   // full bucket
      } else {                                  // partial bucket
        for (rank_t rank = bucket_it->get_count(); rank >= 1; --rank) {
          if (bucket_it->get_rank(rank) >= threshold) {
            ++high_count;
          } else {
            break;
          }
        }
        break;
      }
    }
    return high_count;
  }

  // this is surprisingly accurate, but is only used internally
  // for <100 observations, using the IQR (25/75 quantiles) is appropriate
  // for >100 observations, using 20/80 quantiles might be more accurate
  // we always use the IQR to yield more consistent results
  // https://en.wikipedia.org/wiki/Robust_measures_of_scale
  // https://en.wikipedia.org/wiki/Scale_parameter
  // https://en.wikipedia.org/wiki/Error_function
  double get_standard_deviation() const
  {
    compact_buckets();
    static constexpr double iqr_unbiased = 1.35623115191269; // 2*sqrt(2)*erfc(0.5)
    return static_cast<double>(get_interquantile_range()) / iqr_unbiased;
  }
};

std::ostream&
operator<<(std::ostream& os, const Histogram& histogram) {
  return histogram.write(os);
}

// -----------------------------------------------------------------------------
} // namespace gioppler::histogram

// -----------------------------------------------------------------------------
#endif // defined GIOPPLER_HISTOGRAM_HPP
