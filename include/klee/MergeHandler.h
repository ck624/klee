//===-- MergeHandler.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/** 
 * @file MergeHandler.h
 * @brief Implementation of the region based merging
 *
 * ## Basic usage:
 * 
 * @code{.cpp}
 * klee_open_merge();
 * 
 * code containing branches etc. 
 * 
 * klee_close_merge();
 * @endcode
 * 
 * Will lead to all states that forked from the state that executed the
 * klee_open_merge() being merged in the klee_close_merge(). This allows for
 * fine-grained regions to be specified for merging.
 * 
 * # Implementation Structure
 * 
 * The main part of the new functionality is implemented in the class
 * klee::MergeHandler. The Special Function Handler generates an instance of
 * this class every time a state runs into a klee_open_merge() call.
 * 
 * This instance is appended to a `std::vector<klee::ref<klee::MergeHandler>>`
 * in the ExecutionState that passed the merge open point. This stack is also
 * copied during forks. We use a stack instead of a single instance to support
 * nested merge regions.
 * 
 * Once a state runs into a `klee_close_merge()`, the Special Function Handler
 * notifies the top klee::MergeHandler in the state's stack, pauses the state
 * from scheduling, and tries to merge it with all other states that already
 * arrived at the same close merge point. This top instance is then popped from
 * the stack, resulting in a decrease of the ref count of the
 * klee::MergeHandler.
 * 
 * Since the only references to this MergeHandler are in the stacks of
 * the ExecutionStates currently in the merging region, once the ref count
 * reaches zero, every state which ran into the same `klee_open_merge()` is now
 * paused and waiting to be merged. The destructor of the MergeHandler
 * then continues the scheduling of the corresponding paused states.
*/

#ifndef KLEE_MERGEHANDLER_H
#define KLEE_MERGEHANDLER_H

#include <vector>
#include <map>
#include <stdint.h>
#include "llvm/Support/CommandLine.h"

namespace llvm {
class Instruction;
}

namespace klee {
extern llvm::cl::opt<bool> UseMerge;

extern llvm::cl::opt<bool> DebugLogMerge;

class Executor;
class ExecutionState;

/// @brief Represents one `klee_open_merge()` call. 
/// Handles merging of states that branched from it
class MergeHandler {
private:
  Executor *executor;

  /// @brief Number of states that are tracked by this MergeHandler, that ran
  /// into a relevant klee_close_merge
  unsigned closedStateCount;

  /// @brief Mapping the different 'klee_close_merge' calls to the states that ran into
  /// them
  std::map<llvm::Instruction *, std::vector<ExecutionState *> >
      reachedMergeClose;

public:

  /// @brief Called when a state runs into a 'klee_close_merge()' call
  void addClosedState(ExecutionState *es, llvm::Instruction *mp);

  /// @brief True, if any states have run into 'klee_close_merge()' and have
  /// not been released yet
  bool hasMergedStates();
  
  /// @brief Immediately release the merged states that have run into a
  /// 'klee_merge_close()'
  void releaseStates();

  /// @brief Required by klee::ref objects
  unsigned refCount;


  MergeHandler(Executor *_executor);
  ~MergeHandler();
};
}

#endif	/* KLEE_MERGEHANDLER_H */
