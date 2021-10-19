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
#include "mediapipe/framework/port/opencv_highgui_inc.h"

namespace mediapipe
{

  constexpr char kTickTag[] = "TICK";
  // A calculator takes in pairs of labels and scores or classifications, outputs
  // generates render data. Either both "LABELS" and "SCORES" or "CLASSIFICATIONS"
  // must be present.
  //
  // Usage example:
        // node {
        //   calculator: "TickCalculator"
        //   input_stream: "in"
        //   output_stream: "TICK:tick_start"
        // }
  // }
  class TickCalculator : public CalculatorBase
  {
  public:
    static absl::Status GetContract(CalculatorContract *cc);
    absl::Status Open(CalculatorContext *cc) override;
    absl::Status Process(CalculatorContext *cc) override;

  };
  REGISTER_CALCULATOR(TickCalculator);

  absl::Status TickCalculator::GetContract(CalculatorContract *cc)
  {

    for (CollectionItemId id = cc->Inputs().BeginId();
         id < cc->Inputs().EndId(); ++id) {
      cc->Inputs().Get(id).SetAny();
    }
    cc->Outputs().Tag(kTickTag).Set<int64>();

    return absl::OkStatus();
  }

  absl::Status TickCalculator::Open(CalculatorContext *cc)
  {
    cc->SetOffset(TimestampDiff(0));
    return absl::OkStatus();
  }

  absl::Status TickCalculator::Process(CalculatorContext *cc)
  {
    
    auto tick = absl::make_unique<int64>(cv::getTickCount() );
    cc->Outputs().Tag(kTickTag).Add(tick.release(), cc->InputTimestamp());

    return absl::OkStatus();
  }
} // namespace mediapipe
