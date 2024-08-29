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
        std::cerr << "Example: " << argv[0] << " --in=myRecording.rec --map=123/2-456/3,332/0-111/0 --out=outRecording.rec" << std::endl;
        std::cerr << "                           maps Envelope 123/2 to dataType 456/3, and Envelope 332 to dataType 111." << std::endl;
        retCode = 1;
    }
    else {
        std::unordered_map<std::string, std::string> mapping{};
        {
            std::string tmp{commandlineArguments["map"]};
            if (!tmp.empty()) {
                tmp += ",";
                auto entries = stringtoolbox::split(tmp, ',');
                for (auto e : entries) {
                    std::string m{e};
                    auto dataTypeMapping = stringtoolbox::split(m, '-');
                    if (dataTypeMapping.size() == 2) {
                        std::clog << argv[0] << " mapping " << dataTypeMapping[0]  << " to " << dataTypeMapping[1] << std::endl;
                        mapping[dataTypeMapping[0]] = dataTypeMapping[1];
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
                    std::stringstream sstr;
                    sstr << +env.dataType() << "/" << +env.senderStamp();
                    const std::string str{sstr.str()};
                    if (mapping.count(str) > 0) {
                        std::string newDataTypeSenderStamp = mapping[str];
                        auto p = stringtoolbox::split(newDataTypeSenderStamp, '/');
                        if (p.size() == 2) {
                            env.dataType(std::stoi(p[0]));
                            env.senderStamp(std::stoi(p[1]));
                        }
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
