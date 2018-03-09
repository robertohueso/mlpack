/**
 * @file kde_rules_impl.hpp
 * @author Roberto Hueso (robertohueso96@gmail.com)
 *
 * Implementation of rules for Kernel Density Estimation with generic trees.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */

#ifndef MLPACK_METHODS_KDE_RULES_IMPL_HPP
#define MLPACK_METHODS_KDE_RULES_IMPL_HPP

// In case it hasn't been included yet.
#include "kde_rules.hpp"

namespace mlpack {
namespace kde {

template<typename MetricType, typename KernelType, typename TreeType>
KDERules<MetricType, KernelType, TreeType>::KDERules(
    const arma::mat& referenceSet,
    const arma::mat& querySet,
    arma::vec& densities,
    const double error,
    MetricType& metric,
    const KernelType& kernel) :
    referenceSet(referenceSet),
    querySet(querySet),
    densities(densities),
    error(error),
    metric(metric),
    kernel(kernel),
    lastQueryIndex(querySet.n_cols),
    lastReferenceIndex(referenceSet.n_cols),
    baseCases(0),
    scores(0)
{
  // Nothing to do.
}

//! The base case.
template<typename MetricType, typename KernelType,typename TreeType>
inline force_inline
double KDERules<MetricType, KernelType, TreeType>::BaseCase(
    const size_t queryIndex,
    const size_t referenceIndex)
{
  double distance = metric.Evaluate(querySet.col(queryIndex),
                                    referenceSet.col(referenceIndex));
  densities(queryIndex) += kernel.Evaluate(distance);
  
  ++baseCases;
  lastQueryIndex = queryIndex;
  lastReferenceIndex = referenceIndex;
  return distance;
}

//! Single-tree scoring function.
template<typename MetricType, typename KernelType,typename TreeType>
double KDERules<MetricType, KernelType, TreeType>::
Score(const size_t /* queryIndex */, TreeType& /* referenceNode */)
{
  ++scores;
  traversalInfo.LastScore() = 0.0;
  return 0.0;
}

template<typename MetricType, typename KernelType, typename TreeType>
double KDERules<MetricType, KernelType, TreeType>::Rescore(
    const size_t /* queryIndex */,
    TreeType& /* referenceNode */,
    const double oldScore) const
{
  // If it's pruned it continues to be pruned.
  return oldScore;
}

//! Double-tree scoring function.
template<typename MetricType, typename KernelType,typename TreeType>
double KDERules<MetricType, KernelType, TreeType>::
Score(TreeType& queryNode, TreeType& referenceNode)
{
  double score, bound;
  bound = kernel.Evaluate(queryNode.MinDistance(referenceNode)) -
          kernel.Evaluate(queryNode.MaxDistance(referenceNode));

  if (bound <= (error / referenceSet.n_cols))
  {
    //std::cout << referenceNode.Point(0) << "\n";
    arma::vec center = arma::vec();
    referenceNode.Center(center);
    for (size_t i = 0; i < queryNode.NumDescendants(); ++i)
    {
      densities(queryNode.Point(i)) +=
        referenceNode.NumDescendants() *
        kernel.Evaluate(metric.Evaluate(querySet.col(queryNode.Point(i)),
                                        center));
    }
    score = DBL_MAX;
  }
  else
  {
    score = queryNode.MinDistance(referenceNode);
  }

  ++scores;
  traversalInfo.LastQueryNode() = &queryNode;
  traversalInfo.LastReferenceNode() = &referenceNode;
  traversalInfo.LastScore() = score;
  return score;
}

//! Double-tree
template<typename MetricType, typename KernelType,typename TreeType>
double KDERules<MetricType, KernelType, TreeType>::
Rescore(TreeType& /*queryNode*/,
        TreeType& /*referenceNode*/,
        const double oldScore) const
{
  return oldScore;
}

} // namespace kde
} // namespace mlpack

#endif