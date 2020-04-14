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

#include "myMediapipe/calculators/gestures/fixed_dynamic_gestures_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "myMediapipe/framework/formats/angles.pb.h"
#include "mediapipe/framework/port/ret_check.h"
#include "myMediapipe/framework/formats/mqtt_message.pb.h"
#include <string>

namespace mediapipe {



namespace {

struct LastGesture{
  int start_action;
  decltype(Timestamp().Seconds()) time;
  bool notEmpty;
};

typedef std::vector<Detection> Detections;
typedef std::vector<Angle> Angles;
typedef std::vector<NormalizedLandmark> Landmarks;
typedef std::vector<Mqtt_Message> MqttMessages;



constexpr char kDetectionTag[] = "DETECTIONS";
constexpr char kNormLandmarksTag[] = "NORM_LANDMARKS";
constexpr char kAnglesTag[] = "ANGLES";
constexpr char kFlagTag[] = "FLAG";
constexpr char kMqttMessageTag[] = "MQTT_MESSAGE";
 
void clear(fixedActionMap &currentAction,
           LastGesture& lastGesture) {
  
  currentAction.Clear();
  lastGesture = (struct LastGesture){0};
}

void setLastGesture(LastGesture& lastGesture,
                    fixedActionMap currentAction,
                    decltype(Timestamp().Seconds()) lastGestureTime){
  
  lastGesture.start_action= currentAction.start_action();
  lastGesture.time=lastGestureTime;
  lastGesture.notEmpty= true;
}

decltype(Angle().angle1()) getAngle(int angleNumber, int lmId, const Angles angles){
  // TODO: replace this literal (by changing the field angle in Angle message to repeated)
  if(angleNumber==1) return angles[lmId].angle1();
  else return angles[lmId].angle2();
}

}  // namespace

// Fixed Gestures 
//
// fixed gesture used in momentary actions,
//          ie mute while the gesture is present 

// Input:
//  LANDMARKS: used actions requiering hand location
//  DETECTION: the current detected static gesture.
//  ANGLES;
//
// Output:
//   MQTT_MESSAGE: a message containing the topic and payload 
//                 to be sent to the mqtt dispatcher
//
// Example config:
// node {
//   calculator: "fixedDynamicGesturesCalculator"
//   input_stream: "NORM_LANDMARKS:gated_fixed_landmarks"
//   input_stream: "DETECTIONS:gated_fixed_detection"
//   output_stream: MQTT_MESSAGE:message
//     node_options: {
//   node_options: {
//     [type.googleapis.com/mediapipe.fixedDynamicGesturesCalculatorOptions] {
//      fixed_time_out_s: 1.50
//      fixed_actions_map { start_action: 1                
//                           time_between_actions: 3.0
//                           auto_repeat: false             
//                           mqtt_message{ topic: "handCommander/tv/ir_command" payload: "KEY_MUTE"}
//                         }
//      fixed_actions_map { start_action: 3                 
//                           landmark_id: 0                 angle_number: 1                
//                           angle_limits{angle_limit_pos: 1.8           
//                                        angle_limit_neg: 1.2}
//                           angle_limits{angle_limit_pos: -0.8           
//                                        angle_limit_neg: -1.4} 
//                           angle_limits{angle_limit_pos: 0.85           
//                                        angle_limit_neg: 0.35}
//                           angle_limits{angle_limit_pos: 2.8           
//                                        angle_limit_neg: 2.4}            
//                           time_between_actions: 1.5      auto_repeat: true
//                           mqtt_message{ topic: "handCommander/tv/ir_command" payload: "KEY_VOLUMEUP"} 
//                           mqtt_message{ topic: "handCommander/tv/ir_command" payload: "KEY_VOLUMEDOWN"}
//                           mqtt_message{ topic: "handCommander/tv/ir_command" payload: "KEY_CHANNELUP"}          
//                           mqtt_message{ topic: "handCommander/tv/ir_command" payload: "KEY_CHANNELDOWN"}                          }
//     }
//   }
// }

class fixedDynamicGesturesCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;
  
  private:
  ::mediapipe::Status executeAction(fixedActionMap& currentAction,
                                    LastGesture& lastGesture,
                                    decltype(Timestamp().Seconds()) GestureTime,
                                    const Angles angles,
                                    CalculatorContext* cc);
  
  ::mediapipe::fixedDynamicGesturesCalculatorOptions options_;
  fixedActionMap currentAction;
  LastGesture lastGesture;
  MqttMessages mqttMessages;
  
};
REGISTER_CALCULATOR(fixedDynamicGesturesCalculator);

::mediapipe::Status fixedDynamicGesturesCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kNormLandmarksTag))
      << "Normalized Landmark input stream is NOT provided.";
  RET_CHECK(cc->Inputs().HasTag(kDetectionTag))
      << "Detections input stream is NOT provided.";
  RET_CHECK(cc->Inputs().HasTag(kAnglesTag))
      << "Angles input stream is NOT provided.";

  if (cc->Inputs().HasTag(kNormLandmarksTag)) {
    cc->Inputs().Tag(kNormLandmarksTag).Set<std::vector<NormalizedLandmark>>();
  }

  if (cc->Inputs().HasTag(kDetectionTag))
    cc->Inputs().Tag(kDetectionTag).Set<Detections>();

  if (cc->Inputs().HasTag(kAnglesTag))
    cc->Inputs().Tag(kAnglesTag).Set<Angles>(); 
  
  cc->Outputs().Tag(kFlagTag).Set<bool>();
  cc->Outputs().Tag(kMqttMessageTag).Set<MqttMessages>();
  // cc->Outputs().Index(0).Set<Detections>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status fixedDynamicGesturesCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  options_ = cc->Options<::mediapipe::fixedDynamicGesturesCalculatorOptions>();
  RET_CHECK_GE(options_.fixed_actions_map_size(),0) 
    << "You should at least provide one action map";
  return ::mediapipe::OkStatus();
}

