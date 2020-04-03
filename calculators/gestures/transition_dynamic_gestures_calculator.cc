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

#include "myMediapipe/calculators/gestures/transition_dynamic_gestures_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "myMediapipe/framework/formats/mqtt_message.pb.h"
#include "mediapipe/framework/port/ret_check.h"


namespace mediapipe {

namespace {

struct Mqtt{
  std::string topic;
  std::string payload;
};

struct _actions{
     int startAction;
     int endAction;
     Mqtt mqtt;
     bool notEmpty;
 };

typedef std::vector<Detection> Detections;
typedef std::vector<Mqtt_Message> MqttMessages;

constexpr char kDetectionTag[] = "DETECTIONS";
constexpr char kNormLandmarksTag[] = "NORM_LANDMARKS";
constexpr char kFlagTag[] = "FLAG";
constexpr char kPresenceTag[] = "PRESENCE";
constexpr char kMqttMessageTag[] = "MQTT_MESSAGE";



void clear(_actions &currentAction,
           decltype(Timestamp().Seconds()) &startingGestureTime) {
  currentAction=(struct _actions){0};
  startingGestureTime = 0;
}
}  // namespace

// Transition Gestures 
//
// Gestures that begin with one gesture 
//                and ends with another
// 
//
// Input:
//  LANDMARKS: used actions requiering hand location
//  DETECTION: the current detected static gesture.
//
// Output:
//   MQTT_MESSAGE: a message containing the topic and payload 
//                 to be sent to the mqtt dispatcher
//
// Example config:
// node {
//   calculator: "transitionDynamicGesturesCalculator"
//   input_stream: "NORM_LANDMARKS:gated_transition_landmarks"
//   input_stream: "DETECTIONS:gated_transition_detection"
//   output_stream: MQTT_MESSAGE:message
//     node_options: {
//   [type.googleapis.com/mediapipe.transitionDynamicGesturesCalculatorOptions] {
//     time_out_s: 1.50
//     actions_map { start_action: 0 end_action: 2 
//       mqtt_message{ topic: "handCommander/tv/ir_command" payload: "KEY_POWER"}
//     }
//     actions_map { start_action: 2 end_action: 0 
//       mqtt_message{ topic: "handCommander/tv/ir_command" payload: "KEY_POWER"}
//     }
//   }
// }


class transitionDynamicGesturesCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;
  
  private:
  ::mediapipe::transitionDynamicGesturesCalculatorOptions options_;
  _actions currentAction;
  decltype(Timestamp().Seconds()) startingGestureTime;
  std::vector<_actions> actionsMap;
  MqttMessages mqttMessages;
};
REGISTER_CALCULATOR(transitionDynamicGesturesCalculator);

::mediapipe::Status transitionDynamicGesturesCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kNormLandmarksTag))
      << "Normalized Landmark input stream is NOT provided.";
  RET_CHECK(cc->Inputs().HasTag(kDetectionTag))
      << "Detections input stream is NOT provided.";
  RET_CHECK(cc->Inputs().HasTag(kDetectionTag))
      << "Hand Presence input stream is NOT provided.";

  if (cc->Inputs().HasTag(kNormLandmarksTag)) {
    cc->Inputs().Tag(kNormLandmarksTag).Set<std::vector<NormalizedLandmark>>();
  }

  if (cc->Inputs().HasTag(kDetectionTag))
    cc->Inputs().Tag(kDetectionTag).Set<Detections>();

  
  if (cc->Inputs().HasTag(kPresenceTag))
    cc->Inputs().Tag(kPresenceTag).Set<bool>();  
  
  cc->Outputs().Tag(kFlagTag).Set<bool>();
  cc->Outputs().Tag(kMqttMessageTag).Set<MqttMessages>();
  // cc->Outputs().Index(0).Set<Detections>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status transitionDynamicGesturesCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  options_ = cc->Options<::mediapipe::transitionDynamicGesturesCalculatorOptions>();

  RET_CHECK_GE(options_.actions_map_size(),0) 
    << "You should at least provide one action map"; 

  for(int i = 0; i<options_.actions_map_size(); i++){
    _actions loadAct;
    loadAct.startAction = options_.actions_map(i).start_action();
    loadAct.endAction = options_.actions_map(i).end_action();
    loadAct.mqtt.topic = options_.actions_map(i).mqtt_message().topic();
    loadAct.mqtt.payload = options_.actions_map(i).mqtt_message().payload();
    loadAct.notEmpty=true;
    actionsMap.emplace_back(loadAct);
  }
    return ::mediapipe::OkStatus();
}

::mediapipe::Status transitionDynamicGesturesCalculator::Process(
    CalculatorContext* cc) {
   
   
  if(cc->Inputs().Tag(kDetectionTag).IsEmpty())
    return ::mediapipe::OkStatus();

  const auto& input_detections =
      cc->Inputs().Tag(kDetectionTag).Get<Detections>();
  const auto& input_detection = input_detections.back();
  const int32 label_id = input_detection.label_id().Get(0);
  
  
  if (!currentAction.notEmpty){
    //
    //currentAction=NULL;
    for(auto act_ : actionsMap){
      if(act_.startAction==label_id){
        currentAction = act_;
        startingGestureTime=cc->InputTimestamp().Seconds();
      } 
    }
    //no gesture found 
    if(!currentAction.notEmpty){
      clear(currentAction, startingGestureTime);
    }
      
  }
  else{
        //timeOut
    if((cc->InputTimestamp().Seconds() - 
        startingGestureTime) >= options_.time_out_s()){
          clear( currentAction, startingGestureTime);
    }
    //execute action
    if((currentAction.notEmpty) && 
       (label_id==currentAction.endAction)){
      // std::cout << "\n !!Action:" << std::to_string(currentAction.startAction) 
      //           << "\t :" << std::to_string(currentAction.endAction)
      //           << "\t :" << currentAction.mqtt.topic
      //           << "\t :" << currentAction.mqtt.payload;
      Mqtt_Message message;
      message.set_topic(currentAction.mqtt.topic);
      message.set_payload(currentAction.mqtt.payload);
      mqttMessages.emplace_back(message);

      cc->Outputs().Tag(kMqttMessageTag)
           .AddPacket(MakePacket<MqttMessages>(mqttMessages)
           .At(cc->InputTimestamp()
           .NextAllowedInStream()));
      
      clear(currentAction, startingGestureTime); 
      mqttMessages.clear();         
    } 
  }
  
  if(!currentAction.notEmpty) 
     cc->Outputs().Tag(kFlagTag)
      .AddPacket(MakePacket<bool>(true)
                            .At(cc->InputTimestamp()
                            .NextAllowedInStream()));


  return ::mediapipe::OkStatus();
}

}  // namespace mediapipe
