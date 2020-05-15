#include "load_test_data.h"
#include "epidemiology/secir.h"
#include <gtest/gtest.h>

TEST(TestSecir, compareWithPreviousRun)
{
    double t0   = 0;
    double tmax = 5;
    double dt   = 0.1;

    double tinc = 5.2, tinfmild = 6, tserint = 4.2, thosp2home = 12, thome2hosp = 5, thosp2icu = 2, ticu2home = 8,
           tinfasy = 6.2, ticu2death = 5;

    double cont_freq = 0.5, alpha = 0.09, beta = 0.25, delta = 0.3, rho = 0.2, theta = 0.25;

    double nb_total_t0 = 10000, nb_exp_t0 = 100, nb_inf_t0 = 50, nb_car_t0 = 50, nb_hosp_t0 = 20, nb_icu_t0 = 10,
           nb_rec_t0 = 10, nb_dead_t0 = 0;

    std::vector<epi::SecirParams> params{epi::SecirParams{}};
    epi::ContactFrequencyMatrix contact_freq_matrix{};

    // alpha = alpha_in; // percentage of asymptomatic cases
    // beta  = beta_in; // risk of infection from the infected symptomatic patients
    // rho   = rho_in; // hospitalized per infected
    // theta = theta_in; // icu per hospitalized
    // delta = delta_in; // deaths per ICUs

    params[0].times.set_incubation(tinc);
    params[0].times.set_infectious_mild(tinfmild);
    params[0].times.set_serialinterval(tserint);
    params[0].times.set_hospitalized_to_home(thosp2home);
    params[0].times.set_home_to_hospitalized(thome2hosp);
    params[0].times.set_hospitalized_to_icu(thosp2icu);
    params[0].times.set_icu_to_home(ticu2home);
    params[0].times.set_infectious_asymp(tinfasy);
    params[0].times.set_icu_to_death(ticu2death);

    contact_freq_matrix.set_cont_freq(cont_freq, 0, 0);
    epi::Damping dummy(30., 0.3);
    contact_freq_matrix.update_dampings(dummy, 0, 0);

    params[0].populations.set_total_t0(nb_total_t0);
    params[0].populations.set_exposed_t0(nb_exp_t0);
    params[0].populations.set_carrier_t0(nb_car_t0);
    params[0].populations.set_infectious_t0(nb_inf_t0);
    params[0].populations.set_hospital_t0(nb_hosp_t0);
    params[0].populations.set_icu_t0(nb_icu_t0);
    params[0].populations.set_recovered_t0(nb_rec_t0);
    params[0].populations.set_dead_t0(nb_dead_t0);

    params[0].probabilities.set_asymp_per_infectious(alpha);
    params[0].probabilities.set_risk_from_symptomatic(beta);
    params[0].probabilities.set_hospitalized_per_infectious(rho);
    params[0].probabilities.set_icu_per_hospitalized(theta);
    params[0].probabilities.set_dead_per_icu(delta);

    std::vector<Eigen::VectorXd> secihurd(0);
    auto t = simulate(t0, tmax, dt, contact_freq_matrix, params, secihurd);

    auto compare = load_test_data_csv<double>("data/secihurd-compare.csv");

    ASSERT_EQ(compare.size(), t.size());
    ASSERT_EQ(compare.size(), secihurd.size());
    for (size_t i = 0; i < compare.size(); i++) {
        ASSERT_EQ(compare[i].size(), secihurd[i].size() + 1) << "at row " << i;
        ASSERT_FLOAT_EQ(t[i], compare[i][0]) << "at row " << i;
        for (size_t j = 1; j < compare[i].size(); j++) {
            EXPECT_FLOAT_EQ(secihurd[i][j - 1], compare[i][j]) << " at row " << i;
        }
    }
}