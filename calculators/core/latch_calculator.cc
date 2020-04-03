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
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/util/header_util.h"

namespace mediapipe {
  
namespace {
enum LatchState {
  LATCH_UNINITIALIZED,
  LATCH_ON,
  LATCH_OFF,
};

constexpr char kLatchTag[] = "LATCH"; 
constexpr char kResetTag[] = "RESET";
constexpr char kStateTag[] = "STATE_CHANGE";

std::string ToString(LatchState state) {
  switch (state) {
    case LATCH_UNINITIALIZED:
      return "UNINITIALIZED";
    case LATCH_ON:
      return "LATCH_ON";
    case LATCH_OFF:
      return "LATCH_OFF";
  }
  DLOG(FATAL) << "Unknown LatchState";
  return "UNKNOWN";
}
}  // namespace

// Controls whether or not the input packets are passed further along the graph.
// This is derived from Gate Calculator but instead of enabling packets to pass
// when an "allow" control packet is received, this one will start letting all  
// packets to pass through when a boolean "LATCH" packet is received with a true
// in its state, and will remain doing so until a false is received
// Optional RESET input, used to disable flowing when a true is received
//
// Takes multiple data input streams ,
// as well as an optional STATE_CHANGE stream which downstream
// calculators can use to respond to state-change events.
//
//
// Example config:
// node {
//   calculator: "LatchCalculator"
//   input_stream: "input_stream0"
//   input_stream: "input_stream1"
//   input_stream: "input_streamN"
//   input_stream: "LATCH:latch"
//   input_stream: "RESET:reset"
//   output_stream: "STATE_CHANGE:state_change"
//   output_stream: "output_stream0"
//   output_stream: "output_stream1"
//   output_stream: "output_streamN"
// }
class LatchCalculator : public CalculatorBase {
 public:
  LatchCalculator() {}
  
  

  static ::mediapipe::Status GetContract(CalculatorContract* cc) {
    // Assume that input streams do not have a tag and that gating signal is
    // tagged either ALLOW or DISALLOW.
    RET_CHECK(cc->Inputs().HasTag(kLatchTag));
    const int num_data_streams = cc->Inputs().NumEntries("");
    RET_CHECK_GE(num_data_streams, 1);
    RET_CHECK_EQ(cc->Outputs().NumEntries(""), num_data_streams)
        << "Number of data output streams must match with data input streams.";
    for (int i = 0; i < num_data_streams; ++i) {
      cc->Inputs().Get("", i).SetAny();
      cc->Outputs().Get("", i).SetSameAs(&cc->Inputs().Get("", i));
    }
    if (cc->Inputs().HasTag(kLatchTag)) {
      cc->Inputs().Tag(kLatchTag).Set<bool>();
    }
    if (cc->Inputs().HasTag(kResetTag)) {
      cc->Inputs().Tag(kResetTag).Set<bool>();
    }
    if (cc->Outputs().HasTag(kStateTag)) {
      cc->Outputs().Tag(kStateTag).Set<bool>();
    }
    
    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Open(CalculatorContext* cc) final {
    cc->SetOffset(TimestampDiff(0));
    num_data_streams_ = cc->Inputs().NumEntries("");
    last_latch_state_ = LATCH_UNINITIALIZED;
    RET_CHECK_OK(CopyInputHeadersToOutputs(cc->Inputs(), &cc->Outputs()));

    return ::mediapipe::OkStatus();
  }

  ::mediapipe::Status Process(CalculatorContext* cc) final {

    if (cc->Inputs().HasTag(kLatchTag) && !cc->Inputs().Tag(kLatchTag).IsEmpty()) {
      latched = cc->Inputs().Tag(kLatchTag).Get<bool>();
      cc->Inputs().Tag(kLatchTag).Header().RegisteredTypeName().c_str();
      // std::cout << "\nlatched:" << std::to_string(latched)
      //  << " " << cc->Inputs().Tag(kLatchTag).Header().RegisteredTypeName().c_str();
    }
    
    if (cc->Inputs().HasTag(kResetTag) && !cc->Inputs().Tag(kResetTag).IsEmpty()) {
      latched = !cc->Inputs().Tag(kResetTag).Get<bool>();
    }

    const LatchState new_latch_state = latched ? LATCH_ON : LATCH_OFF;

    if (cc->Outputs().HasTag(kStateTag)) {
      if (last_latch_state_ != LATCH_UNINITIALIZED &&
          last_latch_state_ != new_latch_state) {
        VLOG(2) << "State transition in " << cc->NodeName() << " @ "
                << cc->InputTimestamp().Value() << " from "
                << ToString(last_latch_state_) << " to "
                << ToString(new_latch_state);
        cc->Outputs()
            .Tag(kStateTag)
            .AddPacket(MakePacket<bool>(latched).At(cc->InputTimestamp()));
      }
    }
    last_latch_state_ = new_latch_state;

    if (!latched) {
      return ::mediapipe::OkStatus();
    }

    // Process data streams.
    for (int i = 0; i < num_data_streams_; ++i) {
      if (!cc->Inputs().Get("", i).IsEmpty()) {
        cc->Outputs().Get("", i).AddPacket(cc->Inputs().Get("", i).Value());
      }
    }

    return ::mediapipe::OkStatus();
  }

 private:
  LatchState last_latch_state_ = LATCH_UNINITIALIZED;
  int num_data_streams_;
  bool latched;
  
};
REGISTER_CALCULATOR(LatchCalculator);

}  // namespace mediapipe
