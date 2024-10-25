// Copyright 2023 The XLS Authors
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

#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "xls/common/status/matchers.h"
#include "xls/dslx/type_system/typecheck_test_utils.h"

namespace xls::dslx {
namespace {

using ::absl_testing::StatusIs;
using ::testing::HasSubstr;

TEST(TypecheckTest, ConstantOnStructInstant) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

impl Point {
    const NUM_DIMS = u32:2;
}

fn point_dims(p: Point) -> u32 {
    p::NUM_DIMS
}
)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}

TEST(TypecheckTest, StaticConstantOnStruct) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

impl Point {
    const NUM_DIMS = u32:2;
}

fn point_dims() -> u32 {
    Point::NUM_DIMS
}
)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}

TEST(TypecheckErrorTest, ImplConstantOutsideScope) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

impl Point {
    const NUM_DIMS = u32:2;
}

const GLOBAL_DIMS = NUM_DIMS;
)";
  EXPECT_THAT(Typecheck(kProgram),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Cannot find a definition")));
}

TEST(TypecheckTest, ImplConstantExtracted) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

impl Point {
    const NUM_DIMS = u32:2;
}

const GLOBAL_DIMS = Point::NUM_DIMS;
)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}

TEST(TypecheckErrorTest, ConstantExtractionWithoutImpl) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

const GLOBAL_DIMS = Point::NUM_DIMS;
)";
  EXPECT_THAT(
      Typecheck(kProgram),
      StatusIs(absl::StatusCode::kInvalidArgument,
               HasSubstr("Struct 'Point' has no impl defining 'NUM_DIMS'")));
}

TEST(TypecheckErrorTest, ConstantAccessWithoutImplDef) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

fn point_dims() -> u32 {
    Point::NUM_DIMS
}
)";
  EXPECT_THAT(
      Typecheck(kProgram),
      StatusIs(absl::StatusCode::kInvalidArgument,
               HasSubstr("Struct 'Point' has no impl defining 'NUM_DIMS'")));
}

TEST(TypecheckErrorTest, ImplWithMissingConstant) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

impl Point {
    const NUM_DIMS = u32:2;
}

fn point_dims() -> u32 {
    Point::DIMENSIONS
}
)";
  EXPECT_THAT(
      Typecheck(kProgram),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr(
              "'DIMENSIONS' is not defined by the impl for struct 'Point'")));
}

TEST(TypecheckTest, ImplWithTypeAlias) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

impl Point {
    const NUM_DIMS = u32:2;
}

type ThisPoint = Point;

fn use_point() -> u2 {
    let size = ThisPoint::NUM_DIMS;
    uN[size]:0
}
)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}

TEST(TypecheckErrorTest, ImplWithTypeAliasWrongType) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

impl Point {
    const NUM_DIMS = u32:2;
}

type ThisPoint = Point;

fn use_point() -> u4 {
    let size = ThisPoint::NUM_DIMS;
    uN[size]:0
}
)";
  EXPECT_THAT(Typecheck(kProgram),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("did not match the annotated return type")));
}

TEST(TypecheckErrorTest, TypeAliasConstantAccessWithoutImplDef) {
  constexpr std::string_view kProgram = R"(
struct Point { x: u32, y: u32 }

type ThisPoint = Point;

fn point_dims() -> u32 {
    ThisPoint::NUM_DIMS
}
)";
  EXPECT_THAT(
      Typecheck(kProgram),
      StatusIs(absl::StatusCode::kInvalidArgument,
               HasSubstr("Struct 'Point' has no impl defining 'NUM_DIMS'")));
}

TEST(TypecheckTest, ImplWithMultipleParametricStructs) {
  constexpr std::string_view kProgram = R"(
struct Point<MAX_X: u32> { x: u32, y: u32 }

impl Point<MAX_X> {
    const NUM_DIMS = u32:2;
    const MAX_WIDTH = u32:2 * MAX_X;
}

fn use_point() {
    let p = Point<u32:5>{ x: u32:1, y: u32:0 };
    let pa = Point<u32:4>{ x: u32:0, y: u32:0 };
    assert_eq(uN[p::MAX_WIDTH]:0, u10:0);
    assert_eq(uN[pa::MAX_WIDTH]:0, u8:0);
}
)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}

TEST(TypecheckTest, ImplWithParametricStruct) {
  constexpr std::string_view kProgram = R"(
struct Point<MAX_X: u32> { x: u32, y: u32 }

impl Point<MAX_X> {
    const NUM_DIMS = u32:2;
    const MAX_WIDTH = u32:2 * MAX_X;
}

fn use_point() -> u10 {
    let p = Point<u32:5>{ x: u32:1, y: u32:0 };
    uN[p::MAX_WIDTH]:0
}
)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}

TEST(TypecheckTest, ImplWithParametricStructFromFn) {
  constexpr std::string_view kProgram = R"(
struct Point<MAX_X: u32> { x: u32, y: u32 }

impl Point<MAX_X> {
    const NUM_DIMS = u32:2;
    const MAX_WIDTH = u32:2 * MAX_X;
}

fn use_point<MAX_X: u32>() -> uN[u32:2 * MAX_X] {
    let p = Point<MAX_X>{ x: u32:1, y: u32:0 };
    uN[p::MAX_WIDTH]:0
}

fn main() -> u10 {
    use_point<u32:5>()
}
)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}

TEST(TypecheckTest, ImplWithParametricTypeAlias) {
  constexpr std::string_view kProgram = R"(
struct APFloat<EXP_SZ: u32, FRACTION_SZ: u32> {
    sign: bits[1],
    bexp: bits[EXP_SZ],
    fraction: bits[FRACTION_SZ],
}

impl APFloat<EXP_SZ, FRACTION_SZ> {
    const EXP = EXP_SZ;
}

type BF16 = APFloat<u32:8, u32:7>;

fn bf_ezp() -> u8 {
    let p = BF16 { sign: u1:0, bexp: u8:0, fraction: u7:0 };
    uN[p::EXP]:0
}
)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}

TEST(TypecheckErrorTest, MissingImplWithStructInstance) {
  constexpr std::string_view kProgram = R"(
struct Point<MAX_X: u32> { x: u32, y: u32 }

fn use_point() -> u10 {
    let p = Point<u32:5>{ x: u32:1, y: u32:0 };
    uN[p::MAX_WIDTH]:0
}
)";
  EXPECT_THAT(
      Typecheck(kProgram),
      StatusIs(absl::StatusCode::kInvalidArgument,
               HasSubstr("Struct 'Point' has no impl defining 'MAX_WIDTH'")));
}

TEST(TypecheckTest, ImplWithParametricTypeAliasAsParam) {
  constexpr std::string_view kProgram = R"(
struct Point<MAX_X: u32> { x: u32, y: u32 }

impl Point<MAX_X> {
    const NUM_DIMS = u32:2;
    const MAX_WIDTH = u32:2 * MAX_X;
}

type Narrow = Point<u32: 2>;

fn max_width(n: Narrow) -> u32 {
    n::MAX_WIDTH
}

fn use_point() -> u4 {
    let ezm = Narrow{ x: u32:1, y: u32:0 };
    uN[max_width(ezm)]:0
}

)";
  XLS_EXPECT_OK(Typecheck(kProgram));
}
}  // namespace
}  // namespace xls::dslx
