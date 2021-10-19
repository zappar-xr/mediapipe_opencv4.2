// Copyright 2019 The MediaPipe Authors.
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
//
// A simple example to print out "Hello World!" from a MediaPipe graph.

#include "mediapipe/framework/calculator_graph.h"
#include "mediapipe/framework/port/logging.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/opencv_highgui_inc.h"
#include "mediapipe/framework/calculator_framework.h"

namespace mediapipe {

constexpr char kTickTag[] = "TICK";
constexpr char kFPSTag[] = "FPS";


class TickCalculator : public CalculatorBase {
 public:

  // tj : note: have to set all inputs and outputs, otherwsie graph init will fail
  // tj : identified the stream either by index (no tag) or tag (has tag), same tags have to be differentiable by additional indices
  static absl::Status GetContract(CalculatorContract* cc) {
    for (CollectionItemId id = cc->Inputs().BeginId();
         id < cc->Inputs().EndId(); ++id) {
      cc->Inputs().Get(id).SetAny();
    }
    cc->Outputs().Tag(kTickTag).Set<int64>();
    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) final {
    cc->SetOffset(TimestampDiff(0));
    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) final {
    auto tick = absl::make_unique<int64>(cv::getTickCount() );
    cc->Outputs().Tag(kTickTag).Add(tick.release(), cc->InputTimestamp());
    return absl::OkStatus();
  }
};
REGISTER_CALCULATOR(TickCalculator);


class ProcessCalculator : public CalculatorBase {
 public:

  // tj : note: have to set all inputs and outputs, otherwsie graph init will fail
  // tj : identified the stream either by index (no tag) or tag (has tag), same tags have to be differentiable by additional indices
  static absl::Status GetContract(CalculatorContract* cc) {


    cc->Inputs().Index(0).SetAny();
    cc->Outputs().Tag(kTickTag).Set<int64>();
    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) final {
    cc->SetOffset(TimestampDiff(0));
    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) final {
    absl::SleepFor(absl::Milliseconds(5));
    auto tick = absl::make_unique<int64>(cv::getTickCount() );
    
    cc->Outputs().Tag(kTickTag).Add(tick.release(), cc->InputTimestamp());
    return absl::OkStatus();
  }
};
REGISTER_CALCULATOR(ProcessCalculator);

class FPSCalculator : public CalculatorBase {
 public:

  // tj : note: have to set all inputs and outputs, otherwsie graph init will fail
  // tj : identified the stream either by index (no tag) or tag (has tag), same tags have to be differentiable by additional indices
  static absl::Status GetContract(CalculatorContract* cc) {

    cc->Inputs().Get(kTickTag, 0).Set<int64>();
    cc->Inputs().Get(kTickTag, 1).Set<int64>();
    cc->Outputs().Tag(kFPSTag).Set<int64>();
    // cc->Outputs().Index(1).Set<int64>();
    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) final {
    // for (CollectionItemId id = cc->Inputs().BeginId();
    //      id < cc->Inputs().EndId(); ++id) {
    //   if (!cc->Inputs().Get(id).Header().IsEmpty()) {
    //     cc->Outputs().Get(id).SetHeader(cc->Inputs().Get(id).Header());
    //   }
    // }
    // if (cc->OutputSidePackets().NumEntries() != 0) {
    //   for (CollectionItemId id = cc->InputSidePackets().BeginId();
    //        id < cc->InputSidePackets().EndId(); ++id) {
    //     cc->OutputSidePackets().Get(id).Set(cc->InputSidePackets().Get(id));
    //   }
    // }
    cc->SetOffset(TimestampDiff(0));
    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) final {
    // cc->GetCounter("PassThrough")->Increment();
    // if (cc->Inputs().NumEntries() == 0) {
    //   return tool::StatusStop();
    // }

const auto& tick_start = cc->Inputs().Get(kTickTag, 0).Get<int64>();
const auto& tick_end = cc->Inputs().Get(kTickTag, 1).Get<int64>();
const auto& tick_curr = cv::getTickCount();

std::cout << tick_start << " " << tick_end << " " << tick_curr << std::endl;

auto output_fps = absl::make_unique<int64>(cv::getTickFrequency() / (tick_end - tick_start));
std::cout << *output_fps << std::endl;
  cc->Outputs().Tag(kFPSTag).Add(output_fps.release(),  cc->InputTimestamp());
    return absl::OkStatus();
  }
};
REGISTER_CALCULATOR(FPSCalculator);

absl::Status PrintHelloWorld() {
  // Configures a simple graph, which concatenates 2 PassThroughCalculators.
  CalculatorGraphConfig config =
      ParseTextProtoOrDie<CalculatorGraphConfig>(R"pb(
        input_stream: "in"
        # output_stream: "out"
        output_stream: "fps"

        node {
          calculator: "TickCalculator"
          input_stream: "in"
          output_stream: "TICK:tick_start"
        }

        node {
          calculator: "ProcessCalculator"
          input_stream: "in"
          output_stream: "TICK:tick_end"
        }

        node {
          calculator: "FPSCalculator"
          input_stream: "TICK:0:tick_start"
          input_stream: "TICK:1:tick_end"
          output_stream: "FPS:fps"
        }
      )pb");

  CalculatorGraph graph;
  MP_RETURN_IF_ERROR(graph.Initialize(config));

  ASSIGN_OR_RETURN(OutputStreamPoller poller_fps,
                   graph.AddOutputStreamPoller("fps"));

  MP_RETURN_IF_ERROR(graph.StartRun({}));
  // Give 10 input packets that contains the same std::string "Hello World!".
  for (int i = 0; i < 2; ++i) {
    MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
        "in", MakePacket<int>(i).At(Timestamp(i))));
    // MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
    // "tick", MakePacket<int64>(cv::getTickCount()).At(Timestamp(i))));
  }
  // Close the input stream "in".

  MP_RETURN_IF_ERROR(graph.CloseInputStream("in"));
  mediapipe::Packet packet2;
  // Get the output packets std::string.
  while (poller_fps.Next(&packet2)) {
    LOG(INFO) << packet2.Get<int64>();
  }
  return graph.WaitUntilDone();
}

}  // namespace mediapipe

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  CHECK(mediapipe::PrintHelloWorld().ok());
  return 0;
}