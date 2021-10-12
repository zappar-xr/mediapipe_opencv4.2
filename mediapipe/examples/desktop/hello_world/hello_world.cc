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
#include "mediapipe/framework/stream_handler/fixed_size_input_stream_handler.pb.h"
#include "mediapipe/framework/port/threadpool.h"


namespace mediapipe {
    class FixedPassThroughCalculator : public CalculatorBase {
    public:
      static absl::Status GetContract(CalculatorContract* cc) {
        if (!cc->Inputs().TagMap()->SameAs(*cc->Outputs().TagMap())) {
          return absl::InvalidArgumentError(
              "Input and output streams to PassThroughCalculator must use "
              "matching tags and indexes.");
        }
        for (CollectionItemId id = cc->Inputs().BeginId();
            id < cc->Inputs().EndId(); ++id) {
          cc->Inputs().Get(id).SetAny();
          cc->Outputs().Get(id).SetSameAs(&cc->Inputs().Get(id));
        }
        for (CollectionItemId id = cc->InputSidePackets().BeginId();
            id < cc->InputSidePackets().EndId(); ++id) {
          cc->InputSidePackets().Get(id).SetAny();
        }
        if (cc->OutputSidePackets().NumEntries() != 0) {
          if (!cc->InputSidePackets().TagMap()->SameAs(
                  *cc->OutputSidePackets().TagMap())) {
            return absl::InvalidArgumentError(
                "Input and output side packets to PassThroughCalculator must use "
                "matching tags and indexes.");
          }
          for (CollectionItemId id = cc->InputSidePackets().BeginId();
              id < cc->InputSidePackets().EndId(); ++id) {
            cc->OutputSidePackets().Get(id).SetSameAs(
                &cc->InputSidePackets().Get(id));
          }
        }

        // Assign this calculator's InputStreamHandler and options.
        cc->SetInputStreamHandler("FixedSizeInputStreamHandler");
        MediaPipeOptions options;
        options.MutableExtension(FixedSizeInputStreamHandlerOptions::ext)
            ->set_fixed_min_size(2);
        options.MutableExtension(FixedSizeInputStreamHandlerOptions::ext)
            ->set_trigger_queue_size(2);
        options.MutableExtension(FixedSizeInputStreamHandlerOptions::ext)
            ->set_target_queue_size(2);
        cc->SetInputStreamHandlerOptions(options);

        return absl::OkStatus();
      }

      absl::Status Open(CalculatorContext* cc) final {
        for (CollectionItemId id = cc->Inputs().BeginId();
            id < cc->Inputs().EndId(); ++id) {
          if (!cc->Inputs().Get(id).Header().IsEmpty()) {
            cc->Outputs().Get(id).SetHeader(cc->Inputs().Get(id).Header());
          }
        }
        if (cc->OutputSidePackets().NumEntries() != 0) {
          for (CollectionItemId id = cc->InputSidePackets().BeginId();
              id < cc->InputSidePackets().EndId(); ++id) {
            cc->OutputSidePackets().Get(id).Set(cc->InputSidePackets().Get(id));
          }
        }
        cc->SetOffset(TimestampDiff(0));
        return absl::OkStatus();
      }

      absl::Status Process(CalculatorContext* cc) final {
        cc->GetCounter("PassThrough")->Increment();
        if (cc->Inputs().NumEntries() == 0) {
          return tool::StatusStop();
        }
        for (CollectionItemId id = cc->Inputs().BeginId();
            id < cc->Inputs().EndId(); ++id) {
          if (!cc->Inputs().Get(id).IsEmpty()) {
            VLOG(3) << "Passing " << cc->Inputs().Get(id).Name() << " to "
                    << cc->Outputs().Get(id).Name() << " at "
                    << cc->InputTimestamp().DebugString();
            cc->Outputs().Get(id).AddPacket(cc->Inputs().Get(id).Value());
          }
        }
        return absl::OkStatus();
      }
    };
    REGISTER_CALCULATOR(FixedPassThroughCalculator);
  
