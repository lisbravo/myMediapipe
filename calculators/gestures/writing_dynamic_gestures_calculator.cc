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

#include "myMediapipe/calculators/gestures/writing_dynamic_gestures_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/ret_check.h"
#include "myMediapipe/framework/formats/mqtt_message.pb.h"
#include <math.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>  

namespace mediapipe {


namespace {

struct ROI
{
  float left;
  float rigth;
  float top;
  float bottom;
  int x_length;
  int y_length;
  float ratio;
};

typedef std::vector<Detection> Detections;
typedef std::vector<NormalizedLandmark> Landmarks;
typedef std::vector<Mqtt_Message> MqttMessages;

constexpr char kDetectionTag[] = "DETECTIONS";
constexpr char kNormLandmarksTag[] = "NORM_LANDMARKS";
constexpr char kFlagTag[] = "FLAG";
constexpr char kMqttMessageTag[] = "MQTT_MESSAGE";

int GetAngle(NormalizedLandmark point_start, 
            NormalizedLandmark point_middle, 
            NormalizedLandmark point_end) {
  const auto ang1 = atan2(point_end.y() - point_middle.y(), point_end.x() - point_middle.x());
  const auto ang2 = atan2(point_start.y() - point_middle.y(), point_start.x() - point_middle.x());
  const auto angle = abs((ang1 - ang2) * (180 / M_PI));
  int return_angle = (int)angle;
  return return_angle;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

}  // namespace

// Writing Gestures 
//
// A fixed gesture  used 
//          to draw a number or symbol
// 
// 
// 
// 
//
// Input:
//  LANDMARKS: used actions requiering hand location
//  DETECTION: the current detected static gesture.
//
// Output:
//   TBD
//
// Example config:
// node {
//   calculator: "writingDynamicGesturesCalculator"
//   input_stream: "NORM_LANDMARKS:gated_writing_landmarks"
//   input_stream: "DETECTIONS:gated_writing_detection"
//   output_stream: TBD
//   node_options: {
//     [type.googleapis.com/mediapipe.writingDynamicGesturesCalculatorOptions] {
//       time_out_ms: 2500
//       landmark_id: 8
//       window_for_angle_detection: 15
//       angle_min_limit: 140
//       angle_max_limit: 220
//       accute_angle_trigger: 3  // number of deteccted angles in the windows to trigger an "accute angle detected" routine
//       ratio_trigger: 1.4
//       time_to_inference: 3 //This is the time to wait between a start condition (accute angle detected) and inference 
//       watchdog_time: 4.0     //To avoid blocking in the case that current drawing is too noisy  or gable
//       prediction_threshold: 0.7
//     }
//   }
// }

class writingDynamicGesturesCalculator : public CalculatorBase {
 public:
  static ::mediapipe::Status GetContract(CalculatorContract* cc);
  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;
  
  private:
  void ProcessROI(int y_length,int x_length,int y_start,int x_start);
  ::mediapipe::writingDynamicGesturesCalculatorOptions options_;
  std::vector<NormalizedLandmark> point_list;
  decltype(Timestamp().Seconds()) init_drawing_time; //used by the watchdog
  decltype(Timestamp().Seconds()) digit_start_time;  //tracks the beginning of a digit drawing 
  ROI roi; //region of interest
  int current_angle;
  int number_of_accute_angles;
  bool accute_angle_cleared;
  float old_x;
  float old_y;
  bool minimun_ratio_trigered;
};

REGISTER_CALCULATOR(writingDynamicGesturesCalculator);

::mediapipe::Status writingDynamicGesturesCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kNormLandmarksTag))
      << "Normalized Landmark input stream is NOT provided.";
  RET_CHECK(cc->Inputs().HasTag(kDetectionTag))
      << "Detections input stream is NOT provided.";

  if (cc->Inputs().HasTag(kNormLandmarksTag)) {
    cc->Inputs().Tag(kNormLandmarksTag).Set<std::vector<NormalizedLandmark>>();
  }

  if (cc->Inputs().HasTag(kDetectionTag))
    cc->Inputs().Tag(kDetectionTag).Set<Detections>();
  
  cc->Outputs().Tag(kFlagTag).Set<bool>();
  cc->Outputs().Tag(kMqttMessageTag).Set<MqttMessages>();

  // cc->Outputs().Index(0).Set<Detections>();

  return ::mediapipe::OkStatus();
}

::mediapipe::Status writingDynamicGesturesCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  
  options_ = cc->Options<::mediapipe::writingDynamicGesturesCalculatorOptions>();
  return ::mediapipe::OkStatus();
}

