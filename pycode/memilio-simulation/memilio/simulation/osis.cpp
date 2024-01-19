/*
* Copyright (C) 2020-2024 MEmilio
*
* Authors: Martin Siggel, Daniel Abele, Martin J. Kuehn, Jan Kleinert
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

#include "pybind_util.h"
#include "utils/index.h"
#include "utils/custom_index_array.h"
#include "utils/parameter_set.h"
#include "compartments/simulation.h"
#include "compartments/compartmentalmodel.h"
#include "epidemiology/populations.h"
#include "ode_sis/model.h"
#include "ode_sis/infection_state.h"
#include "memilio/data/analyze_result.h"

namespace py = pybind11;

namespace pymio
{
//specialization of pretty_name
template <>
std::string pretty_name<mio::osis::InfectionState>()
{
    return "InfectionState";
}

} // namespace pymio

PYBIND11_MODULE(_simulation_osis, m)
{
    m.def("interpolate_simulation_result",
          static_cast<mio::TimeSeries<double> (*)(const mio::TimeSeries<double>&, const double)>(
              &mio::interpolate_simulation_result),
          py::arg("ts"), py::arg("abs_tol") = 1e-14);

    m.def("interpolate_simulation_result",
          static_cast<mio::TimeSeries<double> (*)(const mio::TimeSeries<double>&, const std::vector<double>&)>(
              &mio::interpolate_simulation_result),
          py::arg("ts"), py::arg("interpolation_times"));

    m.def("interpolate_ensemble_results", &mio::interpolate_ensemble_results<mio::TimeSeries<double>>);

    pymio::iterable_enum<mio::osis::InfectionState>(m, "InfectionState")
        .value("Susceptible", mio::osis::InfectionState::Susceptible)
        .value("Infected", mio::osis::InfectionState::Infected)
        /*.value("Recovered", mio::osi::InfectionState::Recovered)*/;

    pymio::bind_ParameterSet<mio::osis::ParametersBase>(m, "ParametersBase");

    py::class_<mio::osis::Parameters, mio::osis::ParametersBase>(m, "Parameters")
        .def(py::init<>())
        .def("check_constraints", &mio::osis::Parameters::check_constraints);

    using Populations = mio::Populations<mio::osis::InfectionState>;
    pymio::bind_Population(m, "Population", mio::Tag<mio::osis::Model::Populations>{});
    pymio::bind_CompartmentalModel<mio::osis::InfectionState, Populations, mio::osis::Parameters>(m, "ModelBase");
    py::class_<mio::osis::Model,
               mio::CompartmentalModel<mio::osis::InfectionState, Populations, mio::osis::Parameters>>(m, "Model")
        .def(py::init<>());

    m.def(
        "simulate",
        [](double t0, double tmax, double dt, const mio::osis::Model& model) {
            return mio::simulate(t0, tmax, dt, model);
        },
        "Simulates a osis from t0 to tmax.", py::arg("t0"), py::arg("tmax"), py::arg("dt"), py::arg("model"));

    m.attr("__version__") = "dev";
}
