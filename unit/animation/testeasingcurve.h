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

#ifndef _APL_TEST_EASING_CURVE_H
#define _APL_TEST_EASING_CURVE_H

#include <vector>
#include <algorithm>

struct Cubic {
    Cubic(double start, double end, double a, double b, double c, double d)
        : start(start), end(end), a(a), b(b), c(c), d(d) {}
    double calc(double time) const {
        auto t = (time - start) / (end - start);
        return a * (1 - t) * (1 - t) * (1 - t) + 3 * b * t * (1 - t) * (1 - t) +
               3 * c * t * t * (1 - t) + d * t * t * t;
    }
    const double start, end, a, b, c, d;
};

class TestCurve {
public:
    TestCurve(std::vector<Cubic> cubics, int segmentCount = 10000)
        : cubics(cubics)
    {
        double length = 0;
        for (int i = 0 ; i < segmentCount ; i++) {
            auto t1 = static_cast<double>(i) / segmentCount;
            auto t2 = static_cast<double>(i+1) / segmentCount;
            double sumsquares = 0;
            for (const auto& cubic : cubics) {
                auto dv = cubic.calc(t2) - cubic.calc(t1);
                sumsquares += dv * dv;
            }
            auto dl = ::sqrt(sumsquares);
            length += dl;
            cumulative.push_back(length);
        }
    }

    double position(double percentage, int cubicIndex) const {
        double target = percentage * cumulative.back();
        auto it = std::lower_bound(cumulative.begin(), cumulative.end(), target);
        auto i = std::distance(cumulative.begin(), it);  // Offset into the cumulative array
        auto t = static_cast<double>(i) / cumulative.size();
        return cubics.at(cubicIndex).calc(t);
    }

    int dof() const { return cubics.size(); }

    std::vector<Cubic> cubics;
    std::vector<double> cumulative;
};


#endif // _APL_TEST_EASING_CURVE_H
