/* 
* Copyright (C) 2020-2024 MEmilio
*
* Authors: Rene Schmieding, Henrik Zunker
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
#ifndef MIO_COMPARTMENTS_SIMULATION_H_
#define MIO_COMPARTMENTS_SIMULATION_H_

#include "memilio/compartments/flow_model.h"
#include "memilio/compartments/simulation.h"

namespace mio
{

template <class M>
class FlowSimulation : public Simulation<M>
{
    static_assert(is_flow_model<M>::value, "Template parameter must be a flow model.");

public:
    using Model = M;
    using Base  = Simulation<M>;

    /**
     * @brief Set up the simulation with an ODE solver.
     * @param[in] model An instance of a compartmental model.
     * @param[in] t0 Start time.
     * @param[in] dt Initial step size of integration.
     */
    FlowSimulation(Model const& model, double t0 = 0., double dt = 0.1)
        : Base(model, t0, dt)
        , m_pop(model.get_initial_values().size())
        , m_flow_result(t0, model.get_initial_flows())
    {
    }

    /**
     * @brief Advance the simulation to tmax.
     * tmax must be greater than get_result().get_last_time_point().
     * @param[in] tmax Next stopping time of the simulation.
     */
    Eigen::Ref<Eigen::VectorXd> advance(double tmax)
    {
        // the derivfunktion (i.e. the lambda passed to m_integrator.advance below) requires that there are at least
        // as many entries in m_flow_result as in Base::m_result
        assert(m_flow_result.get_num_time_points() == this->get_result().get_num_time_points());
        auto result = this->get_ode_integrator().advance(
            [this](auto&& flows, auto&& t, auto&& dflows_dt) {
                auto pop_result = this->get_result();
                auto model      = this->get_model();
                // compute current population
                //   flows contains the accumulated outflows of each compartment for each target compartment at time t.
                //   Using that the ODEs are linear expressions of the flows, get_derivatives can compute the total change
                //   in population from t0 to t.
                //   To incorporate external changes to the last values of pop_result (e.g. by applying mobility), we only
                //   calculate the change in population starting from the last available time point in m_result, instead
                //   of starting at t0. To do that, the following difference of flows is used.
                model.get_derivatives(flows - m_flow_result.get_value(pop_result.get_num_time_points() - 1),
                                      m_pop); // note: overwrites values in pop
                //   add the "initial" value of the ODEs (using last available time point in pop_result)
                //     If no changes were made to the last value in m_result outside of FlowSimulation, the following
                //     line computes the same as `model.get_derivatives(flows, x); x += model.get_initial_values();`.
                m_pop += pop_result.get_last_value();
                // compute the current change in flows with respect to the current population
                dflows_dt.setZero();
                model.get_flows(m_pop, m_pop, t, dflows_dt); // this result is used by the integrator
            },
            tmax, this->get_dt(), m_flow_result);
        compute_population_results();
        return result;
    }

    /**
     * @brief Returns the simulation result describing the transitions between compartments for each time step.
     *
     * Which flows are used by the model is defined by the Flows template argument for the FlowModel.
     * To get the correct index for the flow between two compartments use FlowModel::get_flat_flow_index.
     *
     * @return A TimeSeries to represent a numerical solution for the flows in the model. 
     * For each simulated time step, the TimeSeries contains the value of each flow. 
     * @{
     */
    TimeSeries<ScalarType>& get_flows()
    {
        return m_flow_result;
    }

    const TimeSeries<ScalarType>& get_flows() const
    {
        return m_flow_result;
    }
    /** @} */

private:
    /**
     * @brief Computes the distribution of the Population to the InfectionState%s based on the simulated flows.
     * Uses the same method as the DerivFunction used in advance to compute the population given the flows and initial
     * values. Adds time points to Base::m_result until it has the same number of time points as m_flow_result.
     * Does not recalculate older values.
     */
    void compute_population_results()
    {
        const auto& flows = get_flows();
        const auto& model = this->get_model();
        auto& result      = this->get_result();
        // take the last time point as base result (instead of the initial results), so that we use external changes
        const size_t last_tp = result.get_num_time_points() - 1;
        // calculate new time points
        for (Eigen::Index i = result.get_num_time_points(); i < flows.get_num_time_points(); i++) {
            result.add_time_point(flows.get_time(i));
            model.get_derivatives(flows.get_value(i) - flows.get_value(last_tp), result.get_value(i));
            result.get_value(i) += result.get_value(last_tp);
        }
    }

    Eigen::VectorXd m_pop; ///< pre-allocated temporary, used in right_hand_side()
    mio::TimeSeries<ScalarType> m_flow_result; ///< flow result of the simulation
};

/**
 * @brief simulate simulates a compartmental model and returns the same results as simulate and also the flows.
 * @param[in] t0 start time
 * @param[in] tmax end time
 * @param[in] dt initial step size of integration
 * @param[in] model: An instance of a compartmental model
 * @return a Tuple of two TimeSeries to represent the final simulation result and flows
 * @tparam Model a compartment model type
 * @tparam Sim a simulation type that can simulate the model.
 */
template <class Model, class Sim = FlowSimulation<Model>>
std::vector<TimeSeries<ScalarType>> simulate_flows(double t0, double tmax, double dt, Model const& model,
                                                   std::shared_ptr<IntegratorCore> integrator = nullptr)
{
    model.check_constraints();
    Sim sim(model, t0, dt);
    if (integrator) {
        sim.set_integrator(integrator);
    }
    sim.advance(tmax);
    return {sim.get_result(), sim.get_flows()};
}

} // namespace mio

#endif
