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

#include <memory>

#include "myMediapipe/calculators/gestures/moving_dynamic_gestures_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "myMediapipe/framework/formats/angles.pb.h"
#include "mediapipe/framework/port/ret_check.h"
#include "myMediapipe/framework/formats/mqtt_message.pb.h"


namespace mediapipe {


namespace {

struct StartingGesture{
  int start_action;
  decltype(Timestamp().Seconds()) time;
  decltype(Angle().angle1()) angle;
  NormalizedLandmark lmInfo;
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

void clear(movingActionMap &currentAction,
           StartingGesture& startingGesture) {
  currentAction.Clear();
  startingGesture = (struct StartingGesture){0};
}

decltype(Angle().angle1()) getAngle(int angleNumber, int lmId, const Angles angles){
  // TODO: replace this literal (by changing the field angle in Angle message to repeated)
  if(angleNumber==1) return angles[lmId].angle1();
  else return angles[lmId].angle2();
}

void setStartingGesture(StartingGesture& startingGesture,
                        movingActionMap currentAction,
                        decltype(Timestamp().Seconds()) startingGestureTime,
                        const Landmarks landmarks,
                        const Angles angles){
  startingGesture.start_action= currentAction.start_action();
  startingGesture.time=startingGestureTime;
  startingGesture.angle=getAngle(currentAction.angle_number(),
                                 currentAction.landmark_id(),
                                 angles);
  startingGesture.lmInfo=landmarks[currentAction.landmark_id()];

}

}  // namespace

// Moving Gestures 
//
// A fixed gesture with movement, ie a swipe
// 
// 
// 
// 
//
// Input:
//  LANDMARKS: used actions requiering hand location
//  DETECTION: the current detected static gesture.
//  ANGLES
//
// Output:
//   MQTT_MESSAGE: a message containing the topic and payload 
//                 to be sent to the mqtt dispatcher
//
// Example config:
// node {
//   calculator: "movingDynamicGesturesCalculator"
//   input_stream: "NORM_LANDMARKS:latched_transition_landmarks"
//   input_stream: "DETECTIONS:latched_moving_detection"
//   input_stream: "ANGLES:latched_moving_angles"
//   Output:
//   MQTT_MESSAGE: a message containing the topic and payload 
//                 to be sent to the mqtt dispatcher
//   node_options: {
//     [type.googleapis.com/mediapipe.movingDynamicGesturesCalculatorOptions] {
//       moving_time_out_s: 1.50
//       moving_actions_map { start_action: 6                action_type: ROTATION 
//                            landmark_id: 0                 angle_number: 1
//                            action_threshold: 0.1          time_between_actions: 0.5
//                            auto_repeat: true              max_repeat: 5
//                            topic: "handCommander/tv/ir_command"
//                            positive_payload: "KEY_VOLUMEUP"  negative_payload: "KEY_VOLUMEDOWN"}
//       moving_actions_map { start_action: 4                action_type: TRASLATION 
//                            landmark_id: 0                 angle_number: 0
//                            action_threshold: 0.1          time_between_actions: 0.5
//                            auto_repeat: false
//                            topic: "handCommander/tv/ir_command" 
//                            positive_payload: "KEY_CHANNELUP" negative_payload: "KEY_CHANNELDOWN"}                     
//     }
//   }
// }

class movingDynamicGesturesCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;
  
