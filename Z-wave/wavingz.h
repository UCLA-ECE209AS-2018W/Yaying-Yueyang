//
// Copyright (C) 2016 Mirko Maischberger <mirko.maischberger@gmail.com>
//
// This file is part of WavingZ.
//
// WavingZ is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// WavingZ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include "dsp.h"

#include <boost/optional.hpp>

#include <bitset>
#include <iomanip>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <functional>
#include <fstream> 
#include <time.h>
namespace wavingz
{

/// Frame control, first byte
struct frame_control_0_t
{
    uint16_t header_type : 4;
    uint16_t speed : 1;
    uint16_t low_power : 1;
    uint16_t ack_request : 1;
    uint16_t routed : 1;
} __attribute__((packed));

static_assert(sizeof(frame_control_0_t) == 1, "Assumption broken");

/// Frame control, second byte
struct frame_control_1_t
{
    uint16_t sequence_number : 4;
    uint16_t beaming_info : 4;
} __attribute__((packed));

static_assert(sizeof(frame_control_1_t) == 1, "Assumption broken");

/// Z-Wave compatible header (except for errors and omissions)
///
/// We read byte by byte to be endianness agnostic; Network order is Big Endian,
/// so in order to decode the HomeID we need to
///    ((uint32_t)p.home_id3 | p.home_id2 << 8 | p.home_id1 << 16 | p.home_id0
///    << 24)
///
struct packet_t
{
    uint8_t home_id0;
    uint8_t home_id1;
    uint8_t home_id2;
    uint8_t home_id3;
    uint8_t source_node_id;
    union
    {
        frame_control_0_t frame_control_0;
        uint8_t fc0;
    };
    union
    {
        frame_control_1_t frame_control_1;
        uint8_t fc1;
    };
    uint8_t length;
    uint8_t dest_node_id;
    uint8_t command_class;
} __attribute__((packed));

static_assert(sizeof(packet_t) == 10, "Assumption broken");

/// Frame Check Sequence Calculator
template <typename T>
typename std::iterator_traits<T>::value_type
checksum(T begin, T end)
{
    return std::accumulate(begin, end, 0xff, std::bit_xor<uint8_t>());
}

/// Convert double IQ into (unsigned) chars
template <typename Byte>
struct complex8_convert
{
    complex8_convert(double A)
      : A_m(A)
    {
    }
    std::pair<Byte, Byte> operator()(double i, double q)
    {
        check_range(i, q);
        if (std::is_signed<Byte>::value)
        {
            return output_signed8(i, q);
        }
        else
        {
            return output_unsigned8(i, q);
        }
    }

  private:
    void check_range(double i, double q)
    {
        if (std::abs(i * A_m) > 127.0 || std::abs(q * A_m) > 127.0)
        {
            throw std::runtime_error("Value too big!");
        }
    }

    std::pair<Byte, Byte> output_unsigned8(double i, double q)
    {
        double offset = 127.0;
        return std::make_pair((Byte)(i * A_m + offset),
                              (Byte)(q * A_m + offset));
    }

    std::pair<Byte, Byte> output_signed8(double i, double q)
    {
        return std::make_pair((Byte)(i * A_m), (Byte)(q * A_m));
    }
    const double A_m;
};

template< typename Byte >
struct encoder
{
    encoder(size_t sample_rate, size_t baud_rate, double A = 100.0)
        : A(A)
        , sample_rate(sample_rate)
        , baud_rate(baud_rate), Ts(sample_rate / baud_rate)
        , lp1(butter_lp<6>(sample_rate, f1_mul * dfreq * 2.5))
        , lp2(butter_lp<6>(sample_rate, f1_mul * dfreq * 2.5))

    {
        if (std::abs(sin(2.0 * M_PI * dfreq * f0_mul * (double)Ts / sample_rate) -
                     sin(2.0 * M_PI * dfreq * f1_mul * (double)Ts / sample_rate)) > 1e-12)
        {
            throw std::runtime_error(
                "Please choose sample_rate and baud_rate so "
                "that the resulting phase will be coherent "
                "with the (1/2) separation frequency (20KHz).");
        }
    }

