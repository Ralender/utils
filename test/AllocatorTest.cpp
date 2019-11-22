//
// Created by tyker on 2019-11-22.
//

#include "src/StackedBumpAllocator.hpp"
#include "gtest/gtest.h"
#include <cstdlib>
#include <vector>
#include <chrono>
#include <random>
#include <cassert>

using namespace llvm;

namespace {
struct StackedBumpAllocChecker {
  StackedBumpAllocator<> Allocator;
  struct AllocData {
    uint64_t* Ptr;
    uint64_t Value;
    size_t Size;
  };
  uint64_t NextValue = 1;

  struct Action {
    enum {Push, Pop, Allocate} Kind;
    size_t AllocSize = 0;
  };

  std::vector<AllocData> Allocations;
  std::vector<size_t> Levels;
  std::vector<Action> History;
  void WriteAlloc(AllocData& Data, uint64_t Value) {
    Data.Value = Value;
    for (uint64_t* It = Data.Ptr; It < Data.Ptr + Data.Size; It++) {
      *It = Data.Value;
    }
  }
  void ReadAlloc(AllocData Data) {
    for (uint64_t* It = Data.Ptr; It < Data.Ptr + Data.Size; It++) {
      ASSERT_EQ(*It, Data.Value);
    }
  }
public:
  StackedBumpAllocChecker() {
    Allocations.reserve(20);
    Levels.reserve(5);
  }
  void Verify() {
    for (auto& Alloc : Allocations) {
      ReadAlloc(Alloc);
      WriteAlloc(Alloc, NextValue++);
    }
  }
  void PushFrame() {
    History.push_back({Action::Push});
    Levels.emplace_back(Allocations.size());
    Allocator.PushFrame();
    Verify();
  }
  void PopFrame() {
    History.push_back({Action::Pop});
    Allocator.PopFrame();
    Allocations.resize(Levels.back());
    Levels.pop_back();
    Verify();
  }
  void* Allocate(size_t Size) {
    History.push_back({Action::Allocate, Size});
    Size = (Size + 7) & ~7;
    uint64_t* Ptr = (uint64_t*)Allocator.Allocate(Size, alignof(uint64_t));
    for (auto It = Allocations.begin(); It < Allocations.end(); It++) {
      assert(It->Ptr != Ptr);
    }
    Allocations.push_back(AllocData{Ptr, NextValue, Size / sizeof(uint64_t)});
    WriteAlloc(Allocations.back(), NextValue++);
    return Ptr;
  }
  void Reset() {
    Verify();
    Allocator.Reset();
    Allocations.clear();
    Levels.clear();
    History.clear();
  }
};

TEST(AllocatorTest, StackedBumpBasic) {
  StackedBumpAllocChecker Alloc;

  Alloc.PushFrame();
  Alloc.Allocate(64);
  Alloc.Allocate(256);
  Alloc.PopFrame();
}

TEST(AllocatorTest, StackedBumpThree) {
  StackedBumpAllocChecker Alloc;
  Alloc.PushFrame();
  Alloc.Allocate(64);
  Alloc.Allocate(64);
  Alloc.Allocate(64);
  Alloc.PushFrame();
  Alloc.Allocate(64);
  Alloc.PushFrame();
  Alloc.Allocate(64);
  Alloc.PopFrame();
  Alloc.Allocate(8);
  Alloc.PopFrame();
  Alloc.Allocate(8);
  Alloc.PopFrame();
  Alloc.Reset();
}

TEST(AllocatorTest, StackedBumpSlabs) {
  StackedBumpAllocChecker Alloc;
  Alloc.PushFrame();
  Alloc.Allocate(2048);
  Alloc.Allocate(2048);
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 2u);
  Alloc.PushFrame();
  Alloc.Allocate(824);
  Alloc.Allocate(824);
  Alloc.Allocate(824);
  Alloc.Allocate(824);
  Alloc.Allocate(3500);
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 4u);
  Alloc.PushFrame();
  Alloc.Allocate(16);
  Alloc.Allocate(16);
  Alloc.Allocate(16);
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 4u);
  Alloc.PopFrame();
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 4u);
  Alloc.PopFrame();
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 2u);
  Alloc.PopFrame();
}