::mediapipe::Status writingDynamicGesturesCalculator::Process(
    CalculatorContext* cc) {

      /* 
  // RET_CHECK(!cc->Inputs().Tag(kDetectionTag).IsEmpty());
  //  const auto& input_detections =
  //       cc->Inputs().Tag(kDetectionTag).Get<Detections>();
  //  const auto& input_detection = input_detections.back();

  // const int32 label_id = input_detection.label_id().Get(0);

  RET_CHECK(!cc->Inputs().Tag(kNormLandmarksTag).IsEmpty());
  const auto &landmarks = cc->Inputs()
                              .Tag(kNormLandmarksTag)
                              .Get<std::vector<NormalizedLandmark>>();

  const auto current_landmark = landmarks[options_.landmark_id()];
  point_list.push_back(current_landmark);
  
  if(point_list.size()==1){
    init_drawing_time =
    roi.left = current_landmark.x();
    roi.rigth = current_landmark.x();
    roi.top = current_landmark.y();
    roi.bottom = current_landmark.y(); 
  }

  const auto last_point_index = point_list.size() - 1;
  
  if (point_list.size() > options_.window_for_angle_detection()){
    const int mid_point = last_point_index - (int)(options_.window_for_angle_detection() / 2);
    const int start_point = last_point_index - options_.window_for_angle_detection();
    current_angle = GetAngle(point_list[start_point],
                             point_list[mid_point],
                             point_list[last_point_index]);
  }

  //first line with accute angle removal
  if ((current_angle > 0) && 
      (current_angle >= options_.angle_max_limit() ||
       current_angle <= options_.angle_min_limit())){
    
    number_of_accute_angles++;

    if (number_of_accute_angles >= options_.accute_angle_trigger() &&
        (!accute_angle_cleared) ){
      //clear path (point_list.size());
      //eliminates the first line after an accute angle is detected
      point_list.clear();
      roi.left    = old_x;
      roi.rigth   = old_x;
      roi.top     = old_y;
      roi.bottom  = old_y;
      number_of_accute_angles=0;
      accute_angle_cleared=true;
    }
  } 

  // setting the Region Of Interest     
  if (roi.left   > current_landmark.x()) roi.left   = current_landmark.x();
  if (roi.rigth  < current_landmark.x()) roi.rigth  = current_landmark.x();
  if (roi.top    > current_landmark.y()) roi.top    = current_landmark.y();
  if (roi.bottom < current_landmark.y()) roi.bottom = current_landmark.y();

  // ratio is used to trigger the inference system, basically its a way to emulate a "pen up" event
  roi.x_length = roi.rigth - roi.left;
  roi.y_length = roi.bottom - roi.top;
  roi.ratio =  roi.y_length / roi.x_length;

  //at this point we have a valid input
  if (((roi.ratio >= options_.ratio_trigger()) || minimun_ratio_trigered ) 
                && (accute_angle_cleared)){
    minimun_ratio_trigered=true;

    ProcessROI(static_cast<int>(map(roi.y_length,0,1,0,65535)),
               static_cast<int>(map(roi.x_length,0,1,0,65535)),
               static_cast<int>(map(roi.top,0,1,0,65535)),
               static_cast<int>(map(roi.left,0,1,0,65535)));
   
    // // start inference timer and cancel the watchdog
    //   if (self.inference_timer is None) or \
    //   (not self.inference_timer.is_alive()):
    //     self.inference_timer = Timer(self.TIME_TO_INFERENCE, self.infer)
    //     self.inference_timer.start()
    //     self.watchdog_timer.cancel()
  }


  old_x = current_landmark.x();
  old_y = current_landmark.y(); */

  cc->Outputs().Tag(kFlagTag)
      .AddPacket(MakePacket<bool>(true)
                            .At(cc->InputTimestamp()
                            .NextAllowedInStream()));
  

  return ::mediapipe::OkStatus();
}

// this will process the valid Region Of Interest 
void writingDynamicGesturesCalculator::ProcessROI(int y_length,int x_length,int y_start,int x_start){ 
  //print(y_length,x_length,y_start,x_start)
  uint8_t kMargin = 100;
  uint8_t kHorizontalOffset = x_start  - static_cast<int>(kMargin/2);
  uint8_t kVerticalOffset  = y_start  - static_cast<int>(kMargin/2);
  uint8_t kThreshold = 64;
  struct Coord{
    int x;
    int y;
  };
        
  // first it will create a new image with the ROI drawing, plus some margin for centering
  //uint8_t roi_image[(y_length + kMargin)][(x_length + kMargin)];
  cv::Mat roi_image = cv::Mat::zeros((y_length + kMargin),(x_length + kMargin),CV_8UC1);      
  for(auto i=0; i<point_list.size()-2;i++){
    cv::Point line_init;
    cv::Point line_end;
    
    line_init.x = static_cast<int>(map(point_list[i].x(),0,1,0,65535)) - kHorizontalOffset;
    line_init.y = static_cast<int>(map(point_list[i].y(),0,1,0,65535)) - kVerticalOffset;
    line_end.x = static_cast<int>(map(point_list[i + 1].x(),0,1,0,65535)) - kHorizontalOffset;
    line_end.y = static_cast<int>(map(point_list[i].x(),0,1,0,65535)) - kVerticalOffset;
    cv::line(roi_image, line_init, line_end, 255, 8);
  }
//   // then it resizes the imagee to the 28*28 format of the neural model
  cv::Mat scaled_image = cv::Mat::zeros(28,28,CV_8UC1);
  cv::resize(roi_image, scaled_image, scaled_image.size(), CV_INTER_AREA);
//   // applies a simple Threshold operation, improves inference
//   self.scaled_image[self.scaled_image > THRESHOLD] = 255
}

}  // namespace mediapipe
