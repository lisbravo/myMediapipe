# Copyright 2020 Lisandro Bravo.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])  # Apache 2.0

package(default_visibility = ["//visibility:private"])

load("//mediapipe/framework/port:build_config.bzl", "mediapipe_cc_proto_library")


cc_library(
    name = "simple-mqtt-client",
    srcs = ["Mqtt.cpp"],
    hdrs = ["Mqtt.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@system_libs//:libmosquittopp-dev",
        "@system_headers//:headers",
    ],
    alwayslink = 1,
)



