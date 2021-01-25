#===============================================================================
# Copyright 2021 Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#===============================================================================

if (MSVC)
    # Warning level 3, but disable some noisy warnings
    add_compile_options(/W3
                        /wd4244  # 'argument' : conversion from 'type1' to 'type2', possible loss of data
                        /wd4267  # 'var' : conversion from 'size_t' to 'type', possible loss of data
                        /wd4800  # 'type1': forcing value to bool 'true' or 'false'
                        /wd4141  # 'modifier' : used more than once
                        /wd4251  # 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
                        /wd4503  # 'identifier' : decorated name length exceeded, name was truncated
    )

else()
    # Warning level all and extra
    add_compile_options(-Wall -Wextra)

endif()