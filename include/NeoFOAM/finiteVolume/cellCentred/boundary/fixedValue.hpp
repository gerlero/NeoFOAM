// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 NeoFOAM authors
#pragma once
#include "Kokkos_Core.hpp"

#include "NeoFOAM/core.hpp"
#include "NeoFOAM/finiteVolume/cellCentred/boundary/volumeBoundaryBase.hpp"
#include "NeoFOAM/mesh/unstructured.hpp"

namespace NeoFOAM::finiteVolume::cellCentred
{

template<typename ValueType>
class FixedValue : public VolumeBoundaryFactory<ValueType>
{

public:

    using FixedValueType = FixedValue<ValueType>;

    template<typename executor>
    void setFixedValue(const executor& exec, std::span<ValueType> field, ValueType value)
    {
        if constexpr (std::is_same<std::remove_reference_t<executor>, NeoFOAM::CPUExecutor>::value)
        {
            for (std::size_t i = start_; i < end_; i++)
            {
                field[i] = value;
            }
        }
        else
        {
            using runOn = typename executor::exec;
            Kokkos::parallel_for(
                "parallelForImpl",
                Kokkos::RangePolicy<runOn>(start_, end_),
                KOKKOS_LAMBDA(std::size_t i) { field[i] = value; }
            );
        }
    }

    FixedValue(std::size_t start, std::size_t end, std::size_t patchID, ValueType uniformValue)
        : VolumeBoundaryFactory<ValueType>(), start_(start), end_(end), patchID_(patchID),
          uniformValue_(uniformValue)
    {
        VolumeBoundaryFactory<ValueType>::template registerClass<FixedValueType>();
    }

    static std::unique_ptr<VolumeBoundaryFactory<ValueType>>
    create(const NeoFOAM::UnstructuredMesh& mesh, const NeoFOAM::Dictionary& dict, int patchID)
    {

        ValueType uniformValue = dict.get<ValueType>("uniformValue");
        std::size_t start = dict.get<std::size_t>("start");
        std::size_t end = dict.get<std::size_t>("end");
        return std::make_unique<FixedValueType>(start, end, patchID, uniformValue);
    }

    virtual void correctBoundaryCondition(NeoFOAM::DomainField<ValueType>& domainField) override
    {
        std::visit(
            [&](auto exec)
            { setFixedValue(exec, domainField.boundaryField().refValue().field(), uniformValue_); },
            domainField.exec()
        );
    }

    static std::string name() { return "FixedValue"; }

private:

    ValueType uniformValue_;
    std::size_t start_;
    std::size_t end_;
    std::size_t patchID_;
};

}
