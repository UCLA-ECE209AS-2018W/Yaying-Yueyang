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

#include "wavingz.h"

namespace wavingz
{

// demodulator state machine implementation

namespace demod
{

namespace state_machine
{

void
symbol_sm_t::process(const boost::optional<bool>& sample)
{
    current_state_m->process(*this, sample);
}

void
symbol_sm_t::state(std::unique_ptr<symbol_sm::state_base_t>&& next_state)
{
    current_state_m = std::move(next_state);
}

namespace symbol_sm
{

void
start_of_frame_1_t::process(symbol_sm_t& ctx,
                            const boost::optional<bool>& symbol)
{
    if (symbol == boost::none)
    {
        cnt = 0;
    }
    else
    {
        cnt = *symbol ? cnt + 1 : 0;
        if (cnt == 5) // we wait for 5 consecutive '1' (the last 1 of the
                      // preamble, plus the first nibble of the SOF)
        {
            ctx.state(
              std::unique_ptr<start_of_frame_0_t>(new start_of_frame_0_t()));
        }
    }
}

void
start_of_frame_0_t::process(symbol_sm_t& ctx,
                            const boost::optional<bool>& symbol)
{
    if (symbol == boost::none)
    {
        ctx.state(
          std::unique_ptr<start_of_frame_1_t>(new start_of_frame_1_t()));
    }
    else if (*symbol)
    {
        ctx.state(
          std::unique_ptr<start_of_frame_1_t>(new start_of_frame_1_t()));
    }
    else if (++cnt == 4) // we expect four consecutive '0'
    {
        ctx.state(std::unique_ptr<payload_t>(new payload_t()));
    }
}

void
payload_t::process(symbol_sm_t& ctx, const boost::optional<bool>& symbol)
{
    if (symbol == boost::none)
    {
        ctx.callback(payload.data(), payload.data() + payload.size());
        ctx.state(
            std::unique_ptr<start_of_frame_1_t>(new start_of_frame_1_t()));
    }
    else
    {
        b[7 - cnt++] = *symbol;
        if (cnt == b.size())
        {
            payload.push_back(uint8_t(b.to_ulong()));
            b.reset();
            cnt = 0;
        }
    }
}

} // namespace

sample_sm_t::sample_sm_t(size_t sample_rate, symbol_sm_t& sym_sm)
  : sample_rate(sample_rate)
  , sym_sm(sym_sm)
  , current_state_m(new sample_sm::idle_t())
{
}

void
sample_sm_t::state(std::unique_ptr<sample_sm::state_base_t>&& next_state)
{
    current_state_m = std::move(next_state);
}

void
sample_sm_t::process(const boost::optional<bool>& sample)
{
    current_state_m->process(*this, sample);
}

void
sample_sm_t::emit(const boost::optional<bool>& bit)
{
    sym_sm.get().process(bit);
}

namespace sample_sm
{

void
idle_t::process(sample_sm_t& ctx, const boost::optional<bool>& sample)
{
    // When we get a signal we go into preamble
    if (sample != boost::none)
    {
        ctx.state(std::unique_ptr<lead_in_t>(new lead_in_t(*sample)));
    }
}

void
lead_in_t::process(sample_sm_t& ctx, const boost::optional<bool>& sample)
{
    const size_t LEAD_IN_SYMBOLS = 10;

    // On no signal, return to idle
    if (sample == boost::none)
    {
        ctx.state(std::unique_ptr<idle_t>(new idle_t()));
    }
    else
    {
        // skip the first few samples to synchronize
        if (*sample != last_sample)
        {
            if (++counter == LEAD_IN_SYMBOLS)
            {
                ctx.state(std::unique_ptr<preamble_t>(new preamble_t(*sample)));
            }
        }
        last_sample = *sample;
    }
}

void
preamble_t::process(sample_sm_t& ctx, const boost::optional<bool>& sample)
{
    const size_t SYNC_SYMBOLS = 20;

    // No signal, return to idle
    if (sample == boost::none)
    {
        ctx.state(std::unique_ptr<idle_t>(new idle_t()));
    }
    else
    {
        // preamble is at least 80 bits, we use some of this bits to accurately
        // identify the samples per symbol (and the data rate)
        ++samples_counter;
        if (*sample != last_sample)
        {
            ++symbols_counter;
            if (symbols_counter > SYNC_SYMBOLS)
            {
                double sps = double(samples_counter) / (symbols_counter - 1);
                // data_rate = ctx.sample_rate / sps;
                ctx.state(std::unique_ptr<bitlock_t>(new bitlock_t(sps, *sample)));
            }
        }
        last_sample = *sample;
    }
}

void
bitlock_t::process(sample_sm_t& ctx, const boost::optional<bool>& sample)
{
    // On no signal, return to idle
    if (sample == boost::none)
    {
        ctx.emit(boost::none);
        ctx.state(std::unique_ptr<idle_t>(new idle_t()));
    }
    else
    {
        if(*sample != last_sample)
        {
            last_sample = *sample;
            num_samples = 3.0 * samples_per_symbol / 4.0;
        }
        else
        {
            num_samples = num_samples + 1.0;
        }
        if (num_samples >= samples_per_symbol)
        {
            ctx.emit(*sample);
            num_samples -= samples_per_symbol; // keep alignment
        }
    }
}
} // namespace
} // namespace
} // namespace
} // namespace
