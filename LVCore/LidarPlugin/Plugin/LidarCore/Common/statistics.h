//=========================================================================
//
// Copyright 2019 Kitware, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

#include <vector>
#include <numeric>


// source: https://stackoverflow.com/questions/1719070
template<typename T>
double ComputeMedian(const std::vector<T>& v)
{
    bool isEven = !(v.size() % 2);
    size_t n    = v.size() / 2;

    std::vector<size_t> vi(v.size());
    std::iota(vi.begin(), vi.end(), 0);

    std::partial_sort(vi.begin(), vi.begin() + n + 1, vi.end(),
        [&](size_t lhs, size_t rhs) { return v[lhs] < v[rhs]; });

    return isEven ? 0.5 * (v[vi.at(n-1)] + v[vi.at(n)]) : v[vi.at(n)];
}
