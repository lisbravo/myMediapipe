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

//#include <memory>

#include "myMediapipe/calculators/gestures/gesture_classifier_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/util/resource_util.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/file_helpers.h"

namespace mediapipe {



namespace {

typedef std::vector<Detection> Detections;

constexpr char kDetectionTag[] = "DETECTIONS"; 
constexpr char kLatchTransitionTag[] = "LATCH_TRANSITION";
constexpr char kLatchMovingTag[] = "LATCH_MOVING";
constexpr char kLatchWritingTag[] = "LATCH_WRITING";
constexpr char kLatchFixedTag[] = "LATCH_FIXED"; 

// Used when an incoming gesture is not yet mapped to a
// function, to send a FINISHED signal to the Flow Limiter
constexpr char kTBDTag[] = "TBD";

constexpr char transitionTag[] = "transition";
constexpr char movingTag[] = "moving";
constexpr char writingTag[] = "writing";
constexpr char fixedTag[] = "fixed";

constexpr unsigned int str2int(const char* str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

void setLatches(const bool transition,
                const bool moving,
                const bool writing,
                const bool fixed,
                CalculatorContext* cc){

  cc->Outputs().Tag(kLatchTransitionTag).AddPacket(
          MakePacket<bool>(transition).At(cc->InputTimestamp()));
  cc->Outputs().Tag(kLatchMovingTag).AddPacket(
          MakePacket<bool>(moving).At(cc->InputTimestamp()));
  cc->Outputs().Tag(kLatchWritingTag).AddPacket(
          MakePacket<bool>(writing).At(cc->InputTimestamp()));
  cc->Outputs().Tag(kLatchFixedTag).AddPacket(
          MakePacket<bool>(fixed).At(cc->InputTimestamp()));
}

}  // namespace

// Gestures Classifiers (and dealer) 
//
// It takes the incoming gestures in the form of DETECTION, 
// classifies them according to the classes in the file 
// gestures_types_file_name and triggers the corresponding calculator 
//for further proccesing
// Then, it will remain disabled until a cleared flag is received 
// which can happen when a gesture is proccesed or a timeout event
//
// Input:
//  DETECTION: A Detection proto containing the detected gesture.
//
// Output:
//   LATCH: transition_Gesture  
//      Class transition   
//        - Gestures that begin with one gesture 
//          and ends with another
//
//   LATCH: moving_Gesture      
//      Class moving 
//        - A fixed gesture with movement, ie a swipe  
//
//   LATCH: writing_Gesture     
//      Class writing 
//        - Also a fixed gesture but used 
//          to draw a number or symbol
//
//   LATCH: fixed_Gesture     
//      Class fixed
//        - fixed gesture used in momentary actions,
//          ie mute while the gesture is present 
//
//   LATCH: TBD
//      Used when an incoming gesture is not yet mapped to a
//      function, to send a FINISHED signal to the Flow Limiter
//
// Example config:
// node {
//   calculator: "gestureClassifierCalculator"
//   input_stream: "DETECTIONS:detections"
//   output_stream: "LATCH_TRANSITION:transition_gesture_flag"
//   output_stream: "LATCH_MOVING:moving_Gesture_flag"
//   output_stream: "LATCH_WRITING:writing_Gesture_flag"
//   output_stream: "LATCH_FIXED:fixed_Gesture_flag"
//   node_options: {
//     [type.googleapis.com/mediapipe.gestureClassifierCalculatorOptions] {
//       gestures_classes_map_path: "gestures_classes_map.txt"
//     }
// }

class gestureClassifierCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;
  

  ::mediapipe::Status Process(CalculatorContext* cc) override;
 
  private:
    std::unordered_map<int, std::string> gesture_map_;
    ::mediapipe::gestureClassifierCalculatorOptions options_;
    bool disabled;
};
REGISTER_CALCULATOR(gestureClassifierCalculator);

::mediapipe::Status gestureClassifierCalculator::GetContract(
    CalculatorContract* cc) {
  
  RET_CHECK(cc->Inputs().HasTag(kDetectionTag));
  if (cc->Inputs().HasTag(kDetectionTag))
    cc->Inputs().Tag(kDetectionTag).Set<Detections>();

  RET_CHECK(!cc->Outputs().GetTags().empty());

  if (cc->Outputs().HasTag(kLatchTransitionTag)) 
      cc->Outputs().Tag(kLatchTransitionTag).Set<bool>();

  if (cc->Outputs().HasTag(kLatchMovingTag)) 
      cc->Outputs().Tag(kLatchMovingTag).Set<bool>();

  if (cc->Outputs().HasTag(kLatchWritingTag)) 
      cc->Outputs().Tag(kLatchWritingTag).Set<bool>();
  
  if (cc->Outputs().HasTag(kLatchFixedTag)) 
      cc->Outputs().Tag(kLatchFixedTag).Set<bool>();

  RET_CHECK(cc->Outputs().HasTag(kTBDTag));
  if (cc->Outputs().HasTag(kTBDTag)) 
      cc->Outputs().Tag(kTBDTag).Set<bool>(); 

  return ::mediapipe::OkStatus();
}

::mediapipe::Status gestureClassifierCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  
  options_ = cc->Options<::mediapipe::gestureClassifierCalculatorOptions>();
  std::string string_path;
  ASSIGN_OR_RETURN(string_path, PathToResourceAsFile(options_.gestures_types_file_name()));
  std::string gesture_map_string;
  MP_RETURN_IF_ERROR(file::GetContents(string_path, &gesture_map_string));

  std::istringstream stream(gesture_map_string);
  std::string line;
  int i = 0;
  while (std::getline(stream, line)) {
    gesture_map_[i++] = line;
  }
  disabled=false;

  // std::cout << "\n gestureClassifierCalculator::Open";
  return ::mediapipe::OkStatus();
}

::mediapipe::Status gestureClassifierCalculator::Process(
    CalculatorContext* cc) {

   //RET_CHECK(!cc->Inputs().Tag(kDetectionTag).IsEmpty());
   if(!cc->Inputs().Tag(kDetectionTag).IsEmpty()){
    const auto& input_detections =
          cc->Inputs().Tag(kDetectionTag).Get<Detections>();
    const auto& input_detection = input_detections.back();
    const char* outLabel = ""; 
    
    const int32 label_id = input_detection.label_id().Get(0);
    //std::cout << "\n inLabel:" << std::to_string(label_id);
    if (gesture_map_.find(label_id) != gesture_map_.end()) 
      outLabel = gesture_map_[label_id].c_str();

    //std::cout << "\t outLabel:" << outLabel;

    switch (str2int(outLabel)){
      case str2int(transitionTag):
        std::cout << "\n-----transition:";
        setLatches(true, false, false, false, cc);
        break;
      case str2int(movingTag):
        std::cout << "\n-----moving:";
        setLatches(false, true, false, false, cc);
        break;
      case str2int(writingTag):
        std::cout << "\n-----writing:";
        setLatches(false, false, true, false, cc);
        break;
      case str2int(fixedTag):
        std::cout << "\n-----fixed:";
        setLatches(false, false, false, true, cc);
        break;
      default:
        //blocks processing nodes and reenables self input through 
        // flow limiter
        std::cout << "\n-----default:"  << outLabel;
        setLatches(false, false, false, false, cc);;
        cc->Outputs().Tag(kTBDTag).AddPacket(
          MakePacket<bool>(true)
            .At(cc->InputTimestamp().NextAllowedInStream()));
        break;
    }
  }
  return ::mediapipe::OkStatus();
}

}  // namespace mediapipe
