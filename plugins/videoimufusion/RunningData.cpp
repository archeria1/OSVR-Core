/** @file
    @brief Implementation of RunningData methods that compile quickly.

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Internal Includes
#include "RunningData.h"

// Library/third-party includes
#ifdef OSVR_FPE
#include <FPExceptionEnabler.h>
#endif

// Standard includes
// - none

static const double InitialStateError[] = {1.,   1.,   1.,   1.,   1.,   1.,
                                           100., 100., 100., 100., 100., 100.};
static const double IMUError = 1.0E-5;
static const double IMUErrorVector[] = {IMUError, IMUError * 5., IMUError};
static const double CameraOriError = 1.0E-2;
static const double CameraOrientationError[] = {CameraOriError, CameraOriError,
                                                CameraOriError};
static const double CameraPosError = 3.0E-3;
static const double CameraPositionError[] = {CameraPosError, CameraPosError,
                                             CameraPosError * 0.1};

using osvr::kalman::types::Vector;
namespace ei = osvr::util::eigen_interop;

VideoIMUFusion::RunningData::RunningData(
    Eigen::Isometry3d const &cTr, OSVR_OrientationState const &initialIMU,
    OSVR_PoseState const &initialVideo, OSVR_TimeValue const &lastTS)
    : m_filter(),
      m_imuMeas(ei::map(initialIMU), Vector<3>::Map(IMUErrorVector).eval()),
      m_cameraMeas(Vector<3>::Zero(), Eigen::Quaterniond::Identity(),
                   Vector<3>::Map(CameraPositionError).asDiagonal(),
                   Vector<3>::Map(CameraOrientationError)),
      m_cTr(cTr), m_last(lastTS) {

#ifdef OSVR_FPE
    FPExceptionEnabler fpe;
#endif
    Eigen::Isometry3d roomPose = takeCameraPoseToRoom(initialVideo);
    osvr::kalman::types::DimVector<FilterState> initialState =
        osvr::kalman::types::DimVector<FilterState>::Zero();
    using namespace osvr::kalman::pose_externalized_rotation;

    position(initialState) = roomPose.translation();
    m_filter.state().setStateVector(initialState);
    m_filter.state().setQuaternion(Eigen::Quaterniond(roomPose.rotation()));
    m_filter.state().setErrorCovariance(
        Vector<12>(InitialStateError).asDiagonal());

    Vector<6> noiseAutocorrelation;
    noiseAutocorrelation.head<3>() = Vector<3>::Constant(0.00001);
    noiseAutocorrelation.tail<3>() = Vector<3>::Constant(0.0001);
    m_filter.processModel().setNoiseAutocorrelation(noiseAutocorrelation);
}
