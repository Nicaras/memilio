/* 
* Copyright (C) 2020-2024 MEmilio
*
* Authors: Wadim Koslow, Daniel Abele, Martin J. Kühn
*
* Contact: Martin J. Kuehn <Martin.Kuehn@DLR.de>
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
#ifndef ODESECIRVVS_ANALYZE_RESULT_H
#define ODESECIRVVS_ANALYZE_RESULT_H

#include "ode_secirvvs/model.h"
#include "memilio/data/analyze_result.h"

#include <functional>
#include <vector>

namespace mio
{
namespace osecirvvs
{
/**
    * @brief computes the p percentile of the parameters for each node.
    * @param ensemble_result graph of multiple simulation runs
    * @param p percentile value in open interval (0, 1)
    * @return p percentile of the parameters over all runs
    */
template <class Model>
std::vector<Model> ensemble_params_percentile(const std::vector<std::vector<Model>>& ensemble_params, double p)
{
    assert(p > 0.0 && p < 1.0 && "Invalid percentile value.");

    auto num_runs   = ensemble_params.size();
    auto num_nodes  = ensemble_params[0].size();
    auto num_groups = (size_t)ensemble_params[0][0].parameters.get_num_groups();
    auto num_days =
        ensemble_params[0][0].parameters.template get<DailyFirstVaccination>().template size<mio::SimulationDay>();

    std::vector<double> single_element_ensemble(num_runs);

    // lambda function that calculates the percentile of a single parameter
    std::vector<Model> percentile(num_nodes, Model((int)num_groups));
    auto param_percentil = [&ensemble_params, p, num_runs, &percentile](auto n, auto get_param) mutable {
        std::vector<double> single_element(num_runs);
        for (size_t run = 0; run < num_runs; run++) {
            auto const& params  = ensemble_params[run][n];
            single_element[run] = get_param(params);
        }
        std::sort(single_element.begin(), single_element.end());
        auto& new_params = get_param(percentile[n]);
        new_params       = single_element[static_cast<size_t>(num_runs * p)];
    };

    for (size_t node = 0; node < num_nodes; node++) {
        percentile[node].parameters.template get<DailyFirstVaccination>().resize(num_days);
        percentile[node].parameters.template get<DailyFullVaccination>().resize(num_days);

        for (auto i = AgeGroup(0); i < AgeGroup(num_groups); i++) {
            //Population
            for (auto compart = Index<InfectionState>(0); compart < InfectionState::Count; ++compart) {
                param_percentil(
                    node, [ compart, i ](auto&& model) -> auto& {
                        return model.populations[{i, compart}];
                    });
            }
            // times
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<IncubationTime>()[i]; });
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<SerialInterval>()[i]; });
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<TimeInfectedSymptoms>()[i]; });
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<TimeInfectedSevere>()[i]; });
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<TimeInfectedCritical>()[i]; });
            //probs
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<TransmissionProbabilityOnContact>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<RelativeTransmissionNoSymptoms>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<RiskOfInfectionFromSymptomatic>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<MaxRiskOfInfectionFromSymptomatic>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<RecoveredPerInfectedNoSymptoms>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<SeverePerInfectedSymptoms>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<CriticalPerSevere>()[i]; });
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<DeathsPerCritical>()[i]; });
            //vaccinations
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<ReducExposedPartialImmunity>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<ReducExposedImprovedImmunity>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<ReducInfectedSymptomsPartialImmunity>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<ReducInfectedSymptomsImprovedImmunity>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<ReducInfectedSevereCriticalDeadPartialImmunity>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<ReducInfectedSevereCriticalDeadImprovedImmunity>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<ReducTimeInfectedMild>()[i]; });
            param_percentil(
                node, [i](auto&& model) -> auto& { return model.parameters.template get<VaccinationGap>()[i]; });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<DaysUntilEffectivePartialImmunity>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<DaysUntilEffectiveImprovedImmunity>()[i];
                });

            for (auto day = SimulationDay(0); day < num_days; ++day) {
                param_percentil(
                    node, [ i, day ](auto&& model) -> auto& {
                        return model.parameters.template get<DailyFirstVaccination>()[{i, day}];
                    });
                param_percentil(
                    node, [ i, day ](auto&& model) -> auto& {
                        return model.parameters.template get<DailyFullVaccination>()[{i, day}];
                    });
            }
            //virus variants
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<BaseInfectiousnessB161>()[i];
                });
            param_percentil(
                node, [i](auto&& model) -> auto& {
                    return model.parameters.template get<BaseInfectiousnessB117>()[i];
                });
        }
        // group independent params
        param_percentil(
            node, [](auto&& model) -> auto& { return model.parameters.template get<Seasonality>(); });
        param_percentil(
            node, [](auto&& model) -> auto& { return model.parameters.template get<TestAndTraceCapacity>(); });
        param_percentil(
            node, [](auto&& model) -> auto& { return model.parameters.template get<ICUCapacity>(); });

        for (size_t run = 0; run < num_runs; run++) {

            auto const& params = ensemble_params[run][node];
            single_element_ensemble[run] =
                params.parameters.template get<ICUCapacity>() * params.populations.get_total();
        }
        std::sort(single_element_ensemble.begin(), single_element_ensemble.end());
        percentile[node].parameters.template set<ICUCapacity>(
            single_element_ensemble[static_cast<size_t>(num_runs * p)]);
    }
    return percentile;
}

} // namespace osecirvvs
} // namespace mio

#endif //ODESECIRVVS
