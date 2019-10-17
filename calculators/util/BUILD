# Copyright 2019 The MediaPipe Authors.
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

exports_files(["LICENSE"])

load("//mediapipe/framework/port:build_config.bzl", "mediapipe_cc_proto_library")

cc_library(
    name = "landmarks_to_angles_calculator",
    srcs = ["landmarks_to_angles_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//myMediapipe/framework/formats:angles_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
    ],
    alwayslink = 1,
)

proto_library(
    name = "landmarks_and_angles_to_file_calculator_proto",
    srcs = ["landmarks_and_angles_to_file_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_proto",
    ],
)

mediapipe_cc_proto_library(
    name = "landmarks_and_angles_to_file_calculator_cc_proto",
    srcs = ["landmarks_and_angles_to_file_calculator.proto"],
    cc_deps = [
        "//mediapipe/framework:calculator_cc_proto",
    ],
    visibility = ["//mediapipe:__subpackages__",
                  "//myMediapipe:__subpackages__"],
    deps = [":landmarks_and_angles_to_file_calculator_proto"],
)

cc_library(
    name = "landmarks_and_angles_to_file_calculator",
    srcs = ["landmarks_and_angles_to_file_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":landmarks_and_angles_to_file_calculator_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//myMediapipe/framework/formats:angles_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@system_libs//:libncurses",
        "@system_headers//:headers",
    ],
    alwayslink = 1,
)

proto_library(
    name = "angles_to_detection_calculator_proto",
    srcs = ["angles_to_detection_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_proto",
    ],
)

mediapipe_cc_proto_library(
    name = "angles_to_detection_calculator_cc_proto",
    srcs = ["angles_to_detection_calculator.proto"],
    cc_deps = [
        "//mediapipe/framework:calculator_cc_proto",
    ],
    visibility = ["//mediapipe:__subpackages__"],
    deps = [":angles_to_detection_calculator_proto"],
)

cc_library(
    name = "angles_to_detection_calculator",
    srcs = ["angles_to_detection_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":angles_to_detection_calculator_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/formats:detection_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "@com_google_absl//absl/memory",
        "@org_tensorflow//tensorflow/lite:framework",

    ],
    alwayslink = 1,
)
