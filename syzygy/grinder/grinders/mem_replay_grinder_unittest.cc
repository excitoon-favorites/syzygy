// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "syzygy/grinder/grinders/mem_replay_grinder.h"

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/strings/string_util.h"
#include "gtest/gtest.h"
#include "syzygy/bard/events/heap_alloc_event.h"
#include "syzygy/pe/unittest_util.h"

namespace grinder {
namespace grinders {

namespace {

const char kHeapAlloc[] = "asan_HeapAlloc";

class TestMemReplayGrinder : public MemReplayGrinder {
 public:
  // Types.
  using ProcessData = MemReplayGrinder::ProcessData;

  // Member variables.
  using MemReplayGrinder::function_enum_map_;
  using MemReplayGrinder::missing_events_;
  using MemReplayGrinder::parse_error_;
  using MemReplayGrinder::process_data_map_;

  // Member functions.
  using MemReplayGrinder::FindOrCreatePlotLine;
  using MemReplayGrinder::FindOrCreateProcessData;

  // Creates and dispatches a TraceFunctionNameTableEntry event.
  void PlayFunctionNameTableEntry(uint32_t process_id,
                                  uint32_t function_id,
                                  const base::StringPiece& function_name) {
    size_t buffer_size =
        offsetof(TraceFunctionNameTableEntry, name) + function_name.size() + 1;
    std::vector<uint8_t> buffer(buffer_size, 0);
    auto data = reinterpret_cast<TraceFunctionNameTableEntry*>(buffer.data());
    data->function_id = function_id;
    data->name_length = function_name.size();
    // The name doesn't need to be null terminated as the length is explicitly
    // encoded. So using strncpy is perfectly safe.
    ::strncpy(data->name, function_name.data(), function_name.size());
    OnFunctionNameTableEntry(base::Time::Now(), process_id, data);
  }

  // Creates and dispatches a heap alloc function call.
  void PlayHeapAllocCall(uint32_t process_id,
                         uint32_t thread_id,
                         uint64_t timestamp,
                         uint32_t function_id,
                         uint32_t stack_trace_id,
                         HANDLE handle,
                         DWORD flags,
                         SIZE_T bytes,
                         LPVOID ret) {
    size_t arg_data_size = 5 * sizeof(uint32) + sizeof(handle) + sizeof(flags) +
                           sizeof(bytes) + sizeof(ret);
    size_t buffer_size =
        offsetof(TraceDetailedFunctionCall, argument_data) + arg_data_size;
    std::vector<uint8_t> buffer(buffer_size, 0);
    auto data = reinterpret_cast<TraceDetailedFunctionCall*>(buffer.data());
    data->timestamp = timestamp;
    data->function_id = function_id;
    data->stack_trace_id = stack_trace_id;
    data->argument_data_size = arg_data_size;

    // Output the argument data.
    uint8_t* cursor = data->argument_data;
    *reinterpret_cast<uint32_t*>(cursor) = 4;
    cursor += sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(cursor) = sizeof(handle);
    cursor += sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(cursor) = sizeof(flags);
    cursor += sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(cursor) = sizeof(bytes);
    cursor += sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(cursor) = sizeof(ret);
    cursor += sizeof(uint32_t);
    *reinterpret_cast<HANDLE*>(cursor) = handle;
    cursor += sizeof(HANDLE);
    *reinterpret_cast<DWORD*>(cursor) = flags;
    cursor += sizeof(DWORD);
    *reinterpret_cast<SIZE_T*>(cursor) = bytes;
    cursor += sizeof(SIZE_T);
    *reinterpret_cast<LPVOID*>(cursor) = ret;
    cursor += sizeof(LPVOID);
    DCHECK_EQ(static_cast<ptrdiff_t>(arg_data_size),
              cursor - data->argument_data);

    OnDetailedFunctionCall(base::Time::Now(), process_id, thread_id, data);
  }
};

class MemReplayGrinderTest : public testing::Test {
 public:
  MemReplayGrinderTest() : cmd_line_(base::FilePath(L"grinder.exe")) {}

