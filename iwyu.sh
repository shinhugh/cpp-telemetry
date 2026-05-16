#!/bin/bash

# Must be run from project's root directory

mkdir -p build/_include/telemetry
cp telemetry/include/* build/_include/telemetry/

include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --quoted_includes_first -Ibuild/_include -Ibuild/_include/telemetry -D PLATFORM_POSIX telemetry/src/living_span.cpp
include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --quoted_includes_first -Ibuild/_include -Ibuild/_include/telemetry -D PLATFORM_POSIX telemetry/src/log.cpp
include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --quoted_includes_first -Ibuild/_include -Ibuild/_include/telemetry -D PLATFORM_POSIX telemetry/src/log_value.cpp
include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --quoted_includes_first -Ibuild/_include -Ibuild/_include/telemetry -D PLATFORM_POSIX telemetry/src/trace.cpp