TEST(AllocatorTest, StackedBumpCostumSlabs) {
  StackedBumpAllocChecker Alloc;
  Alloc.PushFrame();
  Alloc.Allocate(2048);
  Alloc.Allocate(2048);
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 2u);
  Alloc.PushFrame();
  Alloc.Allocate(8000);
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 3u);
  Alloc.Allocate(8000);
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 4u);
  Alloc.PushFrame();
  Alloc.Allocate(8000);
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 5u);
  Alloc.PopFrame();
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 4u);
  Alloc.PopFrame();
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 2u);
  Alloc.PopFrame();
  ASSERT_EQ(Alloc.Allocator.GetNumSlabs(), 1u);
}

TEST(AllocatorTest, StackedBumpMix) {
  StackedBumpAllocChecker Alloc;
  Alloc.Allocate(64);
  Alloc.PushFrame();
  Alloc.Allocate(4000);
  Alloc.Allocate(56);
  Alloc.PushFrame();
  Alloc.Allocate(8000);
  Alloc.PushFrame();
  Alloc.Allocate(700);
  Alloc.Allocate(700);
  Alloc.Allocate(700);
  Alloc.Allocate(700);
  Alloc.Allocate(700);
  Alloc.PopFrame();
  Alloc.Allocate(8000);
  Alloc.Allocate(700);
  Alloc.Allocate(4000);
  Alloc.Allocate(4000);
  Alloc.PopFrame();
  Alloc.Reset();
  Alloc.Allocate(64);
  Alloc.PushFrame();
  Alloc.Allocate(4000);
  Alloc.Allocate(56);
  Alloc.PushFrame();
  Alloc.Allocate(8000);
  Alloc.PushFrame();
  Alloc.Allocate(700);
  Alloc.Allocate(700);
  Alloc.Allocate(700);
  Alloc.Allocate(700);
  Alloc.Allocate(700);
  Alloc.PopFrame();
  Alloc.Allocate(8000);
  Alloc.Allocate(700);
  Alloc.Allocate(4000);
  Alloc.Allocate(4000);
  Alloc.PopFrame();
}

TEST(AllocatorTest, MoveAllocator) {
  llvm::StackedBumpAllocator<> Alloc;
  Alloc.Allocate(600, 8);
  Alloc.PushFrame();
  ASSERT_GT(Alloc.getBytesAllocated(), 600u);
  ASSERT_EQ(Alloc.HasNoFrame(), false);
  llvm::StackedBumpAllocator<> Alloc1(std::move(Alloc));
  ASSERT_GT(Alloc1.getBytesAllocated(), 600u);
  ASSERT_EQ(Alloc1.HasNoFrame(), false);
  ASSERT_EQ(Alloc.getBytesAllocated(), 0u);
  ASSERT_EQ(Alloc.HasNoFrame(), true);
  Alloc = std::move(Alloc1);
  ASSERT_GT(Alloc.getBytesAllocated(), 600u);
  ASSERT_EQ(Alloc.HasNoFrame(), false);
  ASSERT_EQ(Alloc1.getBytesAllocated(), 0u);
  ASSERT_EQ(Alloc1.HasNoFrame(), true);
}

using namespace std::chrono_literals;

TEST(AllocatorTest, Fuzzer2ms) {
  auto Start = std::chrono::system_clock::now();
  StackedBumpAllocChecker Alloc;
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist_actions(1, 100);
  std::uniform_int_distribution<std::mt19937::result_type> dist_alloc_size(1,
                                                                           625);

  while (std::chrono::system_clock::now() < Start + 5min) {
    int r = dist_actions(rng);
    if (r < 10) {
      if (Alloc.Levels.size() != 0)
        Alloc.PopFrame();
    } else if (r < 20)
      Alloc.PushFrame();
    else if (r < 30)
      Alloc.Reset();
    else
      Alloc.Allocate(dist_alloc_size(rng) * 8);
  }
}

}