#!/bin/bash

# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# First arg is path to source, ${CMAKE_CURRENT_SOURCE_DIR} in cmake
# called from the directories that contains the source files
# Second arg is the name of the output file created

$arg0 = $args[0];
$arg1 = $args[1];

awk `
		'/^MTH_DISPATCH_FUNC/ { \
			f = $1; \
			sub(\"^MTH_DISPATCH_FUNC\\(\", \"\", f); \
			sub(\"\\).*\", \"\", f); next; \
		} \
		/^[[:space:]]*_MTH_I_STATS_INC/ { \
			split($0, s, \"[(,)]\"); \
			print \"DO_MTH_DISPATCH_FUNC(\" f \", \" s[2] \
				\", \" s[3] \", \", s[4] \")\"; f=\"\"; \
		}' `
	$arg0\mth_128defs.c `
	$arg0\mth_128defs.c `
	$arg0\mth_128defs.c `
	| Out-File -FilePath $arg1 -Encoding utf8