    /// Encode the payload into an IQ signal (cu8 or cs8 depending on Byte type)
    template <typename It>
    std::vector<std::pair<Byte, Byte>>
    operator()(It payload_begin, It payload_end, double silence = 1.0)
    {
        constexpr uint8_t PREAMBLE = 0x55; // 10101010...10101010 frame preamble
        constexpr uint8_t SOF = 0xF0;      // Start of frame mark

        std::vector<std::pair<Byte, Byte>> iq;

        // .001" silence
        for (size_t ii(0); ii != sample_rate / 1000; ++ii) {
            iq.emplace_back(convert_iq(lp1(0.0), lp2(0.0)));
        }

        size_t sample = 0;

        // preamble
        for (size_t ii(0); ii != 20; ++ii) {
            emplace_byte(PREAMBLE, sample, iq);
        }

        // SOF
        emplace_byte(SOF, sample, iq);

        // payload
        for (It ch = payload_begin; ch != payload_end; ++ch) {
            emplace_byte(*ch, sample, iq);
        }

        // silence at the end (it seems that more or less 1" is needed by the HackRF
        // One to complete transmission?)
        for (size_t ii(0); ii != size_t(silence*sample_rate); ++ii) {
            iq.emplace_back(convert_iq(lp1(0.0), lp2(0.0)));
        }
        return iq;
    }

private:

    void emplace_byte(char data, size_t& sample, std::vector<std::pair<Byte,Byte>>& iq)
    {
        for (size_t ii(0); ii != 8; ++ii) {
            double f_shift = (((data << ii) & 0x80) ? f1_mul : f0_mul) * dfreq;
            for (size_t kk(0); kk != Ts; ++kk) {
                double i =
                    lp1(sin(2.0 * M_PI * f_shift * (double)sample / sample_rate));
                double q =
                    lp2(cos(2.0 * M_PI * f_shift * (double)sample / sample_rate));
                iq.emplace_back(convert_iq(i, q));
                ++sample;
            }
        }
    }