  private:
  ::mediapipe::movingDynamicGesturesCalculatorOptions options_;
  movingActionMap currentAction;
  StartingGesture startingGesture;
  MqttMessages mqttMessages;
  
};
REGISTER_CALCULATOR(movingDynamicGesturesCalculator);

::mediapipe::Status movingDynamicGesturesCalculator::GetContract(
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

::mediapipe::Status movingDynamicGesturesCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  
  options_ = cc->Options<::mediapipe::movingDynamicGesturesCalculatorOptions>();

  RET_CHECK_GE(options_.moving_actions_map_size(),0) 
    << "You should at least provide one action map"; 
 
  return ::mediapipe::OkStatus();
}

::mediapipe::Status movingDynamicGesturesCalculator::Process(
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
  
  if(startingGesture.start_action!=currentAction.start_action())
    clear(currentAction, startingGesture);
  
  if (!currentAction.IsInitialized()){
    for(auto act_ : options_.moving_actions_map()){
      if(act_.start_action()==label_id){
        currentAction = act_;
        setStartingGesture(startingGesture, currentAction,
                           cc->InputTimestamp().Seconds(), landmarks,
                           angles);
      } 
    }
    //no gesture found 
    if(!currentAction.IsInitialized()){
      clear(currentAction, startingGesture);
    }
  }
  else{
      // std::cout << "\n !!Gesture:" << std::to_string(label_id)
//                 << "\t :" << std::to_string(currentAction.start_action())
//                 << "\t :" << std::to_string(startingGesture.lmInfo.x())
//                 << "\t :" << std::to_string(landmarks[currentAction.landmark_id()].x())
//                  << "\t :" << std::to_string(currentAction.action_type())
//  

    //            << "\t :" << std::to_string(releaseControl)
    //           << "\t :" << std::to_string(cc->InputTimestamp().Seconds());
    // TimeOut
    if((cc->InputTimestamp().Seconds() - 
        startingGesture.time) >= options_.moving_time_out_s()){
           clear( currentAction, startingGesture);
    }

    //execute action
    if((currentAction.IsInitialized()) && 
       ((cc->InputTimestamp().Seconds() - 
        startingGesture.time) >= currentAction.time_between_actions())){
      startingGesture.time=cc->InputTimestamp().Seconds();

      int numActions = 0;
      float movementDiff = 0;
      
      switch(currentAction.action_type()){
        
        case currentAction.TRASLATION:
          movementDiff = startingGesture.lmInfo.x() - 
                         landmarks[currentAction.landmark_id()].x();
          numActions = (int)(movementDiff/currentAction.action_threshold());
          
            //  std::cout << "\n !!TRASLATION:" << std::to_string(currentAction.start_action()) 
            //     << "\t :" << std::to_string(startingGesture.lmInfo.x())
            //     << "\t :" << std::to_string(landmarks[currentAction.landmark_id()].x())
            //     << "\t :" << std::to_string(movementDiff)
            //     << "\t :" << std::to_string(numActions);
          break;  
        
        case currentAction.ROTATION:
          movementDiff = startingGesture.angle - 
                                    getAngle(currentAction.angle_number(),
                                    currentAction.landmark_id(),
                                    angles);
          numActions = (int)(movementDiff/currentAction.action_threshold());                                  
          break;
      }

      if(!currentAction.auto_repeat() && numActions ) numActions=numActions/abs(numActions);
      if(currentAction.has_max_repeat() && numActions){
        if(abs(numActions) > currentAction.max_repeat()){
          if(numActions>0) numActions=currentAction.max_repeat();
          else numActions=currentAction.max_repeat() * (-1);
        }
      } 

      for (int i = 1;  i<=abs(numActions); i++){
        // std::cout << "\n !!Action:" << std::to_string(currentAction.start_action()) 
        //   << "\t :" << std::to_string(startingGesture.angle)
        //   << "\t :" << std::to_string(startingGesture.lmInfo.x())
        //   << "\t :" << std::to_string(movementDiff)
        //   << "\t :" << std::to_string(numActions);
        Mqtt_Message message;
        message.set_topic(currentAction.topic());
        if(numActions>0) message.set_payload(currentAction.positive_payload());
        else message.set_payload(currentAction.negative_payload());
        mqttMessages.emplace_back(message);
      }
        
      if(abs(numActions)){ 
        cc->Outputs().Tag(kMqttMessageTag)
           .AddPacket(MakePacket<MqttMessages>(mqttMessages)
           .At(cc->InputTimestamp()
           .NextAllowedInStream()));
        mqttMessages.clear();   
        clear(currentAction, startingGesture);
      }
    } 
  }


  if(!currentAction.IsInitialized()) 
     cc->Outputs().Tag(kFlagTag)
      .AddPacket(MakePacket<bool>(true)
                            .At(cc->InputTimestamp()
                            .NextAllowedInStream()));



  return ::mediapipe::OkStatus();
}

}  // namespace mediapipe

