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

exports_files(["LICENSE"])

load("//mediapipe/framework/port:build_config.bzl", "mediapipe_cc_proto_library")

proto_library(
    name = "gesture_classifier_calculator_proto",
    srcs = ["gesture_classifier_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_proto",
    ],
)

mediapipe_cc_proto_library(
    name = "gesture_classifier_calculator_cc_proto",
    srcs = ["gesture_classifier_calculator.proto"],
    cc_deps = [
        "//mediapipe/framework:calculator_cc_proto",
    ],
    visibility = ["//mediapipe:__subpackages__"],
    deps = [":gesture_classifier_calculator_proto"],
)

cc_library(
    name = "gesture_classifier_calculator",
    srcs = ["gesture_classifier_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":gesture_classifier_calculator_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/formats:detection_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/util:resource_util",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework/port:file_helpers",
    ],
    alwayslink = 1,
)


proto_library(
    name = "transition_dynamic_gestures_calculator_proto",
    srcs = ["transition_dynamic_gestures_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_proto",
    ],
)

mediapipe_cc_proto_library(
    name = "transition_dynamic_gestures_calculator_cc_proto",
    srcs = ["transition_dynamic_gestures_calculator.proto"],
    cc_deps = [
        "//mediapipe/framework:calculator_cc_proto",
    ],
    visibility = ["//mediapipe:__subpackages__"],
    deps = [":transition_dynamic_gestures_calculator_proto"],
)

cc_library(
    name = "transition_dynamic_gestures_calculator",
    srcs = ["transition_dynamic_gestures_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":transition_dynamic_gestures_calculator_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/formats:detection_cc_proto",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//myMediapipe/framework/formats:mqtt_message_cc_proto",
        "//mediapipe/framework/port:ret_check",
    ],
    alwayslink = 1,
)

proto_library(
    name = "moving_dynamic_gestures_calculator_proto",
    srcs = ["moving_dynamic_gestures_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_proto",
    ],
)

mediapipe_cc_proto_library(
    name = "moving_dynamic_gestures_calculator_cc_proto",
    srcs = ["moving_dynamic_gestures_calculator.proto"],
    cc_deps = [
        "//mediapipe/framework:calculator_cc_proto",
    ],
    visibility = ["//mediapipe:__subpackages__"],
    deps = [":moving_dynamic_gestures_calculator_proto"],
)

cc_library(
    name = "moving_dynamic_gestures_calculator",
    srcs = ["moving_dynamic_gestures_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":moving_dynamic_gestures_calculator_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/formats:detection_cc_proto",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//myMediapipe/framework/formats:angles_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "//myMediapipe/framework/formats:mqtt_message_cc_proto",
    ],
    alwayslink = 1,
)


proto_library(
    name = "writing_dynamic_gestures_calculator_proto",
    srcs = ["writing_dynamic_gestures_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_proto",
    ],
)

mediapipe_cc_proto_library(
    name = "writing_dynamic_gestures_calculator_cc_proto",
    srcs = ["writing_dynamic_gestures_calculator.proto"],
    cc_deps = [
        "//mediapipe/framework:calculator_cc_proto",
    ],
    visibility = ["//mediapipe:__subpackages__"],
    deps = [":writing_dynamic_gestures_calculator_proto"],
)

cc_library(
    name = "writing_dynamic_gestures_calculator",
    srcs = ["writing_dynamic_gestures_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":writing_dynamic_gestures_calculator_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/formats:detection_cc_proto",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "//myMediapipe/framework/formats:mqtt_message_cc_proto",
    ],
    alwayslink = 1,
)

proto_library(
    name = "fixed_dynamic_gestures_calculator_proto",
    srcs = ["fixed_dynamic_gestures_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_proto",
    ],
)

mediapipe_cc_proto_library(
    name = "fixed_dynamic_gestures_calculator_cc_proto",
    srcs = ["fixed_dynamic_gestures_calculator.proto"],
    cc_deps = [
        "//mediapipe/framework:calculator_cc_proto",
    ],
    visibility = ["//mediapipe:__subpackages__"],
    deps = [":fixed_dynamic_gestures_calculator_proto"],
)

cc_library(
    name = "fixed_dynamic_gestures_calculator",
    srcs = ["fixed_dynamic_gestures_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":fixed_dynamic_gestures_calculator_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/formats:detection_cc_proto",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//myMediapipe/framework/formats:angles_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "//myMediapipe/framework/formats:mqtt_message_cc_proto",
    ],
    alwayslink = 1,
)