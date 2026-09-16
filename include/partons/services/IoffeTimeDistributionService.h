#ifndef IOFFE_TIME_DISTRIBUTION_SERVICE_H
#define IOFFE_TIME_DISTRIBUTION_SERVICE_H

/**
 * @file IoffeTimeDistributionService.h
 * @author PARTONS team
 * @date 2026
 * @version 1.0
 */

#include <string>

#include "../beans/gpd/GPDType.h"
#include "../beans/ioffe_time/IoffeTimeDistributionResult.h"
#include "../beans/ioffe_time/IoffeTimeKinematic.h"
#include "../beans/List.h"
#include "../ServiceObjectTyped.h"

namespace PARTONS {
class IoffeTimeDistributionModule;
} /* namespace PARTONS */

namespace PARTONS {

/**
 * @class IoffeTimeDistributionService
 *
 * @brief Service for the evaluation of Ioffe-time (pseudo-)distributions from GPD models.
 *
 * The service evaluates an IoffeTimeDistributionModule for a single kinematics or for a list of kinematics
 * (in parallel threads), either from C++ or from XML scenarios, e.g.:
 *
 * \code{.xml}
 * <task service="IoffeTimeDistributionService" method="computeManyKinematic">
 *    <kinematics type="IoffeTimeKinematic">
 *       <param name="file" value="lattice_kinematics.csv" />
 *    </kinematics>
 *    <computation_configuration>
 *       <module type="IoffeTimeDistributionModule" name="IoffeTimeDistributionPseudoNLO">
 *          <param name="alphaS" value="0.303" />
 *          <module type="GPDModule" name="GPDGK16">
 *          </module>
 *       </module>
 *    </computation_configuration>
 * </task>
 * <task service="IoffeTimeDistributionService" method="printResults">
 * </task>
 * \endcode
 *
 * where the kinematics file contains one kinematic point per line, `nu|z2|xi|t|MuF2|MuR2`, with an optional
 * first line giving the units, e.g. `#none|fm2|none|GeV2|GeV2|GeV2` (see KinematicUtils::getIoffeTimeKinematicFromFile()).
 * Such a file is easily produced from the kinematics of a lattice QCD data set (Ioffe times \f$\nu = p_{3} z_{3}\f$ and
 * separations \f$z_{3}\f$ of the measured matrix elements), which makes the comparison with lattice data, e.g. in a
 * global fit, straightforward.
 *
 * The batch size of the threaded evaluation is set by the property `ioffe_time.service.batch.size` in
 * `partons.properties`; the default is 1000 if the property is missing.
 */
class IoffeTimeDistributionService: public ServiceObjectTyped<
        IoffeTimeKinematic, IoffeTimeDistributionResult> {

public:

    static const unsigned int classId; ///< Unique ID to automatically register the class in the registry.

    static const std::string IOFFE_TIME_DISTRIBUTION_SERVICE_COMPUTE_SINGLE_KINEMATIC; ///< Name of the XML task used to compute a distribution at given kinematics.
    static const std::string IOFFE_TIME_DISTRIBUTION_SERVICE_COMPUTE_MANY_KINEMATIC; ///< Name of the XML task used to compute distributions for a list of kinematics.

    static const std::string PROPERTY_NAME_BATCH_SIZE; ///< Name of the property setting the batch size.
    static const unsigned int DEFAULT_BATCH_SIZE; ///< Default batch size.

    /**
     * Destructor.
     */
    virtual ~IoffeTimeDistributionService();

    virtual void resolveObjectDependencies();
    virtual void computeTask(Task &task);

    /**
     * Compute distributions at a single kinematics.
     * @param kinematic Kinematics.
     * @param pModule Module to be used.
     * @param gpdTypeList GPD types to be computed (all available if empty).
     */
    IoffeTimeDistributionResult computeSingleKinematic(
            const IoffeTimeKinematic &kinematic,
            IoffeTimeDistributionModule* pModule,
            const List<GPDType>& gpdTypeList = List<GPDType>()) const;

    /**
     * Compute distributions for a list of kinematics (threaded).
     * @param kinematicList Kinematics.
     * @param pModule Module to be used.
     * @param gpdTypeList GPD types to be computed (all available if empty).
     */
    List<IoffeTimeDistributionResult> computeManyKinematic(
            const List<IoffeTimeKinematic> &kinematicList,
            IoffeTimeDistributionModule* pModule,
            const List<GPDType>& gpdTypeList = List<GPDType>());

    /**
     * Create and configure a module from a task.
     */
    IoffeTimeDistributionModule* newIoffeTimeDistributionModuleFromTask(
            const Task &task) const;

    /**
     * Create a kinematics from a task.
     */
    IoffeTimeKinematic newKinematicFromTask(const Task &task) const;

    /**
     * Create a list of kinematics from a task (file given by parameter "file").
     */
    List<IoffeTimeKinematic> newListOfKinematicFromTask(const Task &task) const;

protected:

    /**
     * Constructor.
     */
    IoffeTimeDistributionService(const std::string &className);

private:

    IoffeTimeDistributionResult computeSingleKinematicTask(Task &task);
    List<IoffeTimeDistributionResult> computeManyKinematicTask(Task &task);

    List<GPDType> getFinalGPDTypeList(IoffeTimeDistributionModule* pModule,
            const List<GPDType> &gpdTypeList) const;
};

} /* namespace PARTONS */

#endif /* IOFFE_TIME_DISTRIBUTION_SERVICE_H */
