#ifndef UNCERTAINMATRIX_H
#define UNCERTAINMATRIX_H

#include "epidemiology/utils/date.h"
#include "epidemiology/secir/contact_matrix.h"
#include "epidemiology/secir/damping_sampling.h"

#include <vector>

namespace epi
{

/**
 * @brief The UncertainContactMatrix class consists of a
 *        ContactMatrix with fixed baseline and uncertain Dampings. 
 * 
 * The UncertainContactMatrix class represents a matrix-style model parameter 
 * that can take a ContactMatrix value but that is subjected to a uncertainty,
 * based on contact pattern changes realized by zero or more dampings with uncertain coefficients
 * that are sampled to modify the contacts at some points in time.
 * @see UncertainValue
 */
class UncertainContactMatrix
{
public:
    UncertainContactMatrix(size_t num_matrices = 1, Eigen::Index num_groups = 1);

    UncertainContactMatrix(const ContactMatrixGroup& cont_freq);

    /**
     * @brief Conversion to const ContactMatrix reference by returning the 
     *        ContactMatrix contained in UncertainContactMatrix
     */
    operator ContactMatrixGroup const &() const;

    /**
     * @brief Conversion to ContactMatrix reference by returning the 
     *        ContactMatrix contained in UncertainContactMatrix
     */
    operator ContactMatrixGroup&();

    /**
     * @brief Set an UncertainContactMatrix from a ContactMatrix, 
     *        all distributions remain unchanged.
     */
    UncertainContactMatrix& operator=(const ContactMatrixGroup& cont_freq);

    /**
     * @brief Returns the ContactMatrix reference 
     *        of the UncertainContactMatrix object
     */
    ContactMatrixGroup& get_cont_freq_mat();

    /**
     * @brief Returns the const ContactMatrix reference 
     *        of the UncertainContactMatrix object
     */
    ContactMatrixGroup const& get_cont_freq_mat() const;

    /**
     * @brief Get a list of uncertain Dampings that are sampled and added to the contact matrix.
     * @return list of damping samplings.
     * @{
     */
    const std::vector<DampingSampling>& get_dampings() const
    {
        return m_dampings;
    }
    std::vector<DampingSampling>& get_dampings()
    {
        return m_dampings;
    }
    /**@}*/

    /**
     * Damping that is active during school holiday periods.
     * time is ignored and taken from holidays instead.
     * @{
     */
    const DampingSampling& get_school_holiday_damping() const
    {
        return m_school_holiday_damping;
    }
    DampingSampling& get_school_holiday_damping()
    {
        return m_school_holiday_damping;
    }
    /**@}*/

    /**
     * list of school holiday periods.
     * one period is a pair of start and end dates.
     * @{
     */
    std::vector<std::pair<SimulationTime, SimulationTime>>& get_school_holidays()
    {
        return m_school_holidays;
    }
    const std::vector<std::pair<SimulationTime, SimulationTime>>& get_school_holidays() const
    {
        return m_school_holidays;
    }
    /**@}*/

    /**
     * @brief Samples dampings and adds them to the contact matrix.
     * @param accum accumulating current and newly sampled dampings if true;
     *              default: false; removing all previously set dampings
     */
    ContactMatrixGroup draw_sample(bool accum = false);

    /**
     * draw sample of all dampings.
     */
    void draw_sample_dampings();

    /**
     * create the contact matrix using the sampled dampings.
     * @param accum accumulating current and newly dampings if true;
     *              default: false; removing all previously set dampings
     */
    ContactMatrixGroup make_matrix(bool accum = false);

private:
    ContactMatrixGroup m_cont_freq;
    std::vector<DampingSampling> m_dampings;
    DampingSampling m_school_holiday_damping;
    std::vector<std::pair<SimulationTime, SimulationTime>> m_school_holidays;
};

} // namespace epi

#endif // UNCERTAINMATRIX_H
