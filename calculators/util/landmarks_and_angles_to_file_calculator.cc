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
#include "myMediapipe/calculators/util/landmarks_and_angles_to_file_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "myMediapipe/framework/formats/angles.pb.h"
#include "mediapipe/framework/port/ret_check.h"
#include "ncurses.h"
#include <fstream>


namespace mediapipe {

namespace {

constexpr char kLandmarksTag[] = "LANDMARKS";
constexpr char kNormLandmarksTag[] = "NORM_LANDMARKS";
constexpr char kAngleDataTag[] = "ANGLES";

//using ::mediapipe::RenderAnnotation_Point;

}  // namespace

// Writes Landmarks and Angles to a CSV file with the intention to generate data_ 
// to train a model to recognize static gestures.
// If debug is enabled
// it will also open a terminal to display current data through ncurses lib   
//
// Example config:
// node {
//   calculator: "LandmarksAndAnglesToFile"
//   input_stream: "NORM_LANDMARKS:landmarks"
//   input_stream: "ANGLES:angles"
//   options {
//    [type.googleapis.com/mediapipe.LandmarksAndAnglesToFileCalculatorOptions] {
//             file_name: "file.csv" 
//             debug_to_terminal: true
//             minFPS: int ;
//     }
//   }
// }


class LandmarksAndAnglesToFileCalculator : public CalculatorBase {
 public:
  LandmarksAndAnglesToFileCalculator() {}
  ~LandmarksAndAnglesToFileCalculator() override {}
  LandmarksAndAnglesToFileCalculator(const LandmarksAndAnglesToFileCalculator&) =
      delete;
  LandmarksAndAnglesToFileCalculator& operator=(
      const LandmarksAndAnglesToFileCalculator&) = delete;

  static ::mediapipe::Status GetContract(CalculatorContract* cc);

  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Close(CalculatorContext* cc) override;

  ::mediapipe::Status Process(CalculatorContext* cc) override;

 private:
  uint16_t getFPS(double currentUS); 
  LandmarksAndAnglesToFileCalculatorOptions options_;
  uint32_t processedFrames; 
  std::ofstream outputFile;
};
REGISTER_CALCULATOR(LandmarksAndAnglesToFileCalculator);

::mediapipe::Status LandmarksAndAnglesToFileCalculator::GetContract(
    CalculatorContract* cc) {
  RET_CHECK(cc->Inputs().HasTag(kLandmarksTag) ||
            cc->Inputs().HasTag(kNormLandmarksTag))
      << "None of the input streams are provided.";
  RET_CHECK(!(cc->Inputs().HasTag(kLandmarksTag) &&
              cc->Inputs().HasTag(kNormLandmarksTag)))
      << "Can only one type of landmark can be taken. Either absolute or "
         "normalized landmarks.";
  
  RET_CHECK(cc->Inputs().HasTag(kAngleDataTag))
      << "Angle input streams are not provided, please check your graph.";

  if (cc->Inputs().HasTag(kLandmarksTag)) {
    cc->Inputs().Tag(kLandmarksTag).Set<std::vector<Landmark>>();
  }
  if (cc->Inputs().HasTag(kNormLandmarksTag)) {
    cc->Inputs().Tag(kNormLandmarksTag).Set<std::vector<NormalizedLandmark>>();
  }
  cc->Inputs().Tag(kAngleDataTag).Set<std::vector<Angle>>();

  //cc->Outputs().Tag(kAngleDataTag).Set<std::vector<Angle>>();
  return ::mediapipe::OkStatus();
}

::mediapipe::Status LandmarksAndAnglesToFileCalculator::Open(
    CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));
  options_ = cc->Options<LandmarksAndAnglesToFileCalculatorOptions>();
  
  processedFrames = 0;

  if(options_.has_file_name()){
    outputFile.open(options_.file_name());  
  }
    
  return ::mediapipe::OkStatus();
  
}
::mediapipe::Status LandmarksAndAnglesToFileCalculator::Close(
    CalculatorContext* cc) {
      endwin();
  if (outputFile.is_open()) outputFile.close();    
  return ::mediapipe::OkStatus();

}

::mediapipe::Status LandmarksAndAnglesToFileCalculator::Process(
    CalculatorContext* cc) {
  // Only process if there's input landmarks.
  if (cc->Inputs().Tag(kNormLandmarksTag).IsEmpty())
  {
    return ::mediapipe::OkStatus();
  }

  const auto &landmarks = cc->Inputs()
                              .Tag(kNormLandmarksTag)
                              .Get<std::vector<NormalizedLandmark>>();
  
  const auto &angles    = cc->Inputs()
                              .Tag(kAngleDataTag)
                              .Get<std::vector<Angle>>();                            

  //Do not process if no hand is present
  if(!landmarks[0].x()) return ::mediapipe::OkStatus(); 
  
  const auto fps = getFPS(cc->InputTimestamp().Seconds());
  if(fps < options_.minfps())  return ::mediapipe::OkStatus();

  processedFrames++;

  if(options_.debug_to_terminal()){
     initscr();			/* Start curses mode 		  */
     clear();
     std::string header = "Output File: "; 
     header.append(options_.file_name());
     header.append("\nNumber of Processed Frames:");
     header.append(std::to_string(processedFrames));
     header.append("\tFPS:");
     header.append(std::to_string(fps));
     printw(header.c_str());
  }
 
  for (const auto &landmark : landmarks)
  {
    const unsigned char lmIndex = (&landmark - &landmarks[0]);
    
    if(options_.debug_to_terminal()){
      std::string dispText = "LM:"; 
      dispText.append(std::to_string(lmIndex));
      dispText.append("\tX:");
      dispText.append(std::to_string(landmark.x()));
      dispText.append("\tY:");
      dispText.append(std::to_string(landmark.y()));
      dispText.append("\tDegrees 1:");
      dispText.append(std::to_string(angles[lmIndex].angle1()));
      dispText.append("\tDegrees 2:");
      dispText.append(std::to_string(angles[lmIndex].angle2()));


      move(lmIndex + 2,0);
	    printw(dispText.c_str());	
	    
    }

    if (outputFile.is_open()){
      const char* delimiter = ",";
      std::string outText ="";
      outText.append(std::to_string(lmIndex));
      outText.append(delimiter);
      outText.append(std::to_string(landmark.x()));
      outText.append(delimiter);
      outText.append(std::to_string(landmark.y()));
      outText.append(delimiter);
      outText.append(std::to_string(angles[lmIndex].angle1()));
      outText.append(delimiter);
      outText.append(std::to_string(angles[lmIndex].angle2()));
      outText.append("\n");
      outputFile << outText;

    }


    /*if((&landmark - &landmarks[0])==0){
        std::cout  << "X:" << std::to_string(landmark.x()) << " - " << landmark_data->x() << "\n";
      }*/
  }
   if(options_.debug_to_terminal())refresh();			/* Print it on to the real screen */

  // cc->Outputs()
  //     .Tag(kAngleDataTag)
  //     .Add(output_angles.release(), cc->InputTimestamp());
  return ::mediapipe::OkStatus();
}

uint16_t LandmarksAndAnglesToFileCalculator::getFPS(double currentUS){
  static double oldUS=0;
  double deltaUS = currentUS-oldUS;
  oldUS=currentUS;
  
  if(deltaUS>0)return uint16_t(1/deltaUS);
  else return 0;
  


} 

}  // namespace mediapipe


