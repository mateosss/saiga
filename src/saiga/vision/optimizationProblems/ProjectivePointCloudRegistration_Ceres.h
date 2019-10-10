/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once
#include "ProjectivePointCloudRegistration.h"
#include "ceres/autodiff_cost_function.h"
#include "ceres/loss_function.h"
#include "saiga/vision/ceres/local_parameterization_sim3.h"
#include "saiga/vision/ceres/CeresHelper.h"
namespace Saiga
{

template <typename ScalarType, template <typename> class TransformationType, bool invert = false>
struct ProjectivePointCloudRegistrationCeresCost
{
    // Helper function to simplify the "add residual" part for creating ceres problems
    using CostType = ProjectivePointCloudRegistrationCeresCost;
    // Note: The first number is the number of residuals
    //       The following number sthe size of the residual blocks (without local parametrization)
    using CostFunctionType = ceres::AutoDiffCostFunction<CostType, 2, 7>;
    template <typename... Types>
    static CostFunctionType* create(Types... args)
    {
        return new CostFunctionType(new CostType(args...));
    }

    template <typename T>
    bool operator()(const T* const _extrinsics, T* _residuals) const
    {
        Eigen::Map<TransformationType<T> const> const se3(_extrinsics);
        Eigen::Map<Eigen::Matrix<T, 2, 1>> residual(_residuals);

        Eigen::Matrix<T, 3, 1> wp       = _wp.cast<T>();
        auto intr                       = _intr.cast<T>();
        Eigen::Matrix<T, 2, 1> observed = _observed.cast<T>();
        Eigen::Matrix<T, 3, 1> pc;
        if (invert)
            pc = se3.inverse() * wp;
        else
            pc = se3 * wp;
        Eigen::Matrix<T, 2, 1> proj = intr.project(pc);
        residual                    = T(weight) * (observed - proj);
        return true;
    }

    ProjectivePointCloudRegistrationCeresCost(Intrinsics4 intr, Vec2 observed, Vec3 wp, double weight = 1)
        : _intr(intr), _observed(observed), _wp(wp), weight(weight)
    {
    }

    Intrinsics4 _intr;
    Vec2 _observed;
    Vec3 _wp;
    double weight;
};


template <typename ScalarType, template <typename> class TransformationType>
void optimize(ProjectivePointCloudRegistration<TransformationType<ScalarType>>& scene)
{
    ceres::Problem::Options problemOptions;
    ceres::Problem problem(problemOptions);

    double huber = sqrt(scene.chi2Threshold);

    auto camera_parameterization = new Saiga::test::LocalParameterizationSim3<false>;
    problem.AddParameterBlock(scene.T.data(), 7, camera_parameterization);



    for (auto& e : scene.obs1)
    {
        if (e.wp == -1) continue;
        auto& wp                  = scene.points2[e.wp];
        auto* cost                = ProjectivePointCloudRegistrationCeresCost<ScalarType,TransformationType,true>::create(scene.K, e.imagePoint, wp, e.weight);
        ceres::LossFunction* loss = nullptr;
        if (huber > 0) loss = new ceres::HuberLoss(huber);
        problem.AddResidualBlock(cost, loss, scene.T.data());
    }

    for (auto& e : scene.obs2)
    {
        if (e.wp == -1) continue;
        auto& wp                  = scene.points1[e.wp];
        auto* cost                = ProjectivePointCloudRegistrationCeresCost<ScalarType,TransformationType,false>::create(scene.K, e.imagePoint, wp, e.weight);
        ceres::LossFunction* loss = nullptr;
        if (huber > 0) loss = new ceres::HuberLoss(huber);
        problem.AddResidualBlock(cost, loss, scene.T.data());
    }

    ceres::Solver::Options ceres_options;
    //    ceres_options.minimizer_progress_to_stdout = true;
    OptimizationResults result = ceres_solve(ceres_options, problem);
}

}  // namespace Saiga
