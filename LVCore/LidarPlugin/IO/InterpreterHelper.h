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

#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTable.h>

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

//------------------------------------------------------------------------------
/*!
 * @brief         Initialize an array for datapoint attributes and add it to the
 *                polyData.
 * @tparam        T                         The type of the array. This is templated so
 *                                          that the caller does not need to consider the
 *                                          type, which may change with the
 *                                          specification.
 * @param[in]     isAdvanced                The variable used to defined the current array as advanced 
 * @param[in,out] array                     The input array.
 * @param[in]     name                      The name of the array to be created
 * @param[in]     np                        The number of elements that the array must be
 *                                          able to hold after initialization.
 * @param[in]     prereservedNp             The number of elements to prereserve.
 * @param[out]    pd                        The PolyData instance to which the array
 *                                          should be added.
 * @param[in]     isAdvancedArrayEnabled    The current advanced array status
 */
template<typename T>
void InitArrayForPolyData(bool isAdvanced, T& array, const char* name, vtkIdType np, vtkIdType prereservedNp, vtkPolyData* pd, bool isAdvancedArrayEnabled = false)
{
  if (isAdvanced && !isAdvancedArrayEnabled)
  {
    array = nullptr;
    return;
  }

  array = T::New();
  array->Allocate(prereservedNp);
  array->SetName(name);
  if (np > 0)
  {
    array->SetNumberOfTuples(np);
  }
  if (pd)
  {
    pd->GetPointData()->AddArray(array);
  }
}

template<typename T, typename I, typename U>
void SetValueIfNotNull(T& array, I id, U value)
{
  if (array != nullptr)
  {
    array->SetValue(id, value);
  }
}

template<typename T, typename U>
void InsertNextValueIfNotNull(T& array, U value)
{
  if (array != nullptr)
  {
    array->InsertNextValue(value);
  }
}

template<typename T, typename I, typename U>
void InsertValueIfNotNull(T& array, I index, U value)
{
  if (array != nullptr)
  {
    array->InsertValue(index,value);
  }
}

template<typename T, typename U>
void AddArrayIfNotNull(T* holder, U& array)
{
  if (holder != nullptr && array != nullptr)
  {
    holder->AddArray(array);
  }
}

template<typename T, typename U>
void AddArrayIfNotEmpty(T* holder, U& array)
{
  if (holder != nullptr && array != nullptr && array->GetNumberOfValues() != 0)
  {
    holder->AddArray(array);
  }
}

template <typename T>
void AddArrayToTableIfNotNull(T& array, vtkTable* table, char const * name)
{
  if (table != nullptr)
  {
    array = T::New();
    array->SetName(name);
    table->AddColumn(array);
  }
}

#endif // InterpreterHelper_h
