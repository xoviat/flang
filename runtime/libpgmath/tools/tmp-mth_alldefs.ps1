#!/bin/bash

# Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
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

$arg0 = $args[0]
$arg1 = $args[1]

$content = Get-Content $arg0 | `
    awk -v FS=, 'function f(n){print \"MTHTMPDEF(\"n\")\"}/^MTH/{f($4);f($5);f($6);f($7)}' | `
    sort-object | unique | `
    sed 's/ //g' | grep -v -e __math_dispatch_error | `
    Out-File -FilePath $arg1 -Encoding utf8


