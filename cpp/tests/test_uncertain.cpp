#include <memory>
#include <epidemiology/memory.h>
#include <epidemiology/uncertain_value.h>
#include <epidemiology/uncertain_matrix.h>
#include <epidemiology/parameter_studies/parameter_distributions.h>
#include <distributions_helpers.h>
#include <gtest/gtest.h>

TEST(TestUncertain, uncertain_value)
{
    epi::UncertainValue val(3.0);
    EXPECT_EQ(val, 3.0);

    val = 2.0;
    EXPECT_EQ(val, 2.0);

    double dev_rel     = 0.2;
    double lower_bound = std::max(1e-6, (1 - dev_rel * 2.6) * val);
    double upper_bound = (1 + dev_rel * 2.6) * val;
    val.set_distribution(epi::ParameterDistributionNormal(lower_bound, upper_bound, val, dev_rel * val));

    epi::UncertainValue val2(val);
    EXPECT_EQ(val2, 2.0);

    EXPECT_NE(val.get_distribution().get(), val2.get_distribution().get()); // dists get copied
    check_dist(*val.get_distribution().get(), *val2.get_distribution().get());

    for (int i = 0; i < 10; i++) {
        val2.draw_sample();
        EXPECT_GE(val2, lower_bound);
        EXPECT_LE(val2, upper_bound);
    }

    double& dval = val;
    dval         = 4.0;
    EXPECT_EQ(val, 4.0);

    val2 = 4.0;
    EXPECT_EQ(val2, val); // only checks doubles, not dists

    epi::UncertainValue val3;
    val3 = val2;

    EXPECT_EQ(val3, val2);
    check_dist(*val3.get_distribution().get(), *val2.get_distribution().get());
}

TEST(TestUncertain, uncertain_matrix)
{
    epi::ContactFrequencyMatrix cont_freq_matrix{2};
    epi::Damping dummy(30., 0.3);
    for (int i = 0; i < 2; i++) {
        for (int j = i; j < 2; j++) {
            cont_freq_matrix.set_cont_freq((i + 1) * (j + 1), i, j);
            cont_freq_matrix.add_damping(dummy, i, j);
        }
    }

    epi::UncertainContactMatrix uncertain_mat{cont_freq_matrix};

    EXPECT_EQ(uncertain_mat.get_cont_freq_mat().get_cont_freq(0, 1), 2);
    EXPECT_EQ(uncertain_mat.get_cont_freq_mat().get_cont_freq(1, 1), 4);
    EXPECT_EQ(uncertain_mat.get_cont_freq_mat().get_dampings(1, 1).get_factor(37),
              cont_freq_matrix.get_dampings(1, 1).get_factor(37));
    EXPECT_EQ(uncertain_mat.get_cont_freq_mat().get_dampings(1, 1).get_factor(37), 0.3);

    uncertain_mat.set_dist_damp_nb(epi::ParameterDistributionUniform(1, 3));
    uncertain_mat.set_dist_damp_days(epi::ParameterDistributionUniform(0, 19));
    uncertain_mat.set_dist_damp_diag_base(epi::ParameterDistributionUniform(0.1, 1));
    uncertain_mat.set_dist_damp_diag_rel(epi::ParameterDistributionUniform(0.6, 1.4));
    uncertain_mat.set_dist_damp_offdiag_rel(epi::ParameterDistributionUniform(0.7, 1.1));

    epi::UncertainContactMatrix uncertain_mat2{uncertain_mat};
    uncertain_mat2.draw_sample(true); // retain previously added dampings in contact patterns
    EXPECT_GE(uncertain_mat2.get_cont_freq_mat().get_dampings(0, 0).get_factor(20), 0.06);
    EXPECT_LE(uncertain_mat2.get_cont_freq_mat().get_dampings(0, 0).get_factor(20), 1.4);
    EXPECT_EQ(uncertain_mat2.get_cont_freq_mat().get_dampings(1, 1).get_factor(37), 0.3);

    uncertain_mat2.draw_sample(); // removes all previously added dampings (argument default = false)
    EXPECT_EQ(uncertain_mat2.get_cont_freq_mat().get_dampings(1, 1).get_factor(37),
              uncertain_mat2.get_cont_freq_mat().get_dampings(1, 1).get_factor(20));

    check_dist(*uncertain_mat.get_dist_damp_days().get(), *uncertain_mat2.get_dist_damp_days().get());
    check_dist(*uncertain_mat.get_dist_damp_nb().get(), *uncertain_mat2.get_dist_damp_nb().get());
    check_dist(*uncertain_mat.get_dist_damp_diag_base().get(), *uncertain_mat2.get_dist_damp_diag_base().get());
    check_dist(*uncertain_mat.get_dist_damp_diag_rel().get(), *uncertain_mat2.get_dist_damp_diag_rel().get());
    check_dist(*uncertain_mat.get_dist_damp_offdiag_rel().get(), *uncertain_mat2.get_dist_damp_offdiag_rel().get());

    for (int i = 0; i < 10; i++) {
        double sample = uncertain_mat2.get_dist_damp_nb()->get_sample();
        EXPECT_GE(sample, 1);
        EXPECT_LE(sample, 3);

        sample = uncertain_mat2.get_dist_damp_days()->get_sample();
        EXPECT_GE(sample, 0);
        EXPECT_LE(sample, 19);

        sample = uncertain_mat2.get_dist_damp_diag_base()->get_sample();
        EXPECT_GE(sample, 0.1);
        EXPECT_LE(sample, 1);

        sample = uncertain_mat2.get_dist_damp_diag_rel()->get_sample();
        EXPECT_GE(sample, 0.6);
        EXPECT_LE(sample, 1.4);

        sample = uncertain_mat2.get_dist_damp_offdiag_rel()->get_sample();
        EXPECT_GE(sample, 0.7);
        EXPECT_LE(sample, 1.1);
    }

    epi::UncertainContactMatrix uncertain_mat3;
    uncertain_mat3 = uncertain_mat2;

    uncertain_mat3.draw_sample();
    EXPECT_EQ(uncertain_mat2.get_cont_freq_mat().get_dampings(1, 1).get_factor(37),
              uncertain_mat2.get_cont_freq_mat().get_dampings(1, 1).get_factor(20));

    check_dist(*uncertain_mat3.get_dist_damp_days().get(), *uncertain_mat2.get_dist_damp_days().get());
    check_dist(*uncertain_mat3.get_dist_damp_nb().get(), *uncertain_mat2.get_dist_damp_nb().get());
    check_dist(*uncertain_mat3.get_dist_damp_diag_base().get(), *uncertain_mat2.get_dist_damp_diag_base().get());
    check_dist(*uncertain_mat3.get_dist_damp_diag_rel().get(), *uncertain_mat2.get_dist_damp_diag_rel().get());
    check_dist(*uncertain_mat3.get_dist_damp_offdiag_rel().get(), *uncertain_mat2.get_dist_damp_offdiag_rel().get());
}