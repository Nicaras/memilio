#ifndef EPI_SECIR_IMPLICIT_EULER_H
#define EPI_SECIR_IMPLICIT_EULER_H

#include "epidemiology/math/euler.h"
#include "epidemiology/secir/secir.h"

namespace epi
{

/**
 * @brief Implicit Euler integration (not generalized, adapted to SECIHURD-model)
 */
class ImplicitEulerIntegratorCore : public IntegratorCore
{
public:
    /**
     * @brief Setting up the implicit Euler integrator
     * @param params Paramters of the SECIR/SECIHURD model
     */
    ImplicitEulerIntegratorCore(SecirModel const& params);

    /**
     * @brief Fixed step width of the time implicit Euler time integration scheme
     *
     * @param[in] yt value of y at t, y(t)
     * @param[in,out] t current time step h=dt
     * @param[in,out] dt current time step h=dt
     * @param[out] ytp1 approximated value y(t+1)
     */
    bool step(const DerivFunction& f, Eigen::Ref<const Eigen::VectorXd> yt, double& t, double& dt,
              Eigen::Ref<Eigen::VectorXd> ytp1) const override;

    SecirModel const& get_secir_params() const
    {
        return m_model;
    }

    /**
     *  @param tol the required absolute tolerance for the comparison with the Fehlberg approximation (actually not really required but used in SecirSimulation constructor)
     */
    void set_abs_tolerance(double tol)
    {
        m_abs_tol = tol;
    }

private:
    const SecirModel& m_model;
    double m_abs_tol = 1e-4;
};

} // namespace epi

#endif