  base::CommandLine cmd_line_;
};

}  // namespace

TEST_F(MemReplayGrinderTest, ParseCommandLine) {
  TestMemReplayGrinder grinder;
  EXPECT_TRUE(grinder.function_enum_map_.empty());
  EXPECT_TRUE(grinder.ParseCommandLine(&cmd_line_));
  EXPECT_FALSE(grinder.function_enum_map_.empty());
}

TEST_F(MemReplayGrinderTest, RecognizedFunctionName) {
  TestMemReplayGrinder grinder;
  ASSERT_TRUE(grinder.ParseCommandLine(&cmd_line_));
  grinder.PlayFunctionNameTableEntry(1, 1, kHeapAlloc);
  EXPECT_EQ(1u, grinder.process_data_map_.size());

  auto proc_data = grinder.FindOrCreateProcessData(1);
  EXPECT_EQ(1u, proc_data->function_id_map.size());
  auto it = proc_data->function_id_map.begin();
  EXPECT_EQ(1u, it->first);
  EXPECT_EQ(bard::EventInterface::EventType::kHeapAllocEvent, it->second);
}

TEST_F(MemReplayGrinderTest, UnrecognizedFunctionName) {
  static const char kDummyFunction[] = "DummyFunction";
  TestMemReplayGrinder grinder;
  ASSERT_TRUE(grinder.ParseCommandLine(&cmd_line_));
  grinder.PlayFunctionNameTableEntry(1, 1, kDummyFunction);
  EXPECT_TRUE(grinder.process_data_map_.empty());
  EXPECT_EQ(1u, grinder.missing_events_.size());
  EXPECT_STREQ(kDummyFunction, grinder.missing_events_.begin()->c_str());
}

TEST_F(MemReplayGrinderTest, NameBeforeCall) {
  TestMemReplayGrinder grinder;
  ASSERT_TRUE(grinder.ParseCommandLine(&cmd_line_));

  const HANDLE kHandle = reinterpret_cast<HANDLE>(0xDEADBEEF);
  const DWORD kFlags = 0xFF;
  const SIZE_T kBytes = 247;
  const LPVOID kRet = reinterpret_cast<LPVOID>(0xBAADF00D);

  grinder.PlayFunctionNameTableEntry(1, 1, kHeapAlloc);
  EXPECT_EQ(1u, grinder.process_data_map_.size());
  auto proc_data = grinder.FindOrCreateProcessData(1);
  EXPECT_EQ(1u, proc_data->function_id_map.size());
  EXPECT_TRUE(proc_data->pending_function_ids.empty());
  EXPECT_TRUE(proc_data->pending_calls.empty());
  EXPECT_TRUE(proc_data->plot_line_map.empty());

  // The function call is processed immediately upon being seen.
  grinder.PlayHeapAllocCall(1, 1, 0, 1, 0, kHandle, kFlags, kBytes, kRet);
  EXPECT_EQ(1u, proc_data->function_id_map.size());
  EXPECT_TRUE(proc_data->pending_function_ids.empty());
  EXPECT_TRUE(proc_data->pending_calls.empty());
  EXPECT_EQ(1u, proc_data->plot_line_map.size());
  auto plot_line = grinder.FindOrCreatePlotLine(proc_data, 1);
  EXPECT_EQ(1u, plot_line->size());
  auto evt = (*plot_line)[0];
  EXPECT_EQ(bard::EventInterface::EventType::kHeapAllocEvent, evt->type());
  auto ha = reinterpret_cast<const bard::events::HeapAllocEvent*>(&(*evt));
  EXPECT_EQ(kHandle, ha->trace_heap());
  EXPECT_EQ(kFlags, ha->flags());
  EXPECT_EQ(kBytes, ha->bytes());
  EXPECT_EQ(kRet, ha->trace_alloc());
}

TEST_F(MemReplayGrinderTest, CallBeforeName) {
  TestMemReplayGrinder grinder;
  ASSERT_TRUE(grinder.ParseCommandLine(&cmd_line_));

  const HANDLE kHandle = reinterpret_cast<HANDLE>(0xDEADBEEF);
  const DWORD kFlags = 0xFF;
  const SIZE_T kBytes = 247;
  const LPVOID kRet = reinterpret_cast<LPVOID>(0xBAADF00D);

  // The function call is seen before the corresponding function name is
  // defined so no parsing can happen. In this case the call should be
  // placed to the pending list.
  grinder.PlayHeapAllocCall(1, 1, 0, 1, 0, kHandle, kFlags, kBytes, kRet);
  EXPECT_EQ(1u, grinder.process_data_map_.size());
  auto proc_data = grinder.FindOrCreateProcessData(1);
  EXPECT_TRUE(proc_data->function_id_map.empty());
  EXPECT_EQ(1u, proc_data->pending_function_ids.size());
  EXPECT_EQ(1u, proc_data->pending_calls.size());
  EXPECT_TRUE(proc_data->plot_line_map.empty());

  // And processed once the name is defined.
  grinder.PlayFunctionNameTableEntry(1, 1, kHeapAlloc);
  EXPECT_EQ(1u, proc_data->function_id_map.size());
  EXPECT_TRUE(proc_data->pending_function_ids.empty());
  EXPECT_TRUE(proc_data->pending_calls.empty());
  EXPECT_EQ(1u, proc_data->plot_line_map.size());
  auto plot_line = grinder.FindOrCreatePlotLine(proc_data, 1);
  EXPECT_EQ(1u, plot_line->size());
  auto evt = (*plot_line)[0];
  EXPECT_EQ(bard::EventInterface::EventType::kHeapAllocEvent, evt->type());
  auto ha = reinterpret_cast<const bard::events::HeapAllocEvent*>(&(*evt));
  EXPECT_EQ(kHandle, ha->trace_heap());
  EXPECT_EQ(kFlags, ha->flags());
  EXPECT_EQ(kBytes, ha->bytes());
  EXPECT_EQ(kRet, ha->trace_alloc());
}

TEST_F(MemReplayGrinderTest, GrindHarnessTrace) {
  TestMemReplayGrinder grinder;
  EXPECT_TRUE(grinder.ParseCommandLine(&cmd_line_));

  trace::parser::Parser parser;
  ASSERT_TRUE(parser.Init(&grinder));
  base::FilePath trace_file =
      testing::GetExeTestDataRelativePath(testing::kMemProfTraceFile);
  ASSERT_TRUE(parser.OpenTraceFile(trace_file));
  grinder.SetParser(&parser);
  EXPECT_TRUE(parser.Consume());

  EXPECT_FALSE(grinder.parse_error_);

  // TODO(chrisha): Implement Grind!
  EXPECT_FALSE(grinder.Grind());
}

}  // namespace grinders
}  // namespace grinder
