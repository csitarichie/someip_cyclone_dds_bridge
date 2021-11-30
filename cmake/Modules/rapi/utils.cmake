# collecting generic utility functions
#

# Copyright 2015 Open Source Robotics Foundation, Inc.
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

#
# Convert a camel case string to lower case and underscores.
#
# :param STR_: the string
# :type STR_: string
# :param VAR_: the output variable name
# :type VAR_: string
#
function(string_camel_case_to_lower_case_underscore STR_ VAR_)
    # insert an underscore before any upper case letter
    # which is not followed by another upper case letter
    string(REGEX REPLACE "(.)([A-Z][a-z]+)" "\\1_\\2" value "${STR_}")
    # insert an underscore before any upper case letter
    # which is preseded by a lower case letter or number
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" value "${value}")
    string(TOLOWER "${value}" value)
    set(${VAR_} "${value}" PARENT_SCOPE)
endfunction()

#! snake_to_camel_case converts input string (${STR_}) from snake to camel case in OUT_
#
# EXAMPLEs:
#
#     STR_: hello_world -> OUT_: HelloWorld
#
# DESCRIPTION:
#
# The intended use of snake_to_camel_case is to convert component directory
# names (which use snake_case) to FAPI class/target names (in CamelCase).
#
# Special cases:
#
#   - consecutive '_' in ${str} will be shortened to a single '_'
#   - an input ${str} contains only '_' will result in an empty out_var
#
# MANDATORY parameters:
#
# \param: STR_ input string to be converted (in snake_case)
# \param: OUT_ name of the output variable to be set with the CamelCase
#              converted string
function(snake_to_camel_case STR_ OUT_)
    string(REPLACE "_" ";" WORDS "${STR_}")

    unset(CAMEL_STR)
    foreach (WORD ${WORDS})
        string(LENGTH "${WORD}" WORD_LENGTH)
        if (${WORD_LENGTH} EQUAL 0)
            # two underscores in a row? how do we handle this? error?
            continue()
        elseif (${WORD_LENGTH} EQUAL 1)
            string(TOUPPER "${WORD}" WORD)
            string(APPEND CAMEL_STR "${WORD}")
        else ()
            string(SUBSTRING "${WORD}" 0 1 FIRST_CHAR)
            string(SUBSTRING "${WORD}" 1 -1 REST)

            string(TOUPPER "${FIRST_CHAR}" FIRST_CHAR)
            string(TOLOWER "${REST}" REST)
            string(APPEND CAMEL_STR "${FIRST_CHAR}" "${REST}")
        endif ()
    endforeach ()

    set(${OUT_} "${CAMEL_STR}" PARENT_SCOPE)
endfunction()

function(removeElement stringIn stringOut toRemove)
    separate_arguments(stringIn)
    list(REMOVE_ITEM stringIn ${toRemove})
    string(REPLACE ";" " " stringIn "${stringIn}")
    set(${stringOut} "${stringIn}" PARENT_SCOPE)
endfunction()