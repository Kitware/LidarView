// Copyright 2021 Kitware SAS.
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

#ifndef InterpreterHelper_h
#define InterpreterHelper_h

//! @brief Simple getter that handles conversion to native unsigned integer types.
#define GET_NATIVE_UINT(n, attr) uint ## n ##_t Get ## attr() const { return this->attr; }
#define SET_NATIVE_UINT(n, attr) void Set ## attr(uint ## n ##_t x) { this->attr = x; }

#define BIT(n)                  ( 1<<(n) )
//! Create a bitmask of length \a len.
#define BIT_MASK(len)           ( BIT(len)-1 )

//! Create a bitfield mask of length \a starting at bit \a start.
#define BF_MASK(start, len)     ( BIT_MASK(len)<<(start) )

//! Extract a bitfield of length \a len starting at bit \a start from \a y.
#define BF_GET(y, start, len)   ( ((y)>>(static_cast<decltype(y)>(start))) & BIT_MASK(len) )

//! Prepare a bitmask for insertion or combining.
#define BF_PREP(x, start, len)  ( ((x)&BIT_MASK(len)) << (start) )

//! Insert a new bitfield value x into y.
#define BF_SET(y, x, start, len)    ( y= ((y) &~ BF_MASK(start, len)) | BF_PREP(x, start, len) )


#endif // InterpreterHelper_h
