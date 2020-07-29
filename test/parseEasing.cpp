/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <regex>
#include <iomanip>
#include <algorithm>

#include "utils.h"

#include "apl/animation/easing.h"

static const char *USAGE_STRING = "parseEasing [OPTIONS] EXPRESSION+";

static auto COLORS = std::vector<std::string>{
    "black",
    "gray",
    "red",
    "blue",
    "green",
};

static const char *PATH_TEMPLATE = R"(      <path d="PATH" stroke="COLOR" stroke-width="4"></path>\n)";

static const char *SVG_TEMPLATE = R"(<?xml version="1.0" encoding="UTF-8"?>
<svg width="560px" height="540px" viewBox="0 0 560 540" version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
  <style>
    .small { font: 16px; sans-serif; fill: black; text-anchor: middle;}
    .left { font: 16px; sans-serif; fill: black; text-anchor: end;}
  </style>
  <title>Easing Curve NAME</title>
  <g stroke="none" stroke-width="1" fill="none" fill-rule="evenodd">
    <g transform="translate(40.000000, 10.000000) " >
      <line x1="0.499001996" y1="250" x2="499.500998" y2="250" stroke="#979797" stroke-linecap="square" stroke-dasharray="7"></line>
      <line x1="250" y1="5.48902196" x2="250" y2="499.500998" stroke="#979797" stroke-linecap="square" stroke-dasharray="8"></line>
      <rect stroke="#979797" x="0" y="0" width="500.001996" height="500.001996"></rect>
      PATHLIST
      <g transform="translate(52.000000, 0.000000) " stroke="#E2E2E2" stroke-dasharray="3" stroke-linecap="square">
        <line x1="0.5" y1="-1.11022302e-15" x2="0.5" y2="500" ></line>
        <line x1="48" y1="1.73472348e-15" x2="48" y2="500" ></line>
        <line x1="98" y1="-1.73472348e-15" x2="98" y2="500" ></line>
        <line x1="148" y1="-1.11022302e-15" x2="148" y2="500" ></line>
        <line x1="248" y1="-1.73472348e-15" x2="248" y2="500" ></line>
        <line x1="298" y1="-1.11022302e-15" x2="298" y2="500" ></line>
        <line x1="348" y1="-1.11022302e-15" x2="348" y2="500" ></line>
        <line x1="398" y1="-1.11022302e-15" x2="398" y2="500" ></line>
      </g>
      <g transform="translate(250.500000, 249.500000) rotate(-270.000000) translate(-250.500000, -249.500000) translate(50.500000, -0.500000) "
         stroke="#E2E2E2" stroke-dasharray="3" stroke-linecap="square">
        <line x1="0.501253133" y1="-1.11022302e-15" x2="0.501253133" y2="500" ></line>
        <line x1="48.1203008" y1="1.73472348e-15" x2="48.1203008" y2="500" ></line>
        <line x1="98.245614" y1="-1.73472348e-15" x2="98.245614" y2="500" ></line>
        <line x1="148.370927" y1="-1.11022302e-15" x2="148.370927" y2="500" ></line>
        <line x1="248.621554" y1="-1.73472348e-15" x2="248.621554" y2="500" ></line>
        <line x1="298.746867" y1="-1.11022302e-15" x2="298.746867" y2="500" ></line>
        <line x1="348.87218" y1="-1.11022302e-15" x2="348.87218" y2="500" ></line>
        <line x1="398.997494" y1="-1.11022302e-15" x2="398.997494" y2="500" ></line>
      </g>
      <text x="0" y="515" class="small">XMIN</text>
      <text x="500" y="515" class="small">XMAX</text>
      <text x="-4" y="500" class="left">YMIN</text>
      <text x="-4" y="12" class="left">YMAX</text>
    </g>
  </g>
</svg>
)";

std::string numToString(double value)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << value;
    return ss.str();
}

int
main(int argc, char *argv[])
{
    ArgumentSet argumentSet(USAGE_STRING);

    std::string filename;
    std::string title;

    auto x1 = std::numeric_limits<float>::max();
    auto x2 = std::numeric_limits<float>::min();
    auto y1 = std::numeric_limits<float>::max();
    auto y2 = std::numeric_limits<float>::min();

    bool x1set = false;
    bool x2set = false;
    bool y1set = false;
    bool y2set = false;

    argumentSet.add(
        {Argument("-o", "--output", Argument::ONE, "Output file name", "FILENAME",
                  [&](const std::vector<std::string>& values) { filename = values[0]; }),
         Argument("-t", "--title", Argument::ONE, "Graph title", "TITLE",
                  [&](const std::vector<std::string>& name) { title = name[0]; }),
         Argument("", "--xmin", Argument::ONE, "Minimum X-value", "X_MIN",
                  [&](const std::vector<std::string>& values) {
                      x1 = std::stod(values[0]);
                      x1set = true;
                  }),
         Argument("", "--xmax", Argument::ONE, "Maximum X-value", "X_MAX",
                  [&](const std::vector<std::string>& values) {
                      x2 = std::stod(values[0]);
                      x2set = true;
                  }),
         Argument("", "--ymin", Argument::ONE, "Minimum Y-value", "Y_MIN",
                  [&](const std::vector<std::string>& values) {
                      y1 = std::stod(values[0]);
                      y1set = true;
                  }),
         Argument("", "--ymax", Argument::ONE, "Maximum Y-value", "Y_MAX",
                  [&](const std::vector<std::string>& values) {
                      y2 = std::stod(values[0]);
                      y2set = true;
                  })});

    std::vector<std::string> args(argv + 1, argv + argc);
    argumentSet.parse(args);

    auto session = apl::makeDefaultSession();
    std::vector<std::shared_ptr<apl::Easing>> easingCurves;

    for (const auto& m : args) {
        auto p = apl::Easing::parse(session, m);
        auto bounds = p->bounds();
        if (!x1set)
            x1 = std::min(x1, bounds.start);
        if (!x2set)
            x2 = std::max(x2, bounds.end);
        if (!y1set)
            y1 = std::min(y1, bounds.minimum);
        if (!y2set)
            y2 = std::max(y2, bounds.maximum);
        easingCurves.push_back(p);
    }

    std::cout << "min=" << x1 << "," << y1 << " max=" << x2 << "," << y2 << std::endl;
    auto pathString = std::string();
    int colorIndex = 0;
    for (const auto& p : easingCurves) {
        apl::streamer ss;
        for (int i = 0; i <= 100; i++) {
            float t = i * 0.01 * (x2 - x1) + x1;
            float v = (p->calc(t) - y1) / (y2 - y1);
            ss << (i == 0 ? "M" : "L") << i * 5 << "," << 500 - v * 500;
        }
        auto s = std::regex_replace(PATH_TEMPLATE, std::regex("COLOR"), COLORS.at(colorIndex));
        pathString += std::regex_replace(s, std::regex("PATH"), ss.str());
        colorIndex = (colorIndex + 1) % COLORS.size();
    }

    auto s = std::regex_replace(SVG_TEMPLATE, std::regex("NAME"), title.empty() ? filename : title);
    s = std::regex_replace(s, std::regex("PATHLIST"), pathString);
    s = std::regex_replace(s, std::regex("XMIN"), numToString(x1));
    s = std::regex_replace(s, std::regex("XMAX"), numToString(x2));
    s = std::regex_replace(s, std::regex("YMIN"), numToString(y1));
    s = std::regex_replace(s, std::regex("YMAX"), numToString(y2));

    if (!filename.empty()) {
        std::ofstream out(filename);
        out << s;
        out.close();
    }
    else {
        std::cout << s;
    }
}
