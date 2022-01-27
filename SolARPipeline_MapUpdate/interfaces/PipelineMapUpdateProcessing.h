/**
 * @copyright Copyright (c) 2020 B-com http://www.b-com.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PIPELINEMAPUPDATEPROCESSING_H
#define PIPELINEMAPUPDATEPROCESSING_H

#if _WIN32
#ifdef SolARPipelineMapUpdate_API_DLLEXPORT
#define SOLARPIPELINE_MAPUPDATE_EXPORT_API __declspec(dllexport)
#else // SolARPipeline_MapUpdate_API_DLLEXPORT
#define SOLARPIPELINE_MAPUPDATE_EXPORT_API __declspec(dllimport)
#endif // SolARPipeline_MapUpdate_API_DLLEXPORT
#else //_WIN32
#define SOLARPIPELINE_MAPUPDATE_EXPORT_API
#endif //_WIN32

#include "xpcf/component/ConfigurableBase.h"
#include "xpcf/threading/DropBuffer.h"
#include "xpcf/threading/BaseTask.h"
#include <mutex>

#include "api/pipeline/IMapUpdatePipeline.h"
#include "api/storage/IMapManager.h"
#include "api/loop/IOverlapDetector.h"
#include "api/solver/map/IMapFusion.h"
#include "api/solver/map/IMapUpdate.h"
#include "api/solver/map/IBundler.h"

namespace SolAR {
namespace PIPELINES {

    /**
     * @class PipelineMapUpdateProcessing
     * @brief Implementation of a map update pipeline
     * <TT>UUID: 7eb960b3-862f-4921-bd7c-a67222d0bf82</TT>
     *
     * @SolARComponentInjectablesBegin
	 * @SolARComponentInjectable{SolAR::api::storage::IMapManager}
	 * @SolARComponentInjectable{SolAR::api::loop::IOverlapDetector}
	 * @SolARComponentInjectable{SolAR::api::solver::map::IMapFusion}
	 * @SolARComponentInjectable{SolAR::api::solver::map::IMapUpdate}
	 * @SolARComponentInjectable{SolAR::api::solver::map::IBundler}
     * @SolARComponentInjectablesEnd
     *
     */

    class SOLARPIPELINE_MAPUPDATE_EXPORT_API PipelineMapUpdateProcessing : public org::bcom::xpcf::ConfigurableBase,
            public api::pipeline::IMapUpdatePipeline
    {
    public:
		PipelineMapUpdateProcessing();
        ~PipelineMapUpdateProcessing() override;

        void unloadComponent() override final {}

        /// @brief Initialization of the pipeline
        /// @return FrameworkReturnCode::_SUCCESS if the init succeed, else
        FrameworkReturnCode init() override;

        /// @brief Set the camera parameters
        /// @param[in] cameraParams: the camera parameters (its resolution and its focal)
        /// @return FrameworkReturnCode::_SUCCESS if the camera parameters are correctly set, else FrameworkReturnCode::_ERROR_
        FrameworkReturnCode setCameraParameters(const datastructure::CameraParameters & cameraParams) override;

        /// @brief Start the pipeline
        /// @return FrameworkReturnCode::_SUCCESS if the stard succeed, else FrameworkReturnCode::_ERROR_
        FrameworkReturnCode start() override;

        /// @brief Stop the pipeline.
        /// @return FrameworkReturnCode::_SUCCESS if the stop succeed, else FrameworkReturnCode::_ERROR_
        FrameworkReturnCode stop() override;

		/// @brief Request to the map update pipeline to update the global map from a local map
        /// @param[in] map: the input local map to process
		/// @return FrameworkReturnCode::_SUCCESS if the data are ready to be processed, else FrameworkReturnCode::_ERROR_
		FrameworkReturnCode mapUpdateRequest(const SRef<datastructure::Map> map) override;

        /// @brief Request to the map update pipeline to get the global map
        /// @param[out] map: the output global map
        /// @return FrameworkReturnCode::_SUCCESS if the global map is available, else FrameworkReturnCode::_ERROR_
        FrameworkReturnCode getMapRequest(SRef<SolAR::datastructure::Map> & map) const override;

		/// @brief Request to the map update pipeline to get a submap based on a query frame.
		/// @param[in] frame the query frame
		/// @param[out] map the output submap
		/// @return FrameworkReturnCode::_SUCCESS if submap is found, else FrameworkReturnCode::_ERROR_
		FrameworkReturnCode getSubmapRequest(const SRef<SolAR::datastructure::Frame> frame,
											 SRef<SolAR::datastructure::Map> & map) const override;

	private:
		/// @brief method that implementes the full maping processing
		void processMapUpdate();

    private:
        bool										m_init = false;
        bool                                        m_setCameraParameters = false;
        bool										m_startedOK = false;
		datastructure::CameraParameters				m_cameraParams;
		mutable std::mutex							m_mutex;
		int											m_nbKeyframeSubmap = 100;
		// Injected components
		SRef<api::storage::IMapManager>				m_mapManager;
		SRef<api::loop::IOverlapDetector>			m_mapOverlapDetector;
		SRef<api::solver::map::IMapFusion>			m_mapFusion;
		SRef<api::solver::map::IMapUpdate>			m_mapUpdate;
		SRef<api::solver::map::IBundler>			m_bundler;
		SRef<api::reloc::IKeyframeRetriever>        m_kfRetriever;
        
        // Delegate task dedicated to asynchronous map update processing
        xpcf::DelegateTask *						m_mapUpdateTask;

        // Drop buffer containing maps sent by client
        xpcf::SharedFifo<SRef<datastructure::Map>>	m_inputMapBuffer;
    };

}
}

XPCF_DEFINE_COMPONENT_TRAITS(SolAR::PIPELINES::PipelineMapUpdateProcessing,
                             "7eb960b3-862f-4921-bd7c-a67222d0bf82",
                             "PipelineMapUpdateProcessing",
                             "PipelineMapUpdateProcessing implements api::pipeline::IMapUpdatePipeline interface");

#endif // PIPELINEMAPUPDATEPROCESSING_H
