// Copyright 2019 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>

#include "myMediapipe/calculators/util/skeleton_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"


namespace mediapipe {



namespace {

 

}  // namespace

// Sumary 
//
// Extended sumary 
// 
// 
// 
// 
//
// Input:
//  example TENSOR: A Vector of TfLiteTensor of type kTfLiteFloat32 with the confidence score
//          for each static gesture.
//
// Output:
//   example DETECTION: A Detection proto.
//
// Example config:
// node {
//   calculator: "skeletonCalculator"
//   input_stream: ""TENSORS:tensors"
//   output_stream: "DETECTIONS:detections"
// }

class skeletonCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;

  ::mediapipe::Status Process(CalculatorContext* cc) override;
  ::mediapipe::skeletonCalculatorOptions options_;

  
};
REGISTER_CALCULATOR(skeletonCalculator);

::mediapipe::Status skeletonCalculator::GetContract(
    CalculatorContract* cc) {
  //Check examples
  //RET_CHECK(cc->Inputs().HasTag(kTfLiteFloat32));
  //RET_CHECK(cc->Outputs().HasTag(kDetectionTag));
  // TODO: Also support converting Landmark to Detection.
  // cc->Inputs()
  //     .Tag(kTfLiteFloat32) 
  //     .Set<std::vector<TfLiteTensor>>();
  //cc->Outputs().Tag(kDetectionTag).Set<Detection>();
  // cc->Outputs().Index(0).Set<Detections>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status skeletonCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));

  options_ = cc->Options<::mediapipe::skeletonCalculatorOptions>();
  
 
  return ::mediapipe::OkStatus();
}

::mediapipe::Status skeletonCalculator::Process(
    CalculatorContext* cc) {
  //RET_CHECK(!cc->Inputs().Tag(kTfLiteFloat32).IsEmpty());

  // const auto& input_tensors =
  //     cc->Inputs().Tag(kTfLiteFloat32).Get<std::vector<TfLiteTensor>>();

  // output_detections->push_back(*detection);

  // cc->Outputs()
  //     .Index(0)
  //     .Add(output_detections, cc->InputTimestamp());
  return ::mediapipe::OkStatus();
}

}  // namespace mediapipe
