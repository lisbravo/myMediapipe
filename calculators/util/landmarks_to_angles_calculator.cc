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

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "myMediapipe/framework/formats/angles.pb.h"
#include "mediapipe/framework/port/ret_check.h"

namespace mediapipe {

namespace {

constexpr char kLandmarksTag[] = "LANDMARKS";
constexpr char kNormLandmarksTag[] = "NORM_LANDMARKS";
constexpr char kAngleDataTag[] = "ANGLES"; 
constexpr char kPresenceTag[] = "PRESENCE";

// Remap x from range [lo hi] to range [0 1] then multiply by scale.
inline float Remap(float x, float lo, float hi, float scale) {
  return (x - lo) / (hi - lo + 1e-6) * scale;
}

// Wraps around an angle in radians to within -M_PI and M_PI.
inline float NormalizeRadians(float angle) {
  return angle - 2 * M_PI * std::floor((angle - (-M_PI)) / (2 * M_PI));
}


}  // namespace

// A calculator that calculates Angles from Landmarks
// The input should be std::vector<Landmark>
//
// Example config:
// node {
//   calculator: "LandmarksToAnglesCalculator"
//   input_stream: "NORM_LANDMARKS:landmarks"
//   output_stream: "ANGLES:angles"
//   options {
//     }
//   }
// }


class LandmarksToAnglesCalculator : public CalculatorBase {
 public:
  LandmarksToAnglesCalculator() {}
  ~LandmarksToAnglesCalculator() override {}
  LandmarksToAnglesCalculator(const LandmarksToAnglesCalculator&) =
      delete;
  LandmarksToAnglesCalculator& operator=(
      const LandmarksToAnglesCalculator&) = delete;

  static ::mediapipe::Status GetContract(CalculatorContract* cc);

  ::mediapipe::Status Open(CalculatorContext* cc) override;

  ::mediapipe::Status Process(CalculatorContext* cc) override;

 private:
  float angleBetweenLines(float x0, float y0, float x1, float y1, float x2, float y2, bool rigth_hand);

  //LandmarksToAnglesCalculatorOptions options_;
};
REGISTER_CALCULATOR(LandmarksToAnglesCalculator);

::mediapipe::Status LandmarksToAnglesCalculator::GetContract(
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
    cc->Inputs().Tag(kLandmarksTag).Set<std::vector<Landmark>>();
  }
  if (cc->Inputs().HasTag(kNormLandmarksTag)) {
    cc->Inputs().Tag(kNormLandmarksTag).Set<std::vector<NormalizedLandmark>>();
  }
  cc->Outputs().Tag(kAngleDataTag).Set<std::vector<Angle>>();
  return ::mediapipe::OkStatus();
}

::mediapipe::Status LandmarksToAnglesCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  //options_ = cc->Options<LandmarksToAnglesCalculatorOptions>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status LandmarksToAnglesCalculator::Process(
    CalculatorContext* cc) {
  // Only process if there's input landmarks.
  
  if ((cc->Inputs().Tag(kNormLandmarksTag).IsEmpty())/* ||
      (cc->Inputs().Tag(kPresenceTag).Get() )*/)
  {
    return ::mediapipe::OkStatus();
  }
 
  const auto &landmarks = cc->Inputs()
                              .Tag(kNormLandmarksTag)
                              .Get<std::vector<NormalizedLandmark>>();
  auto output_angles =
      absl::make_unique<std::vector<Angle>>();
  
  //this only works if palm is facing the camera,
  //TODO: add palm/back dettection
  bool rigthHand = (landmarks[5].x()>landmarks[17].x());     
  

  for (const auto &landmark : landmarks)
  {
    Angle new_angle;

    new_angle.set_landmarkid(&landmark - &landmarks[0]);

    //TODO: Replace Literals with something more elegant
    //      maybe a new format with palmBase, finger[].mcp .pip .dip .tip

    //Pip Dip angles
    if (((new_angle.landmarkid() > 1) && (new_angle.landmarkid() < 4)) ||
        ((new_angle.landmarkid() > 5) && (new_angle.landmarkid() < 8)) ||
        ((new_angle.landmarkid() > 9) && (new_angle.landmarkid() < 12)) ||
        ((new_angle.landmarkid() > 13) && (new_angle.landmarkid() < 16)) ||
        ((new_angle.landmarkid() > 17) && (new_angle.landmarkid() < 20)))
    {

      new_angle.set_angle1(angleBetweenLines(landmark.x(), landmark.y(),
                                             landmarks[new_angle.landmarkid() + 1].x(), landmarks[new_angle.landmarkid() + 1].y(),
                                             landmarks[new_angle.landmarkid() - 1].x(), landmarks[new_angle.landmarkid() - 1].y(),
                                             rigthHand)); //float x0, float y0, float x1, float y1, float x2, float y2
    }

    //MCP angles
    //Angles between fingers
    if ((new_angle.landmarkid() == 1) ||
        (new_angle.landmarkid() == 5) ||
        (new_angle.landmarkid() == 9) ||
        (new_angle.landmarkid() == 13)){
      new_angle.set_angle2(angleBetweenLines(landmark.x(), landmark.y(),
                                               landmarks[new_angle.landmarkid() + 7].x(), landmarks[new_angle.landmarkid() + 7].y(),
                                               landmarks[new_angle.landmarkid() + 3].x(), landmarks[new_angle.landmarkid() + 3].y(),
                                               rigthHand));
    }

    // Palm angle


  if(new_angle.landmarkid()== 0) 
    new_angle.set_angle1( /*atan2(-(landmarks[0].y()-landmarks[9].y()), landmarks[0].x()-landmarks[9].x()));*/
        angleBetweenLines(landmarks[0].x(),landmarks[0].y(),
                                           landmarks[9].x(),landmarks[9].y(),
                                           0,landmarks[0].y(),
                                           0));
  

  //return NormalizeRadians(rotation)
    
    //   new_angle.set_angle2(rigthHand);

    output_angles->emplace_back(new_angle);

    /*if((&landmark - &landmarks[0])==0){
        std::cout  << "X:" << std::to_string(landmark.x()) << " - " << landmark_data->x() << "\n";
      }*/
  }

  cc->Outputs()
      .Tag(kAngleDataTag)
      .Add(output_angles.release(), cc->InputTimestamp());
  return ::mediapipe::OkStatus();
}

float LandmarksToAnglesCalculator::angleBetweenLines(float x0, float y0, float x1, float y1, float x2, float y2, bool rigth_hand) {
  float angle1 = atan2((y0-y1), x0-x1);
  float angle2 = atan2((y0-y2), x0-x2);
  float result; 
  
  if(rigth_hand) result = (angle2-angle1);
  else result = (angle1-angle2);
  /*result *= 180 / 3.1415; //To degrees
  if (result<0) {
      result+=360;
  }*/
  return NormalizeRadians(result);
}  

}  // namespace mediapipe


