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

#include <math.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/statusor.h"
#include "mediapipe/util/color.pb.h"
#include "mediapipe/util/render_data.pb.h"
#include "mediapipe/framework/port/opencv_highgui_inc.h"
#include "mediapipe/framework/formats/image_format.pb.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
namespace mediapipe
{

  constexpr char kRenderDataTag[] = "RENDER_DATA";
  constexpr char kTickTag[] = "CVTICK";
  constexpr char kImageFrameTag[] = "IMAGE";
  constexpr float kFontHeightScale = 1.25f;

  // A calculator takes in pairs of labels and scores or classifications, outputs
  // generates render data. Either both "LABELS" and "SCORES" or "CLASSIFICATIONS"
  // must be present.
  //
  // Usage example:
  // node {
  //   calculator: "TickToRenderDataCalculator"
  //   input_stream: "TICK:tick"
  //   output_stream: "RENDER_DATA:fps_render_data"
  //   }
  // }
  class TickToRenderDataCalculator : public CalculatorBase
  {
  public:
    static absl::Status GetContract(CalculatorContract *cc);
    absl::Status Open(CalculatorContext *cc) override;
    absl::Status Process(CalculatorContext *cc) override;

  private:
    int num_colors_ = 0;
    int video_width_ = 0;
    int video_height_ = 0;
    int label_height_px_ = 0;
    int label_left_px_ = 0;
  };
  REGISTER_CALCULATOR(TickToRenderDataCalculator);

  absl::Status TickToRenderDataCalculator::GetContract(CalculatorContract *cc)
  {

    cc->Inputs().Tag(kTickTag).Set<int64>();
    // cc->Inputs().Tag(kImageFrameTag).Set<ImageFrame>();
    cc->Outputs().Tag(kRenderDataTag).Set<RenderData>();

    return absl::OkStatus();
  }

  absl::Status TickToRenderDataCalculator::Open(CalculatorContext *cc)
  {
    cc->SetOffset(TimestampDiff(0));
    return absl::OkStatus();
  }

  absl::Status TickToRenderDataCalculator::Process(CalculatorContext *cc)
  {
    
    const auto& tick_start = cc->Inputs().Tag(kTickTag).Get<int64>();
    int output_fps = cv::getTickFrequency() / (cv::getTickCount() - tick_start);

    RenderData render_data;
    auto *label_annotation = render_data.add_render_annotations();
    label_annotation->set_thickness(5.0);
    label_annotation->mutable_color()->set_r(0);
    label_annotation->mutable_color()->set_g(255);
    label_annotation->mutable_color()->set_b(0);
    auto *text = label_annotation->mutable_text();
    std::string display_text = std::to_string(output_fps);
    // std::string display_text = std::to_string(30);
    text->set_display_text(display_text);
    text->set_font_height(40);
    text->set_left(50+420);
    text->set_baseline(50+20);
    text->set_font_face(2);

    cc->Outputs()
        .Tag(kRenderDataTag)
        .AddPacket(MakePacket<RenderData>(render_data).At(cc->InputTimestamp()));

    return absl::OkStatus();
  }
} // namespace mediapipe