    const double A = 100;
    complex8_convert<Byte> convert_iq = complex8_convert<Byte>(A);
    const size_t sample_rate;
    const size_t baud_rate;
    const size_t Ts;
    iir_filter<6> lp1, lp2;
    static constexpr size_t dfreq = 20000;
    static constexpr double f0_mul = 0.5;
    static constexpr double f1_mul = 2.5;

};

/// Debug print a packet
template <typename It>
inline std::ostream&
zwave_print(std::ofstream& fout,std::ostream& out, It data_begin, It data_end)
{   time_t now=time(0);
    struct tm tstruct;
    char buf[80];
    tstruct=*localtime(&now);
    strftime(buf,sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    
    fout<<std::string(buf)<< "@" ;
    for(auto ch = data_begin; ch != data_end; ++ch)
       { 
        fout<<std::hex <<std::setfill('0') <<std::setw(2) << (int)*ch <<' ';
	out << std::hex << std::setfill('0') << std::setw(2) << (int)*ch << ' ';}
    out << std::endl;
    fout << std::endl; 

    size_t len = data_end - data_begin;
    packet_t& p = *(packet_t*)data_begin;
    if (len < sizeof(packet_t) || len < p.length)
    {
        out << "[ ] ";
        return out;
    }
    else if (checksum(data_begin, data_begin + (size_t)p.length - 1) !=
             data_begin[p.length - 1])
    {
        out << "[ ] ";
    }
    else
    {
        out << "[x] ";
    }
    out << std::hex << std::setfill('0') << std::setw(2)
        << "HomeId: " << ((uint32_t)p.home_id3 | p.home_id2 << 8 |
                          p.home_id1 << 16 | p.home_id0 << 24)
        << ", SourceNodeId: " << (int)p.source_node_id << std::hex
        << ", FC0: " << (int)p.fc0 << ", FC1: " << (int)p.fc1 << std::dec
        << ", FC[speed=" << p.frame_control_0.speed
        << " low_power=" << p.frame_control_0.low_power
        << " ack_request=" << p.frame_control_0.ack_request
        << " header_type=" << p.frame_control_0.header_type
        << " beaming_info=" << p.frame_control_1.beaming_info
        << " seq=" << p.frame_control_1.sequence_number
        << "], Length: " << std::dec << (int)p.length
        << ", DestNodeId: " << std::dec << (int)p.dest_node_id
        << ", CommandClass: " << std::hex << (int)p.command_class
        << ", Payload: " << std::hex << std::setfill('0');
    for (int i = sizeof(packet_t); i < p.length - 1; i++) {
        out << std::setw(2) << (int)data_begin[i] << " ";
    }
     out<<std::endl;
    if((int)data_begin[9]==0x31&&(int)data_begin[10]==0x05){
    if((int)data_begin[11]==0x01){
      if((int)data_begin[12]==0x22) {
            std::bitset<8> set1(data_begin[13]);
	    std::bitset<8> set2(data_begin[14]);
            int base=1;
            int sum=0;
            for (int i=0;i<set2.size();i++) {
                    if(set2.test(i))
		{sum+=base;base=base*2;}
                else {sum+=0;base=base*2;}
}           
            for (int i=0;i<set1.size();i++) {
                if (set1.test(i)) {sum+=base;base=base*2;}
                else {sum+=0; base=base*2;}

}      
            double temp=sum/10.0;
              out<<"Temperature: "<<std::dec<<temp<<"C"<<std::endl;
}     if((int)data_begin[12]==0x2a){
            std::bitset<8> set1(data_begin[13]);
            std::bitset<8> set2(data_begin[14]);
            int sum=0;
            int base=1;
            for (int i=0;i<set2.size();i++){
                  out<<set2.test(i); if(set2.test(i)) {sum+=base;base=base*2;}
                else {sum+=0;base=base*2;}
}           for (int i=0;i<set1.size();i++) {
                if (set1.test(i)) {sum+=base;base=base*2;}
                else {sum+=0;base=base*2;}
}
	    double temp=(sum/10.0-32)*5/9;
            out<<"Temperature: "<<std::dec<<temp<<"C"<<std::endl;
}


}   else if ((int)data_begin[11]==0x03) {
	 if((int)data_begin[12]==0x0a) {
                std::bitset<8> set1(data_begin[13]);
                std::bitset<8> set2(data_begin[14]);
                int sum=0;
                int base=1;
                for (int i=0;i<set2.size();i++) {
                 if (set2.test(i)) {sum+=base;base=base*2;}
                  else {sum+=0;base=base*2;}
}              for (int i=set1.size()-1;i>=0;i--) {
                if (set1.test(i)) {sum+=base;base=base*2;}
                else {sum+=0;base=base*2;}
}
             
		out<<"Luminance: "<<std::dec<<sum<<"Lux"<<std::endl;
}}
    else if ((int)data_begin[11]==0x05) {
         if((int)data_begin[12]==0x01) {
           
              std::bitset<8> set2(data_begin[13]);
                int sum=0;
                int base=1;
                for (int i=0;i<set2.size();i++) {
               
		 if (set2.test(i)) {sum+=base;base=base*2;}
                  else {sum+=0;base=base*2;}
}              
            out<<"Humidity: "<<std::dec<<sum<<"%"<<std::endl;
}  
}
    else if ((int)data_begin[11]==0x1b){
         if((int)data_begin[12]==0x01){
                std::bitset<8> set2(data_begin[13]);
                int sum=0;
                int base=1;
                for (int i=0;i<set2.size();i++) {
                  if (set2.test(i)) {sum+=base;base=base*2;}
                  else {sum+=0;base=base*2;}
}             
		out<<"Ultraviolet: "<<std::dec<<sum<<"UV"<<std::endl;
}
}
}   if((int)data_begin[9]==0x30) {
       // out<<"ahahahahah"<<std::endl;   
     if((int)data_begin[11]==0x00){
           out<<"Door CLOSED"<<std::endl;
}       else if ((int)data_begin[11]==0xff) {
           out<<"Door OPEN"<<std::endl;

}
}
    return out;
}

// demodulation state machine
namespace demod
{
namespace state_machine
{
struct symbol_sm_t;

// -----------------------------------------------------------------------------

namespace symbol_sm
{

struct state_base_t
{
    virtual void process(symbol_sm_t& ctx, const boost::optional<bool>& symbol) = 0;
};

// Detecting the first nibble of the SOF (0xF)
struct start_of_frame_1_t : public state_base_t
{
    void process(symbol_sm_t& ctx, const boost::optional<bool>& symbol) override;
private:
    size_t cnt = 0;
};

// Parsing the second nibble of the SOF (0x0)
struct start_of_frame_0_t : public state_base_t
{
    void process(symbol_sm_t& ctx, const boost::optional<bool>& symbol) override;
private:
    size_t cnt = 0;
};

// Pushing data into payload
struct payload_t : public state_base_t
{
    void process(symbol_sm_t& ctx, const boost::optional<bool>& symbol) override;
private:
    std::vector<uint8_t> payload;
    std::bitset<8> b = 0;
    size_t cnt = 0;
};

} // namespace

// -----------------------------------------------------------------------------

struct symbol_sm_t
{
    symbol_sm_t(const std::function<void(uint8_t*, uint8_t*)>& callback)
      : callback(callback)
      , current_state_m(new symbol_sm::start_of_frame_1_t())
    {
    }
    // sample can be 0, 1 or none (no signal)
    void process(const boost::optional<bool>& symbol);
    void state(std::unique_ptr<symbol_sm::state_base_t>&& next_state);
    std::function<void(uint8_t*, uint8_t*)> callback;
private:
    std::unique_ptr<symbol_sm::state_base_t> current_state_m;
};

struct sample_sm_t;

// -----------------------------------------------------------------------------

namespace sample_sm
{

struct state_base_t
{
    virtual void process(sample_sm_t& ctx, const boost::optional<bool>& sample) = 0;
};

struct idle_t : public state_base_t
{
    void process(sample_sm_t& ctx, const boost::optional<bool>& sample) override;
};

struct lead_in_t : public state_base_t
{
    lead_in_t(bool last_sample)
      : counter(0)
      , last_sample(last_sample)
    {
    }
    void process(sample_sm_t& ctx, const boost::optional<bool>& sample) override;
private:
    size_t counter;
    bool last_sample;
};

struct preamble_t : public state_base_t
{
    preamble_t(bool last_sample)
        : last_sample(last_sample)
    {}
    void process(sample_sm_t& ctx, const boost::optional<bool>& sample) override;
    size_t symbols_counter = 0;
    size_t samples_counter = 0;
    bool last_sample;
};

struct bitlock_t : public state_base_t
{
    bitlock_t(double samples_per_symbol, bool last_sample)
      : samples_per_symbol(samples_per_symbol)
      , num_samples(3.0 * samples_per_symbol / 4.0),
        last_sample(last_sample)
    {}
    void process(sample_sm_t& ctx, const boost::optional<bool>& sample) override;
    const double samples_per_symbol;
    double num_samples;
    bool last_sample;
};

} // namespace

// -----------------------------------------------------------------------------

struct sample_sm_t
{
    sample_sm_t(size_t sample_rate, symbol_sm_t& sym_sm);
    // sample can be 0, 1 or none (no signal)
    void process(const boost::optional<bool>& sample);
    void state(std::unique_ptr<sample_sm::state_base_t>&& next_state);
    bool preamble() { return
            typeid(*current_state_m.get()) == typeid(sample_sm::preamble_t) ||
            typeid(*current_state_m.get()) == typeid(sample_sm::lead_in_t);
    }
    bool idle() { return typeid(*current_state_m.get()) == typeid(sample_sm::idle_t); }
    void emit(const boost::optional<bool>& symbol);
    const size_t sample_rate;
private:
    std::reference_wrapper<symbol_sm_t> sym_sm;
    std::unique_ptr<sample_sm::state_base_t> current_state_m;
};
} // namespace

struct demod_nrz
{
    demod_nrz(size_t sample_rate,
              std::function<void(uint8_t* begin, uint8_t* end)> packet_callback)
        : lp1(butter_lp<6>(sample_rate, 150000))
        , lp2(butter_lp<6>(sample_rate, 150000))
        , freq_filter(butter_lp<3>(sample_rate, 50000))
        , lock_filter(butter_lp<3>(sample_rate, 750))
        , symbols_sm(packet_callback)
        , samples_sm(sample_rate, symbols_sm)

    {
    }

    void operator()(std::complex<double> iq)
    {
        iq = std::complex<double>(lp1(iq.real()), lp2(iq.imag()));
        double f = fsk_demod(iq);
        double s = freq_filter(f);
        double lock_freq = lock_filter(f);
        boost::optional<bool> sample;

        // check for signal, adjust central freq, and get sample
        bool signal = std::abs(lock_freq) > 0.01;
        if(signal)
        {
            if (samples_sm.idle()) omega_c = lock_freq;
            sample = (s - omega_c) < 0.0;
            if (samples_sm.preamble()) omega_c = 0.95 * omega_c + lock_freq * 0.05;
        }
        // process the sample with the state machine
        samples_sm.process(sample);
    }

    atan_fm_demodulator fsk_demod;
    iir_filter<6> lp1, lp2;
    iir_filter<3> freq_filter;
    iir_filter<3> lock_filter;
    state_machine::symbol_sm_t symbols_sm;
    state_machine::sample_sm_t samples_sm;
    double omega_c = 0.0;
};

} // namespace
} // namespace
