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

#include "myMediapipe/calculators/util/mqtt_publisher_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/port/ret_check.h"
#include "myMediapipe/framework/formats/mqtt_message.pb.h"
#include <iostream>
#include "myMediapipe/third_party/simple-mqtt-client/Mqtt.h"

namespace mediapipe {

namespace {

typedef std::vector<Mqtt_Message> MqttMessages;

constexpr char Kmessage[] = "MQTT_MESSAGE";

}  // namespace

// MQTT publisher 
//
// takes incoming payloads and publishes  
// them to the specified broker
//
//  MQTT_MESSAGE: A message containing the topic to publish
//                and its payload
//
// Example config:
// node {
//   calculator: "MqttPublisherCalculator"
//   input_stream: "MQTT_MESSAGE:message"
// }

class MqttPublisherCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Close(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;
  
  private:
  ::mediapipe::MqttPublisherCalculatorOptions options_;
  /**
   *  List in which all subscription topics are stored that the broker should subscribe too
  */
  std::vector<string> subscription_topic_list;
  std::vector<Mqtt> *mqtt_list;
};
REGISTER_CALCULATOR(MqttPublisherCalculator);

::mediapipe::Status MqttPublisherCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(Kmessage));

  cc->Inputs()
      .Tag(Kmessage) 
      .Set<MqttMessages>();
  //cc->Outputs().Tag(kDetectionTag).Set<Detection>();
  
  return ::mediapipe::OkStatus();
}

::mediapipe::Status MqttPublisherCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));

  options_ = cc->Options<::mediapipe::MqttPublisherCalculatorOptions>();
  return ::mediapipe::OkStatus();
}

::mediapipe::Status MqttPublisherCalculator::Process(
    CalculatorContext* cc) {
  RET_CHECK(!cc->Inputs().Tag(Kmessage).IsEmpty());

  const auto& input_messages =
      cc->Inputs().Tag(Kmessage).Get<MqttMessages>();
  
  // for (auto msg : input_messages){
  //   std::cout << "\n !!Message:" << msg.topic()
  //             << "\t :" << msg.payload();
  // }
  
  // bool exist = false;
  // for(auto mqtt_ : mqtt_list){
  //        if(mqtt_->publish_topic==input_message.topic()){
  //          exist=true;
  //        }
  // }
  static Mqtt *mqtt = new Mqtt(options_.client_id(), 
                               "", 
                               subscription_topic_list, 
                               options_.broker_ip(), 
                               options_.broker_port());
  
  for (auto msg : input_messages){
    mqtt->publish_topic = msg.topic();
    mqtt->publish(msg.payload());
  }  
  
  return ::mediapipe::OkStatus();
}

::mediapipe::Status MqttPublisherCalculator::Close(
    CalculatorContext* cc) {
  //TODO: delete mqtt;    
  return ::mediapipe::OkStatus();
}

}  // namespace mediapipe
