/**
 * @file kde_main.cpp
 * @author Roberto Hueso (robertohueso96@gmail.com)
 *
 * Executable for running Kernel Density Estimation.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */

#include <mlpack/prereqs.hpp>
#include <mlpack/core/util/cli.hpp>
#include <mlpack/core/util/mlpack_main.hpp>
#include <mlpack/core/tree/statistic.hpp>

#include "kde.hpp"
#include "kde_model.hpp"

using namespace mlpack;
using namespace mlpack::kde;
using namespace mlpack::util;
using namespace std;

// Define parameters for the executable.
PROGRAM_INFO("Kernel Density Estimation",
    "This program performs a Kernel Density Estimation. KDE is a "
    "non-parametric way of estimating probability density function. "
    "For each query point the program will estimate its probability density "
    "by applying a kernel function to each reference point. Computational "
    " complexity is O(n^2) but it is optimized by making use of dual-trees. "
    "\n\n"
    "For example, the following will run KDE using the points in "
    "reference_set.csv and query_set.csv. It will apply an Epanechnikov kernel "
    "with a 0.2 bandwidth to each reference point and use a KD-Tree for the "
    "dual-tree optimization. The result will be stored in a densities.csv file "
    "with a maximum error of 5%"
    "\n\n"
    "$ kde --reference reference_set.csv --query query_set.csv --bandwidth 0.2 "
    "--kernel epanechnikov --tree kd-tree --rel_error 0.05 --output "
    "densities.csv"
    "\n\n"
    "Dual-tree optimization allows to avoid lots of barely relevant "
    "calculations (as kernel function values decrease with distance) if you "
    "can afford a little error (you can define how much is the maximum you are "
    "willing to afford) over the final result. This program runs using an "
    "Euclidean metric. If no output file is specified then it will output the "
    "result to standard output.");

// Required options.
PARAM_MATRIX_IN("reference", "Input dataset to KDE on.", "r");
PARAM_MATRIX_IN_REQ("query", "Query dataset to KDE on.", "q");
PARAM_DOUBLE_IN("bandwidth", "Bandwidth of the kernel", "b", 1.0);

// Load or save models.
PARAM_MODEL_IN(KDEModel,
               "input_model",
               "File containing pre-trained KDE model.",
               "m");
PARAM_MODEL_OUT(KDEModel,
                "output_model",
                "If specified, the KDE model will be saved to the given file.",
                "M");

// Configuration options
PARAM_STRING_IN("kernel", "Kernel to use for the estimation"
    "('gaussian', 'epanechnikov').", "k", "gaussian");
PARAM_STRING_IN("tree", "Tree to use for the estimation"
    "('kd-tree', 'ball-tree').", "t", "kd-tree");
PARAM_DOUBLE_IN("rel_error",
                "Relative error tolerance for the result",
                "e",
                1e-8);
PARAM_DOUBLE_IN("abs_error",
                "Relative error tolerance for the result",
                "E",
                0.0);
PARAM_FLAG("breadth_first", "Use breadth-first traversal instead of depth"
           "first.", "w");
// Maybe in the future it could be interesting to implement different metrics.

// Output options.
PARAM_MATRIX_OUT("output", "Matrix to store output estimations.",
    "o");

static void mlpackMain()
{
  const size_t output_precision = 40;
  // Get some parameters.
  arma::mat query = std::move(CLI::GetParam<arma::mat>("query"));
  const double bandwidth = CLI::GetParam<double>("bandwidth");
  const std::string kernelStr = CLI::GetParam<std::string>("kernel");
  const std::string treeStr = CLI::GetParam<std::string>("tree");
  const double relError = CLI::GetParam<double>("rel_error");
  const double absError = CLI::GetParam<double>("abs_error");
  const bool breadthFirst = CLI::GetParam<bool>("breadth_first");
  // Initialize results vector.
  arma::vec estimations;

  // You can only specify reference data or a pre-trained model.
  RequireOnlyOnePassed({ "reference", "input_model" }, true);
  ReportIgnoredParam({{ "input_model", true }}, "tree");
  ReportIgnoredParam({{ "input_model", true }}, "kernel");
  ReportIgnoredParam({{ "input_model", true }}, "rel_error");
  ReportIgnoredParam({{ "input_model", true }}, "abs_error");
  ReportIgnoredParam({{ "input_model", true }}, "breadth_first");

  KDEModel* kde;

  if (CLI::HasParam("reference"))
  {
    arma::mat reference = std::move(CLI::GetParam<arma::mat>("reference"));

    kde = new KDEModel();
    // Set parameters
    kde->Bandwidth() = bandwidth;
    kde->RelativeError() = relError;
    kde->AbsoluteError() = absError;
    kde->BreadthFirst() = breadthFirst;

    // Set KernelType
    if (kernelStr == "gaussian")
      kde->KernelType() = KDEModel::GAUSSIAN_KERNEL;
    else if (kernelStr == "epanechnikov")
      kde->KernelType() = KDEModel::EPANECHNIKOV_KERNEL;
    else
      Log::Fatal << "Input kernel is not valid or not supported yet."
                 << std::endl;

    // Set TreeType
    if (treeStr == "kd-tree")
      kde->TreeType() = KDEModel::KD_TREE;
    else if (treeStr == "ball-tree")
      kde->TreeType() = KDEModel::BALL_TREE;
    else
      Log::Fatal << "Input tree is not valid or not supported yet."
                 << std::endl;

    // Build model
    kde->BuildModel(std::move(reference));
  }
  else
  {
    // Load model
    kde = CLI::GetParam<KDEModel*>("input_model");
  }

  kde->Evaluate(std::move(query), estimations);

  // Output estimations to file if defined.
  if (CLI::HasParam("output"))
  {
    CLI::GetParam<arma::mat>("output") = std::move(estimations);
  }
  else
  {
    std::cout.precision(output_precision);
    estimations.raw_print(std::cout);
  }

  // Save model.
  if (CLI::HasParam("output_model"))
    CLI::GetParam<KDEModel*>("output_model") = kde;
}