    absl::Status PrintHelloWorld() {
      // Configures a simple graph, which concatenates 2 PassThroughCalculators.
      #define NUM_STREAMS 4
      CalculatorGraphConfig config =
          mediapipe::ParseTextProtoOrDie<CalculatorGraphConfig>(
              R"pb(
                input_stream: "in_0"
                input_stream: "in_1"
                input_stream: "in_2"
                input_stream: "in_3"
                node {
                  calculator: "FixedPassThroughCalculator"
                  input_stream: "in_0"
                  input_stream: "in_1"
                  input_stream: "in_2"
                  input_stream: "in_3"
                  output_stream: "out_0"
                  output_stream: "out_1"
                  output_stream: "out_2"
                  output_stream: "out_3"
                  # FixedSizeInputStreamHandler set in GetContract()
                })pb");
      // CalculatorGraph graph;
      // MP_RETURN_IF_ERROR(graph.Initialize(config));
      // ASSIGN_OR_RETURN(OutputStreamPoller poller_0,
      //                  graph.AddOutputStreamPoller("out_0"));
      // // ASSIGN_OR_RETURN(OutputStreamPoller poller_1,
      // //                  graph.AddOutputStreamPoller("out_1"));
      // MP_RETURN_IF_ERROR(graph.StartRun({}));
      // // Give 10 input packets that contains the same std::string "Hello World!".
      // for (int i = 0; i < 10; ++i) {
      //   MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
      //       "in_0", MakePacket<std::string>("hello, world").At(Timestamp(i))));
      //   MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
      //       "in_1", MakePacket<int64>(cv::getTickCount()).At(Timestamp(i))));
      // }
      // // Close the input stream "in".
      // MP_RETURN_IF_ERROR(graph.CloseInputStream("in_0"));
      // // MP_RETURN_IF_ERROR(graph.CloseInputStream("in_1"));
      // mediapipe::Packet packet_0, packet_1;
      // // Get the output packets std::string.
      // while (poller_0.Next(&packet_0) ) {
      //   LOG(INFO) << packet_0.Get<std::string>();
      // // while (poller_0.Next(&packet_0) && poller_1.Next(&packet_1)) {
      // //   LOG(INFO) << packet_0.Get<std::string>() << packet_1.Get<int64>();
      // }

    ///////////////////////
      std::vector<Packet> output_packets[NUM_STREAMS];
      for (int i = 0; i < NUM_STREAMS; ++i) {
        tool::AddVectorSink(absl::StrCat("out_", i), &config,
                            &output_packets[i]);
      }
      CalculatorGraph graph;
      MP_RETURN_IF_ERROR(graph.Initialize(config));
      MP_RETURN_IF_ERROR(graph.StartRun({}));

      {
        mediapipe::ThreadPool pool(NUM_STREAMS);
        pool.StartWorkers();

        // Start writers.
        for (int w = 0; w < NUM_STREAMS; ++w) {
          pool.Schedule([&, w]() {
            std::string stream_name = absl::StrCat("in_", w);
            for (int i = 0; i < 50; ++i) {
              Packet p = MakePacket<int>(i).At(Timestamp(i));
              MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(stream_name, p));
              absl::SleepFor(absl::Microseconds(100));
            }
          });
        }
      }
      MP_RETURN_IF_ERROR(graph.CloseAllInputStreams());
      MP_RETURN_IF_ERROR(graph.WaitUntilDone());
      for (int i = 0; i < NUM_STREAMS; ++i) {
        // EXPECT_EQ(output_packets[i].size(), output_packets[0].size());
        for (int j = 0; j < output_packets[i].size(); j++) {
          LOG(INFO) << output_packets[i][j].Get<int>();
          // EXPECT_EQ(output_packets[i][j].Get<int>(),
          //           output_packets[0][j].Get<int>());
        }
      }
        
        return graph.WaitUntilDone();
      }
}  // namespace mediapipe

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  CHECK(mediapipe::PrintHelloWorld().ok());
  return 0;
}
