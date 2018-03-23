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
// FSK @40000bps, NZR, Separation=40KHz

#include "wavingz.h"

#include <vector>
#include <cstdio>
#include <cstdint>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>

using namespace std;
namespace po = boost::program_options;

int
main(int argc, char* argv[])
{
    std::string payload;
    size_t sample_rate;
    size_t baud_rate;

    po::options_description desc("WavingZ - Wave-out options");
    desc.add_options()
        ("help,h", "Produce this help message")
        ("payload,p", po::value<std::string>(&payload), "Payload in format 01 23 45 67 89 AB CD EF ..")
        ("sample_rate,s", po::value<size_t>(&sample_rate)->default_value(2000000), "Sample rate (default 2M)")
        ("baud_rate,b", po::value<size_t>(&baud_rate)->default_value(40000), "Baudrate (default 40kbaud)")
        ("unsigned,u", "Produce uint8 output instead if int8")
       ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cerr << desc << "\n";
        cerr << "\n";
        cerr << "Example:\n";
        cerr << "  HomeId:       0xD6B26208\n";
        cerr << "  SourceId:     0x01\n";
        cerr << "  FrameControl: 0x410F\n";
        cerr << "  Length:       0x0D (13 Bytes)\n";
        cerr << "  DestId:       0x03\n";
        cerr << "  CommandClass: 0x25 (SwitchBinary)\n";
        cerr << "  Payload:      0x01 0xFF 0x6B\n";
        cerr << "\n";
        cerr << "     wave-out -p 'd6 b2 62 08 01 41 0f 0d 03 25 01 ff 6b'\n";
        cerr << "\n";
        cerr << "  The Frame Check Sequence (8bit) is added automatically.\n";
        cerr << "\n";
        return EXIT_SUCCESS;
    }

    std::vector<uint8_t> buffer;
    if (!vm.count("payload"))
    {
        cerr << "Payload is mandatory." << std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        std::stringstream interpreter(payload);
        while (interpreter) {
            int ch;
            interpreter >> std::hex >> ch;
            if(!interpreter) break;
            buffer.push_back((uint8_t)ch);
        }
    }
    buffer.push_back(wavingz::checksum(buffer.begin(), buffer.end()));
    std::ofstream file;
    wavingz::zwave_print(file,std::cerr, &buffer.front(), &buffer.front()+buffer.size()) << std::endl;

    // encode and output wavingz buffer
    if(vm.count("unsigned"))
    {
        wavingz::encoder<uint8_t> waver(sample_rate, baud_rate);
        auto complex_bytes = waver(buffer.begin(), buffer.end());
        for (auto pair : complex_bytes) std::cout << pair.first << pair.second;
    }
    else
    {
        wavingz::encoder<int8_t> waving(sample_rate, baud_rate);
        auto complex_bytes = waving(buffer.begin(), buffer.end());
        for (auto pair : complex_bytes) std::cout << pair.first << pair.second;
    }

    return EXIT_SUCCESS;
}
