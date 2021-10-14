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


class FPSCalculator : public CalculatorBase {
 public:

  // tj : note: have to set all inputs and outputs, otherwsie grash init will fail
  static absl::Status GetContract(CalculatorContract* cc) {
    cc->Inputs().Index(0).Set<int>();
    cc->Inputs().Tag(kTickTag).Set<int64>();
    cc->Outputs().Index(0).Set<int>();
    cc->Outputs().Tag(kFPSTag).Set<int64>();
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

const auto& tick_start =
        cc->Inputs().Tag(kTickTag).Get<int64>();

auto output_fps = absl::make_unique<int64>(cv::getTickFrequency() / (cv::getTickCount() - tick_start));

  auto output_int = absl::make_unique<int>(cc->Inputs().Index(0).Get<int>());
  cc->Outputs().Index(0).Add(output_int.release(), cc->InputTimestamp());
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
        input_stream: "tick"
        output_stream: "out"
        output_stream: "fps"
        node {
          calculator: "PassThroughCalculator"
          input_stream: "in"
          output_stream: "out1"
        }
        node {
          calculator: "FPSCalculator"
          input_stream: "out1"
          input_stream: "TICK:tick"
          output_stream: "out"
          output_stream: "FPS:fps"
        }
      )pb");

  CalculatorGraph graph;
  MP_RETURN_IF_ERROR(graph.Initialize(config));
  ASSIGN_OR_RETURN(OutputStreamPoller poller,
                   graph.AddOutputStreamPoller("out"));
  ASSIGN_OR_RETURN(OutputStreamPoller poller_fps,
                   graph.AddOutputStreamPoller("fps"));
  MP_RETURN_IF_ERROR(graph.StartRun({}));
  // Give 10 input packets that contains the same std::string "Hello World!".
  for (int i = 0; i < 2; ++i) {
    MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
        "in", MakePacket<int>(i).At(Timestamp(i))));
    MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
    "tick", MakePacket<int64>(cv::getTickCount()).At(Timestamp(i))));
  }
  // Close the input stream "in".
  MP_RETURN_IF_ERROR(graph.CloseInputStream("in"));
  MP_RETURN_IF_ERROR(graph.CloseInputStream("tick"));
  mediapipe::Packet packet, packet2;
  // Get the output packets std::string.
  while (poller.Next(&packet) && poller_fps.Next(&packet2)) {
    LOG(INFO) << packet.Get<int>() << " " << packet2.Get<int64>();
  }
  return graph.WaitUntilDone();
}

}  // namespace mediapipe

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  CHECK(mediapipe::PrintHelloWorld().ok());
  return 0;
}