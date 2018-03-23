/*
 *  Butterworth LP/HP IIR filter design
 *
 *  Copyright (C) 2016 Mirko Maischberger <mirko.maischberger@gmail.com>
 *
 *  Portions Copyright (C) 2014 Exstrom Laboratories LLC <stefan(AT)exstrom.com>
 *                              Longmont, CO 80503, USA
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  A copy of the GNU General Public License is available on the internet at:
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <boost/circular_buffer.hpp>
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <complex>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <vector>
#include <array>
#include <tuple>
#include <cmath>

namespace
{

inline std::vector<std::complex<double>>
binomial_mult(const std::vector<std::complex<double>>& p)
{
    std::vector<std::complex<double>> a(p.size(), 0.0);
    for (size_t ii(0); ii != p.size(); ++ii) {
        for (size_t jj(ii); jj != 0; --jj) {
            a[jj] += p[ii] * a[jj - 1];
        }
        a[0] += p[ii];
    }
    return a;
}

template <int ORDER>
std::array<double, ORDER + 1>
acof_bwlp(double fcf)
{
    std::vector<std::complex<double>> rcof(ORDER);
    double theta = M_PI * fcf;
    double st = sin(theta);
    double ct = cos(theta);
    for (int k = 0; k < ORDER; ++k) {
        double parg = M_PI * (2.0 * k + 1) / (2.0 * ORDER);
        double a = 1.0 + st * sin(parg);
        rcof[k].real(-ct / a);
        rcof[k].imag(-st * cos(parg) / a);
    }
    auto ddcof = binomial_mult(rcof);

    std::array<double, ORDER + 1> dcof;
    dcof[0] = 1.0;
    for (int k(1); k != ORDER + 1; ++k) {
        dcof[k] = ddcof[k - 1].real();
    }
    return dcof;
}

template <int ORDER>
std::array<double, ORDER + 1>
acof_bwhp(int n, double fcf)
{
    return acof_bwlp<ORDER>(fcf);
}

template <int ORDER>
double
sf_bwlp(double fcf)
{
    int k;         // loop variables
    double omega;  // M_PI * fcf
    double fomega; // function of omega
    double parg0;  // zeroth pole angle
    double sf;     // scaling factor

    omega = M_PI * fcf;
    fomega = sin(omega);
    parg0 = M_PI / (double)(2 * ORDER);

    sf = 1.0;
    for (k = 0; k < ORDER / 2; ++k)
        sf *= 1.0 + fomega * sin((double)(2 * k + 1) * parg0);

    fomega = sin(omega / 2.0);

    if (ORDER % 2)
        sf *= fomega + cos(omega / 2.0);
    sf = pow(fomega, ORDER) / sf;

    return (sf);
}

template <int ORDER>
double
sf_bwhp(double fcf)
{
    int m, k;      // loop variables
    double omega;  // M_PI * fcf
    double fomega; // function of omega
    double parg0;  // zeroth pole angle
    double sf;     // scaling factor

    omega = M_PI * fcf;
    fomega = sin(omega);
    parg0 = M_PI / (double)(2 * ORDER);

    m = ORDER / 2;
    sf = 1.0;
    for (k = 0; k < ORDER / 2; ++k)
        sf *= 1.0 + fomega * sin((double)(2 * k + 1) * parg0);

    fomega = cos(omega / 2.0);

    if (ORDER % 2)
        sf *= fomega + sin(omega / 2.0);
    sf = pow(fomega, ORDER) / sf;

    return sf;
}

template <int ORDER>
std::array<double, ORDER + 1>
ccof_bwlp()
{
    std::array<double, ORDER + 1> ccof;
    ccof[0] = 1;
    ccof[1] = ORDER;
    int m = ORDER / 2;
    for (int i = 2; i <= m; ++i) {
        ccof[i] = (ORDER - i + 1) * ccof[i - 1] / i;
        ccof[ORDER - i] = ccof[i];
    }
    ccof[ORDER - 1] = ORDER;
    ccof[ORDER] = 1;
    return ccof;
}

template <int ORDER>
std::array<double, ORDER + 1>
ccof_bwhp()
{
    std::array<double, ORDER + 1> ccof;
    // TBD
    return ccof;
}

} // anonymous namespace

///
/// An IIR Butterworth LP Filter
///
/// Similar to octave-signal butter function, low-pass
///
/// @param sample_rate The desired sample rate (=2*Nyquist)
/// @param cutoff_freq The -3dB cutoff frequency
///
/// @returns [b,a] coefficients ready to be used by the iir_filter class.
///
template <int ORDER>
std::tuple<double, std::array<double, ORDER + 1>, std::array<double, ORDER + 1>>
butter_lp(double sample_rate, double cutoff_freq)
{
    return std::make_tuple(sf_bwlp<ORDER>(2.0 * cutoff_freq / sample_rate),
                           ccof_bwlp<ORDER>(),
                           acof_bwlp<ORDER>(2.0 * cutoff_freq / sample_rate));
}

///
/// An IIR Butterworth HP Filter
///
/// Similar to octave-signal butter function, high-pass
///
/// @param sample_rate The desired sample rate (=2*Nyquist)
/// @param cutoff_freq The -3dB cutoff frequency
///
/// @returns [b,a] coefficients ready to be used by the iir_filter class.
///
template <int ORDER>
std::tuple<double, std::array<double, ORDER + 1>, std::array<double, ORDER + 1>>
butter_hp(double sample_rate, double cutoff_freq)
{
    return std::make_tuple(sf_bwhp<ORDER>(2.0 * cutoff_freq / sample_rate),
                           ccof_bwhp<ORDER>(),
                           acof_bwhp<ORDER>(2.0 * cutoff_freq / sample_rate));
}

/// Simple arctan demodulator
struct atan_fm_demodulator
{
    atan_fm_demodulator()
      : s1(0)
    {
    }

    /// Q&I
    double operator()(const std::complex<double>& s)
    {
        double d = std::arg(std::conj(s1) * s);
        s1 = s;
        return d;
    }
    std::complex<double> s1;
};

///
/// Generic IIR filter simulator or specified ORDER
///
template <int ORDER>
struct iir_filter
{
    ///
    /// Creates the filter.
    ///
    /// @param gain input gain
    /// @param b Coefficients for the input
    /// @param a Coefficients for the output
    ///
    explicit iir_filter(double gain, const std::array<double, ORDER + 1>& b,
                        const std::array<double, ORDER + 1>& a)
      : gain_m(gain)
      , b_m(b)
      , a_m(a)
      , xv_m(ORDER + 1, 0)
      , yv_m(ORDER + 1, 0)
    {
        assert(a[0] == 1.0);
        for (size_t ii(0); ii != (ORDER + 1) / 2; ++ii)
            assert(b[ii] == b[b.size() - ii - 1]);
    }

    ///
    /// Creates the filter.
    ///
    /// @param tuple(gain, b, a) as returned by the butter_lp function
    ///
    explicit iir_filter(const std::tuple< double, std::array<double, ORDER + 1>, std::array<double, ORDER + 1> >& params)
        : iir_filter(std::get<0>(params), std::get<1>(params), std::get<2>(params))
    {
    }

    ///
    /// Feed the filter with samples.
    ///
    /// @param in The latest input to the filter
    /// @returns The filtered output
    ///
    double operator()(double in)
    {
        xv_m.push_front(in);
        double yvn =
            gain_m * std::inner_product(xv_m.begin(), xv_m.end(), b_m.begin(), 0.0) -
            std::inner_product(yv_m.begin(), yv_m.end(), a_m.begin() + 1, 0.0);
        yv_m.push_front(yvn);
        return yvn;
    }

  private:
    double gain_m;
    std::array<double, ORDER + 1> b_m;
    std::array<double, ORDER + 1> a_m;
    boost::circular_buffer<double> xv_m;
    boost::circular_buffer<double> yv_m;
};
