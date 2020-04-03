// Copyright 2020 Lisandro Bravo.
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

#include <string>
#include <vector>
//#include <algorithm>

#include "myMediapipe/calculators/tflite/angles_to_tflite_converter_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"
#include "myMediapipe/framework/formats/angles.pb.h"
#include "mediapipe/framework/port/ret_check.h"
//#include "mediapipe/util/resource_util.h"
#include "tensorflow/lite/error_reporter.h"
#include "tensorflow/lite/interpreter.h"



namespace mediapipe {

constexpr char kAngleDataTag[] = "ANGLES";   

// Converts Angle streams to Tensors to feed them to an Inference calculator
//
// Input:
// "ANGLES:angles" 
//
// Output:
//  One of the following tags:
//  TENSORS - Vector of TfLiteTensor of type kTfLiteFloat32, or kTfLiteUint8.
//
// Example use:
// node {
//   calculator: "anglesToTfLiteConverterCalculator"
//   input_stream: "ANGLES:angles"
//   output_stream: "TENSORS:angle_tensor"
//   options: {
//     [mediapipe.anglesToTfLiteConverterCalculatorOptions.ext] {
//       zero_center: true
//     }
//   }
// }

class anglesToTfLiteConverterCalculator : public CalculatorBase {
 public:
  anglesToTfLiteConverterCalculator() {}
  ~anglesToTfLiteConverterCalculator() override {}
  anglesToTfLiteConverterCalculator(const anglesToTfLiteConverterCalculator&) =
      delete;
  anglesToTfLiteConverterCalculator& operator=(
    const anglesToTfLiteConverterCalculator&) = delete;
  
  static ::mediapipe::Status GetContract(CalculatorContract* cc);

  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;
  ::mediapipe::Status Close(CalculatorContext* cc) override;

 private:
  //::mediapipe::Status CopyMatrixToTensor(const Matrix& matrix,
  //                                       float* tensor_buffer);
  

  std::unique_ptr<tflite::Interpreter> interpreter_ = nullptr;
  anglesToTfLiteConverterCalculatorOptions options_;

  bool zero_center_ = true;  // normalize range to [-1,1] | otherwise [0,1]
  bool row_major_matrix_ = false;
  bool use_quantized_tensors_ = false;

};
REGISTER_CALCULATOR(anglesToTfLiteConverterCalculator);

::mediapipe::Status anglesToTfLiteConverterCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kAngleDataTag))
      << "Input streams are not provided.";
  
  if (cc->Inputs().HasTag(kAngleDataTag)) {
    cc->Inputs().Tag(kAngleDataTag).Set<std::vector<Angle>>();
  }

  if (cc->Outputs().HasTag("TENSORS"))
    cc->Outputs().Tag("TENSORS").Set<std::vector<TfLiteTensor>>();
  
  return ::mediapipe::OkStatus();
}

::mediapipe::Status anglesToTfLiteConverterCalculator::Open(CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));

  options_ = cc->Options<anglesToTfLiteConverterCalculatorOptions>();

  // Get data normalization mode.
  zero_center_ = options_.zero_center();

  // Get row_major_matrix mode.
  row_major_matrix_ = options_.row_major_matrix();

  // Get tensor type, float or quantized.
  use_quantized_tensors_ = options_.use_quantized_tensors();

  interpreter_ = absl::make_unique<tflite::Interpreter>();
  interpreter_->AddTensors(1);
  interpreter_->SetInputs({0});

  return ::mediapipe::OkStatus();
}

::mediapipe::Status anglesToTfLiteConverterCalculator::Process(CalculatorContext* cc) {
  
  if (cc->Inputs().Tag(kAngleDataTag).IsEmpty()){
    return ::mediapipe::OkStatus();
  }
  
  const int channels = 1; //Move to proto
  const int numAngles = 2; //number of angles on each Angle Input (see angle.proto)

  
  const auto &angles = cc->Inputs()
                          .Tag(kAngleDataTag)
                          .Get<std::vector<Angle>>();

  int size_ = angles.size() * numAngles;
  std::vector<int> sizes = {size_};
  float oneRowAngles[size_];
 
  // std::cout << "\n inAngle:\n";
  for(int i=0;i<angles.size();i++){
    oneRowAngles[i*2]   = angles[i].angle1();    
    oneRowAngles[(i*2)+1] = angles[i].angle2();
    // std::cout  << " i:" << std::to_string(i) << " "
    //            << std::to_string(angles[i].angle1()) 
    //            << "\t" << std::to_string(angles[i].angle2()) << "\n";
   // std::cout  << std::to_string(oneRowAngles[i]) 
   //            << "\t" << std::to_string(oneRowAngles[i+1]) << "\n";
  }

  // std::cout << "\n OneRowAngle size:" << std::to_string(size_) << "\n";
  // for(int i=0;i<size_;i++){
  //   std::cout  << "i:" << std::to_string(i)
  //              << " :" << std::to_string(oneRowAngles[i]) << "\t";
  // }
  
  const int tensor_idx = interpreter_->inputs()[0];

  interpreter_->SetTensorParametersReadWrite(
        /*tensor_index=*/0, /*type=*/kTfLiteFloat32, /*name=*/"",
        /*dims=*/{channels}, /*quantization=*/TfLiteQuantization());
   
  interpreter_->ResizeInputTensor(tensor_idx, sizes);
  interpreter_->AllocateTensors();
  float* inTensor = interpreter_->typed_input_tensor<float>(0);

  std::memcpy(inTensor, &oneRowAngles, sizeof(oneRowAngles));
  //asasdasdasdasd

  TfLiteTensor* tensor = interpreter_->tensor(tensor_idx);

  auto output_tensors = absl::make_unique<std::vector<TfLiteTensor>>();
  output_tensors->emplace_back(*tensor);
  cc->Outputs().Tag("TENSORS").Add(output_tensors.release(),
                                    cc->InputTimestamp());


  return ::mediapipe::OkStatus();
}

::mediapipe::Status anglesToTfLiteConverterCalculator::Close(CalculatorContext* cc) {
  return ::mediapipe::OkStatus();
}


}  // namespace mediapipe
