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


#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/ret_check.h"

namespace mediapipe {

namespace {

constexpr char kLandmarksTag[] = "LANDMARKS";
constexpr char kNormLandmarksTag[] = "NORM_LANDMARKS";



}  // namespace

// HandCommander was built arround a Mediapipe version
// that only had support for a single hand, now that there are multihand
// support, this simple calculator reformat the new LandmarkList protobuff
// to the old vector of Landmarks, as a temporary fix until the rest of
// HandCommander can be updated to LandmarkList
//
// Example config:
// node {
//   calculator: "LandmarksListToVectorLandmarksCalculator"
//   input_stream: "NORM_LANDMARKS:landmarks"
//   output_stream: "NORM_LANDMARKS:landmarks"
//   options {
//     }
//   }
// }
//LandmarksListToVectorLandmarksCalculator

class LandmarksListToVectorLandmarksCalculator : public CalculatorBase {
 public:
  LandmarksListToVectorLandmarksCalculator() {}
  ~LandmarksListToVectorLandmarksCalculator() override {}
  LandmarksListToVectorLandmarksCalculator(const LandmarksListToVectorLandmarksCalculator&) =
      delete;
  LandmarksListToVectorLandmarksCalculator& operator=(
      const LandmarksListToVectorLandmarksCalculator&) = delete;

  static ::mediapipe::Status GetContract(CalculatorContract* cc);

  ::mediapipe::Status Open(CalculatorContext* cc) override;

  ::mediapipe::Status Process(CalculatorContext* cc) override;

 private:
  float angleBetweenLines(float x0, float y0, float x1, float y1, float x2, float y2, bool rigth_hand);

  //LandmarksListToVectorLandmarksCalculatorOptions options_;
};
REGISTER_CALCULATOR(LandmarksListToVectorLandmarksCalculator);

::mediapipe::Status LandmarksListToVectorLandmarksCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kLandmarksTag) ||
            cc->Inputs().HasTag(kNormLandmarksTag))
      << "None of the input streams are provided.";
  RET_CHECK(!(cc->Inputs().HasTag(kLandmarksTag) &&
              cc->Inputs().HasTag(kNormLandmarksTag)))
      << "Only one type of landmark can be taken. Either absolute or "
         "normalized landmarks.";

  //  RET_CHECK(cc->Inputs().HasTag(kPresenceTag))
  //     << "Hand Presence input stram required.";

  if (cc->Inputs().HasTag(kLandmarksTag)) {
    cc->Inputs().Tag(kLandmarksTag).Set<LandmarkList>();
  }
  if (cc->Inputs().HasTag(kNormLandmarksTag)) {
    cc->Inputs().Tag(kNormLandmarksTag).Set<NormalizedLandmarkList>();
  }
  if (cc->Outputs().HasTag(kLandmarksTag)) {
    cc->Outputs().Tag(kLandmarksTag).Set<std::vector<Landmark>>();
  }

  if (cc->Outputs().HasTag(kNormLandmarksTag)) {
    cc->Outputs().Tag(kNormLandmarksTag).Set<std::vector<NormalizedLandmark>>();
  }
  return ::mediapipe::OkStatus();
}

::mediapipe::Status LandmarksListToVectorLandmarksCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  //options_ = cc->Options<LandmarksListToVectorLandmarksCalculatorOptions>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status LandmarksListToVectorLandmarksCalculator::Process(
    CalculatorContext* cc) {
  // Only process if there's input landmarks.
  
  
  if (cc->Outputs().HasTag(kLandmarksTag)) {

    if ((cc->Inputs().Tag(kLandmarksTag).IsEmpty()))
      return ::mediapipe::OkStatus();

    auto output_landmarks = absl::make_unique<std::vector<Landmark>>();

    const auto& landmarkList = cc->Inputs().Tag(kLandmarksTag).Get<LandmarkList>();   
    //Landmark landmark (landmarkList.front().landmark();
    
    for (int i = 0; i < landmarkList.landmark_size(); ++i) {
        output_landmarks->push_back(landmarkList.landmark(i)); 
    }
    cc->Outputs().Tag(kLandmarksTag)
     .Add(output_landmarks.release(), cc->InputTimestamp());
  }

  if (cc->Outputs().HasTag(kNormLandmarksTag)) {
    
    if ((cc->Inputs().Tag(kNormLandmarksTag).IsEmpty()))
      return ::mediapipe::OkStatus();

    auto output_landmarks = absl::make_unique<std::vector<NormalizedLandmark>>();

    const auto& landmarkList = cc->Inputs().Tag(kNormLandmarksTag).Get<NormalizedLandmarkList>(); 
 
    for (int i = 0; i < landmarkList.landmark_size(); ++i) {
        output_landmarks->emplace_back(landmarkList.landmark(i)); 
    }
    cc->Outputs().Tag(kNormLandmarksTag)
     .Add(output_landmarks.release(), cc->InputTimestamp());
  }


  return ::mediapipe::OkStatus();
}

 

}  // namespace mediapipe


