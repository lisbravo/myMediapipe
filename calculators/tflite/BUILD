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
#

licenses(["notice"])  # Apache 2.0

package(default_visibility = ["//visibility:private"])

load("//mediapipe/framework/port:build_config.bzl", "mediapipe_cc_proto_library")


proto_library(
    name = "angles_to_tflite_converter_calculator_proto",
    srcs = ["angles_to_tflite_converter_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = ["//mediapipe/framework:calculator_proto"],
)

mediapipe_cc_proto_library(
    name = "angles_to_tflite_converter_calculator_cc_proto",
    srcs = ["angles_to_tflite_converter_calculator.proto"],
    cc_deps = ["//mediapipe/framework:calculator_cc_proto"],
    visibility = ["//visibility:public"],
    deps = [":angles_to_tflite_converter_calculator_proto"],
)

cc_library(
    name = "angles_to_tflite_converter_calculator",
    srcs = ["angles_to_tflite_converter_calculator.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":angles_to_tflite_converter_calculator_cc_proto",
        #"//mediapipe/util:resource_util",
        "//mediapipe/framework:calculator_framework",
        "//myMediapipe/framework/formats:angles_cc_proto",
        "//mediapipe/framework/tool:status_util",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework/port:ret_check",
        "@org_tensorflow//tensorflow/lite:framework",
        "@org_tensorflow//tensorflow/lite/kernels:builtin_ops",
    ],
    alwayslink = 1,
)
