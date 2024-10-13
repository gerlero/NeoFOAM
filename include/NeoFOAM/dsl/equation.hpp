// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023-2024 NeoFOAM authors
#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <utility>

#include "NeoFOAM/core/primitives/scalar.hpp"
#include "NeoFOAM/fields/field.hpp"
#include "NeoFOAM/dsl/operator.hpp"
// TODO redundant name
#include "NeoFOAM/dsl/timeIntegration/timeIntegration.hpp"
#include "NeoFOAM/core/error.hpp"

#include "NeoFOAM/dsl/timeIntegration/forwardEuler.hpp"
#include "NeoFOAM/finiteVolume/cellCentred/fields/volumeField.hpp"

namespace NeoFOAM::dsl
{


class Equation
{
public:

    Equation(const Executor& exec, std::size_t nCells)
        : exec_(exec), nCells_(nCells), temporalOperators_(), implicitOperators_(),
          explicitOperators_()
    {}

    /* @brief perform all explicit operation and accumulate the result */
    Field<scalar> explicitOperation() const
    {
        Field<scalar> source(exec_, nCells_, 0.0);
        return explicitOperation(source);
    }

    /* @brief perform all explicit operation and accumulate the result */
    Field<scalar> explicitOperation(Field<scalar>& source) const
    {
        for (auto& Operator : explicitOperators_)
        {
            Operator.explicitOperation(source);
        }
        return source;
    }

    void addOperator(const Operator& Operator)
    {
        switch (Operator.getType())
        {
        case Operator::Type::Temporal:
            temporalOperators_.push_back(Operator);
            break;
        case Operator::Type::Implicit:
            implicitOperators_.push_back(Operator);
            break;
        case Operator::Type::Explicit:
            explicitOperators_.push_back(Operator);
            break;
        }
    }

    void addEquation(const Equation& equation)
    {
        for (auto& Operator : equation.temporalOperators_)
        {
            temporalOperators_.push_back(Operator);
        }
        for (auto& Operator : equation.implicitOperators_)
        {
            implicitOperators_.push_back(Operator);
        }
        for (auto& Operator : equation.explicitOperators_)
        {
            explicitOperators_.push_back(Operator);
        }
    }

    template<typename SolutionFieldType>
    void solve(SolutionFieldType& solution, const Dictionary& solverProperties)
    {
        if (temporalOperators_.size() == 0 && implicitOperators_.size() == 0)
        {
            NF_ERROR_EXIT("No temporal or implicit terms to solve.");
        }
        if (temporalOperators_.size() > 0)
        {
            // integrate equations in time
            TimeIntegration<Equation, finiteVolume::cellCentred::VolumeField<scalar>>
                timeIntegrator(solverProperties.subDict("ddtSchemes"));
            // timeIntegrator.solve(solution, solverProperties);
        }
        else
        {
            NF_ERROR_EXIT("Not implemented.");
            // solve sparse matrix system
        }
    }

    /* @brief getter for the total number of terms in the equation */
    size_t size() const
    {
        return temporalOperators_.size() + implicitOperators_.size() + explicitOperators_.size();
    }

    // getters
    const std::vector<Operator>& temporalOperators() const { return temporalOperators_; }

    const std::vector<Operator>& implicitOperators() const { return implicitOperators_; }

    const std::vector<Operator>& explicitOperators() const { return explicitOperators_; }

    std::vector<Operator>& temporalOperators() { return temporalOperators_; }

    std::vector<Operator>& implicitOperators() { return implicitOperators_; }

    std::vector<Operator>& explicitOperators() { return explicitOperators_; }

    const Executor& exec() const { return exec_; }

    std::size_t nCells() const { return nCells_; }

    scalar getDt() const { return dt_; }

    scalar dt_ = 0;

private:

    const Executor exec_;

    const std::size_t nCells_;

    std::vector<Operator> temporalOperators_;

    std::vector<Operator> implicitOperators_;

    std::vector<Operator> explicitOperators_;
};

Equation operator+(Equation lhs, const Equation& rhs)
{
    lhs.addEquation(rhs);
    return lhs;
}

Equation operator+(Equation lhs, const Operator& rhs)
{
    lhs.addOperator(rhs);
    return lhs;
}

Equation operator+(const Operator& lhs, const Operator& rhs)
{
    Equation equation(lhs.exec(), lhs.getSize());
    equation.addOperator(lhs);
    equation.addOperator(rhs);
    return equation;
}

Equation operator*(scalar scale, const Equation& es)
{
    Equation results(es.exec(), es.nCells());
    for (const auto& Operator : es.temporalOperators())
    {
        results.addOperator(scale * Operator);
    }
    for (const auto& Operator : es.implicitOperators())
    {
        results.addOperator(scale * Operator);
    }
    for (const auto& Operator : es.explicitOperators())
    {
        results.addOperator(scale * Operator);
    }
    return results;
}

Equation operator-(Equation lhs, const Equation& rhs)
{
    lhs.addEquation(-1.0 * rhs);
    return lhs;
}

Equation operator-(Equation lhs, const Operator& rhs)
{
    lhs.addOperator(-1.0 * rhs);
    return lhs;
}

Equation operator-(const Operator& lhs, const Operator& rhs)
{
    Equation equation(lhs.exec(), lhs.getSize());
    equation.addOperator(lhs);
    equation.addOperator(Coeff(-1) * rhs);
    return equation;
}

/* @brief free function to solve an equation
 *
 * @param eqn - Equation to solve
 * @param solutionField - Field for which the equation is to be solved
 * @param fvSchemes - Dictionary containing spatial operator and time  integration properties
 * @param fvSolution - Dictionary containing linear solver properties
 */
template<typename FieldType>
void solve(
    const Equation& eqn,
    FieldType& solutionField,
    const Dictionary& schemesDict,
    const Dictionary& solutionDict
)
{
    eqn.solve(solutionField, schemesDict);
}

template class ForwardEuler<Equation, finiteVolume::cellCentred::VolumeField<scalar>>;

} // namespace NeoFOAM::dsl
