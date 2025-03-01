/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef MLIR_HLO_GML_ST_TRANSFORMS_FUSION_FUSION_H
#define MLIR_HLO_GML_ST_TRANSFORMS_FUSION_FUSION_H

#include "gml_st/transforms/peeling/peeling.h"
#include "gml_st/transforms/tiling/tiling.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/PatternMatch.h"

namespace mlir {
namespace gml_st {

// Create fused operation based on the specificed subset. The result is
// equivalent to the given `tensor.extract_slice` op.
FailureOr<Value> createFusedOp(PatternRewriter &rewriter,
                               tensor::ExtractSliceOp materializeOp);

// Fuses an op into `tensor.extract_slice` and performs the necessary updates to
// the surrounding loop if any.
FailureOr<Operation *> fuse(PatternRewriter &rewriter,
                            tensor::ExtractSliceOp materializeOp);

// Finds `tensor.extract_slice` ops in the block and fuses ops into them.
// Verifies that fusion candidate doesn't have any uses except the one
// `tensor.extract_slice` in the block to avoid exponential code growth.
void fuseGreedily(PatternRewriter &rewriter, Block &block,
                  llvm::function_ref<bool(Operation *)> filterFn = nullptr);

/// Populate fusion patterns.
void populateFusionPatterns(
    MLIRContext *ctx,
    function_ref<LogicalResult(tensor::ExtractSliceOp)> filterFn,
    RewritePatternSet *patterns);

struct FusionCluster {
  DenseSet<Operation *> operations;
  Operation *root;
};

// Find a cluster of operations that can be tiled and fused together around
// the root op. We want to fuse output of the fusion op with elementwise ops. In
// general case a cluster is a tree that can have multiple leaf-node ops,
// e.g. map(op, map(op)).
// First element of the cluster is always the root for tiling.
FusionCluster findMapFusionCluster(Operation *op);

// Fuses linalg.fill that is used in init argument of the op.
LogicalResult fuseOutputFill(PatternRewriter &rewriter, Operation *op);

// Tiles the op to gml_st.parallel and fuses greedily according to the filter.
FailureOr<ParallelOp> tileUsingGmlStParallelAndFuseGreedily(
    PatternRewriter &rewriter, Operation *op,
    const mlir::gml_st::TilingOptions &opts, StringRef label,
    llvm::function_ref<bool(Operation *)> fuseFilterFn);

// Tiles the op to scf.for and fuses greedily according to the filter.
FailureOr<scf::SCFTilingResult> tileUsingSCFForOpAndFuseGreedily(
    PatternRewriter &rewriter, Operation *op, const scf::SCFTilingOptions &opts,
    StringRef label, llvm::function_ref<bool(Operation *)> fuseFilterFn);

// Tiles the op to 1 for all dimensions and fuses greedily according to the
// filter function.
LogicalResult tilePeeledOpsToScalars(
    PatternRewriter &rewriter, const GmlStPeelingResult &peelingResult,
    StringRef label, llvm::function_ref<bool(Operation *)> fuseFilterFn);

}  // namespace gml_st
}  // namespace mlir

#endif  // MLIR_HLO_GML_ST_TRANSFORMS_FUSION_FUSION_H
