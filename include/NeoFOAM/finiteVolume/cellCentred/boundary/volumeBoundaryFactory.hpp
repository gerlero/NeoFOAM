// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 NeoFOAM authors

#pragma once

#include "NeoFOAM/core/dictionary.hpp"
#include "NeoFOAM/core/runtimeSelectionFactory.hpp"
#include "NeoFOAM/fields/domainField.hpp"
#include "NeoFOAM/finiteVolume/cellCentred/boundary/boundaryPatchMixin.hpp"
#include "NeoFOAM/mesh/unstructured.hpp"

namespace NeoFOAM::finiteVolume::cellCentred
{

template<typename ValueType>
class VolumeBoundaryFactory :
    public NeoFOAM::RuntimeSelectionFactory<
        VolumeBoundaryFactory<ValueType>,
        Parameters<const UnstructuredMesh&, const Dictionary&, int>>,
    public BoundaryPatchMixin
{
public:

    static std::string name() { return "VolumeBoundaryFactory"; }

    VolumeBoundaryFactory(
        const UnstructuredMesh& mesh, [[maybe_unused]] const Dictionary&, std::size_t patchID
    )
        : BoundaryPatchMixin(mesh, patchID) {};

    virtual void correctBoundaryCondition(DomainField<ValueType>& domainField) = 0;
};


/**
 * @brief Represents a volume boundary field for a cell-centered finite volume method.
 *
 * @tparam ValueType The data type of the field.
 */
template<typename ValueType>
class VolumeBoundary : public BoundaryPatchMixin
{
public:

    VolumeBoundary(const UnstructuredMesh& mesh, const Dictionary& dict, int patchID)
        : BoundaryPatchMixin(
            mesh.boundaryMesh().offset()[patchID],
            mesh.boundaryMesh().offset()[patchID + 1],
            patchID
        ),
          boundaryCorrectionStrategy_(VolumeBoundaryFactory<ValueType>::create(
              dict.get<std::string>("type"), mesh, dict, patchID
          ))
    {}

    virtual void correctBoundaryCondition(DomainField<ValueType>& domainField)
    {
        boundaryCorrectionStrategy_->correctBoundaryCondition(domainField);
    }

private:

    // NOTE needs full namespace to be not ambiguous
    std::unique_ptr<NeoFOAM::finiteVolume::cellCentred::VolumeBoundaryFactory<ValueType>>
        boundaryCorrectionStrategy_;
};

}