::mediapipe::Status fixedDynamicGesturesCalculator::Process(
    CalculatorContext* cc) {
      
  RET_CHECK(!cc->Inputs().Tag(kDetectionTag).IsEmpty());
  const auto& input_detections =
        cc->Inputs().Tag(kDetectionTag).Get<Detections>();
  const auto& input_detection = input_detections.back();
  const int32 label_id = input_detection.label_id().Get(0);
  
  RET_CHECK(!cc->Inputs().Tag(kNormLandmarksTag).IsEmpty());
  const auto &landmarks = cc->Inputs()
                              .Tag(kNormLandmarksTag)
                              .Get<std::vector<NormalizedLandmark>>();
  RET_CHECK(!cc->Inputs().Tag(kAnglesTag).IsEmpty());
  const auto &angles = cc->Inputs()
                              .Tag(kAnglesTag)
                              .Get<std::vector<Angle>>();
  
  // std::cout << "\n\t\t dbg:" << std::to_string(lastGesture.notEmpty)
  //           << "\t :" << std::to_string(lastGesture.start_action)
  //           << "\t :" << std::to_string(lastGesture.start_action)

  if(lastGesture.notEmpty &&
     (lastGesture.start_action!=label_id))
    clear(currentAction, lastGesture);
  
  if (!currentAction.IsInitialized()){
    for(auto act_ : options_.fixed_actions_map()){
      if(act_.start_action()==label_id){
        currentAction = act_;
        if(currentAction.has_landmark_id()){ 
          RET_CHECK(currentAction.has_angle_number())
            << "angle_number not provided";
           RET_CHECK_EQ(currentAction.angle_limits().size(),currentAction.mqtt_message().size())
             << "Command should have the same number of entries as angle_limits";
        }
      } 
    }
    //no gesture found 
    if(!currentAction.IsInitialized()){
      clear(currentAction, lastGesture);
    }
  }
  if (currentAction.IsInitialized()){

    //  std::cout << "\n !!Gesture:" << std::to_string(label_id)
    //              << "\t :" << std::to_string(currentAction.start_action())
    //              << "\t :" << std::to_string(currentAction.time_between_actions())
    //              << "\t :" << std::to_string(currentAction.auto_repeat())
    //              << "\t angle :" << std::to_string(angles[0].angle1())
                 
    //              << "\t :" << std::to_string(cc->InputTimestamp().Seconds())
    //              << "\t :" << std::to_string(lastGesture.notEmpty)
    //              << "\t :" << std::to_string(lastGesture.time)
    //              ;
    
    //first execution
    if(!lastGesture.notEmpty){
      executeAction(currentAction,lastGesture,
                    cc->InputTimestamp().Seconds(),
                    angles,
                    cc);                            
    }
    
    else{
      if(currentAction.auto_repeat() && 
         ((cc->InputTimestamp().Seconds() - 
          lastGesture.time) >= currentAction.time_between_actions()))
          executeAction(currentAction,lastGesture,
                        cc->InputTimestamp().Seconds(),
                        angles,
                        cc);
    }
    
    
    // TimeOut
    if((cc->InputTimestamp().Seconds() - 
        lastGesture.time) >= options_.fixed_time_out_s()){
           clear( currentAction, lastGesture);
    }

  }
  if(!currentAction.IsInitialized()) 
     cc->Outputs().Tag(kFlagTag)
      .AddPacket(MakePacket<bool>(true)
                            .At(cc->InputTimestamp()
                            .NextAllowedInStream()));

  return ::mediapipe::OkStatus();
}

::mediapipe::Status
   fixedDynamicGesturesCalculator::executeAction(fixedActionMap& currentAction,
                   LastGesture& lastGesture,
                   decltype(Timestamp().Seconds()) GestureTime,
                   const Angles angles,
                   CalculatorContext* cc){
  
  Mqtt_Message currCommand;
  currCommand.set_topic("empty");

  if(currentAction.has_landmark_id()){ 
    
    const auto currAngle = getAngle(currentAction.angle_number(),
                                    currentAction.landmark_id(),
                                    angles);
    
    for(int i=0;i<currentAction.angle_limits().size();i++){
      if((currAngle<=currentAction.angle_limits(i).angle_limit_pos()) &&
        (currAngle>=currentAction.angle_limits(i).angle_limit_neg())){
      
        currCommand.set_topic(currentAction.mqtt_message(i).topic());
        currCommand.set_payload(currentAction.mqtt_message(i).payload());
      } 
    }
  }
  else{
    currCommand.set_topic(currentAction.mqtt_message(0).topic());
    currCommand.set_payload(currentAction.mqtt_message(0).payload());
  }

  if(currCommand.topic().compare("empty") != 0){
    setLastGesture(lastGesture, currentAction, GestureTime);
    //  std::cout << "\n !!Action:" << std::to_string(currentAction.start_action())
    //        << "\t :" << currCommand.payload(); 
     mqttMessages.emplace_back(currCommand);
     cc->Outputs().Tag(kMqttMessageTag)
         .AddPacket(MakePacket<MqttMessages>(mqttMessages)
         .At(cc->InputTimestamp()
         .NextAllowedInStream()));        
  }
  currentAction.Clear();
  mqttMessages.clear();
  return ::mediapipe::OkStatus();
}



}  // namespace mediapipe
