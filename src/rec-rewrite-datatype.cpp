/*
 * Copyright (c) 2021 - Christian Berger <christian.berger@gu.se>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

// Include the single-file, header-only cluon library.
#include "cluon-complete.hpp"

#include <ctime>
#include <cmath>
#include <iomanip>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{0};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("in")) || (0 == commandlineArguments.count("map")) || (0 == commandlineArguments.count("out")) ) {
        std::cerr << argv[0] << " reads a .rec file specified as --in to rewrite the datatypes according to --map and stores the rewritten Envelopes in --out." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --in=<Recording from an OD4Session> --map=<dataType mapping> --out=<OutputFile>" << std::endl;
        std::cerr << "Example: " << argv[0] << " --in=myRecording.rec --map=123-456,332-111 --out=outRecording.rec" << std::endl;
        std::cerr << "                           maps Envelope 123 to dataType 456, and Envelope 332 to dataType 111." << std::endl;
        retCode = 1;
    }
    else {
        std::unordered_map<int32_t, int32_t, cluon::UseUInt32ValueAsHashKey> mapping{};
        {
            std::string tmp{commandlineArguments["map"]};
            if (!tmp.empty()) {
                tmp += ",";
                auto entries = stringtoolbox::split(tmp, ',');
                for (auto e : entries) {
                    std::string m{e};
                    auto p = stringtoolbox::split(m, '-');
                    if (p.size() == 2) {
                      std::clog << argv[0] << " mapping " << p[0]  << " to " << p[1] << std::endl;
                      mapping[std::stoi(p[0])] = std::stoi(p[1]);
                    }
                }
            }
        }

        std::fstream fin(commandlineArguments["in"], std::ios::in|std::ios::binary);
        std::fstream fout(commandlineArguments["out"], std::ios::out|std::ios::binary|std::ios::trunc);
        if (fin.good() && fout.good()) {
            fin.close();

            constexpr bool AUTOREWIND{false};
            constexpr bool THREADING{false};
            cluon::Player player(commandlineArguments["in"], AUTOREWIND, THREADING);

            while (player.hasMoreData()) {
                auto next = player.getNextEnvelopeToBeReplayed();
                if (next.first) {
                    cluon::data::Envelope env{std::move(next.second)};
                    if (mapping.count(env.dataType()) > 0) {
                        env.dataType(mapping[env.dataType()]);
                    }
                    const std::string serializedEnvelope{cluon::serializeEnvelope(std::move(env))};
                    fout.write(serializedEnvelope.c_str(), serializedEnvelope.size());
                }
            }
            fout.flush();
            fout.close();
        }
        else {
            std::cerr << argv[0] << ": Recording '" << commandlineArguments["in"] << "' not found." << std::endl;
        }
    }
    return retCode;
}
