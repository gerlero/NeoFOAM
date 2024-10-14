// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023-2024 NeoFOAM authors
#pragma once

#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <concepts>

#include "NeoFOAM/core/primitives/scalar.hpp"
#include "NeoFOAM/fields/field.hpp"
#include "NeoFOAM/core/input.hpp"
#include "NeoFOAM/dsl/coeff.hpp"


namespace NeoFOAM::dsl
{

template<typename T>
concept HasTemporalOperator = requires(T t) {
    {
        t.temporalOperation(std::declval<Field<scalar>&>(), std::declval<scalar>())
    } -> std::same_as<void>; // Adjust return type and arguments as needed
};

template<typename T>
concept HasExplicitOperator = requires(T t) {
    {
        t.explicitOperation(std::declval<Field<scalar>&>())
    } -> std::same_as<void>; // Adjust return type and arguments as needed
};


/* @class OperatorMixin
 * @brief A mixin class to represent simplify implementations of concrete operators
 * in NeoFOAMs dsl
 *
 *
 * @ingroup dsl
 */
class OperatorMixin
{

public:

    OperatorMixin(const Executor exec) : exec_(exec), coeffs_() {};

    virtual ~OperatorMixin() = default;

    virtual const Executor& exec() const final { return exec_; }

    Coeff& getCoefficient() { return coeffs_; }

    const Coeff& getCoefficient() const { return coeffs_; }

    /* @brief Given an input this function reads required coeffs */
    void build([[maybe_unused]] const Input& input) {}

protected:

    const Executor exec_; //!< Executor associated with the field. (CPU, GPU, openMP, etc.)

    Coeff coeffs_;
};


/* @class Operator
 * @brief A class to represent a operator in NeoFOAMs dsl
 *
 * The design here is based on the type erasure design pattern
 * see https://www.youtube.com/watch?v=4eeESJQk-mw
 *
 * Motivation for using type erasure is that concrete implementation
 * of Operators e.g Divergence, Laplacian, etc can be stored in a vector of
 * Operators
 *
 * @ingroup dsl
 */
class Operator
{
public:

    enum class Type
    {
        Temporal,
        Implicit,
        Explicit
    };

    template<typename T>
    Operator(T cls) : model_(std::make_unique<OperatorModel<T>>(std::move(cls)))
    {}

    Operator(const Operator& eqnOperator) : model_ {eqnOperator.model_->clone()} {}

    Operator(Operator&& eqnOperator) : model_ {std::move(eqnOperator.model_)} {}

    void explicitOperation(Field<scalar>& source) const { model_->explicitOperation(source); }

    void temporalOperation(Field<scalar>& field) { model_->temporalOperation(field); }

    /* returns the fundamental type of an operator, ie explicit, implicit, temporal */
    Operator::Type getType() const { return model_->getType(); }

    Coeff& getCoefficient() { return model_->getCoefficient(); }

    Coeff getCoefficient() const { return model_->getCoefficient(); }

    /* get the corresponding field size over which the operator operates */
    size_t getSize() const { return model_->getSize(); }

    /* @brief Given an input this function reads required coeffs */
    void build(const Input& input) { model_->build(input); }

    /* @brief Get the executor */
    const Executor& exec() const { return model_->exec(); }


private:

    /* @brief Base class defining the concept of a term. This effectively
     * defines what functions need to be implemented by a concrete Operator implementation
     * */
    struct OperatorConcept
    {
        virtual ~OperatorConcept() = default;

        virtual void explicitOperation(Field<scalar>& source) const = 0;

        virtual void temporalOperation(Field<scalar>& field) = 0;

        /* @brief Given an input this function reads required coeffs */
        virtual void build(const Input& input) = 0;

        /* returns the name of the operator */
        virtual std::string getName() const = 0;

        /* returns the fundamental type of an operator, ie explicit, implicit, temporal */
        virtual Operator::Type getType() const = 0;

        virtual size_t getSize() const = 0;

        /* @brief get the associated coefficient for this term */
        virtual Coeff& getCoefficient() = 0;

        /* @brief get the associated coefficient for this term */
        virtual Coeff getCoefficient() const = 0;

        /* @brief Get the executor */
        virtual const Executor& exec() const = 0;

        // The Prototype Design Pattern
        virtual std::unique_ptr<OperatorConcept> clone() const = 0;
    };

    // Templated derived class to implement the type-specific behavior
    template<typename ConcreteOperatorType>
    struct OperatorModel : OperatorConcept
    {
        /* @brief build with concrete operator */
        OperatorModel(ConcreteOperatorType concreteOp) : concreteOp_(std::move(concreteOp)) {}

        /* returns the name of the operator */
        std::string getName() const override { return concreteOp_.getName(); }

        virtual void explicitOperation(Field<scalar>& source) const override
        {
            if constexpr (HasExplicitOperator<ConcreteOperatorType>)
            {
                concreteOp_.explicitOperation(source);
            }
        }

        virtual void temporalOperation(Field<scalar>& field) override
        {
            if constexpr (HasTemporalOperator<ConcreteOperatorType>)
            {
                concreteOp_.temporalOperation(field);
            }
        }

        /* @brief Given an input this function reads required coeffs */
        virtual void build(const Input& input) override { concreteOp_.build(input); }

        /* returns the fundamental type of an operator, ie explicit, implicit, temporal */
        Operator::Type getType() const override { return concreteOp_.getType(); }

        /* @brief Get the executor */
        const Executor& exec() const override { return concreteOp_.exec(); }

        /* @brief get the associated coefficient for this term */
        virtual Coeff& getCoefficient() override { return concreteOp_.getCoefficient(); }

        /* @brief get the associated coefficient for this term */
        virtual Coeff getCoefficient() const override { return concreteOp_.getCoefficient(); }

        /* @brief get the associated coefficient for this term */
        virtual size_t getSize() const override { return concreteOp_.getSize(); }

        // The Prototype Design Pattern
        std::unique_ptr<OperatorConcept> clone() const override
        {
            return std::make_unique<OperatorModel>(*this);
        }

        ConcreteOperatorType concreteOp_;
    };

    std::unique_ptr<OperatorConcept> model_;
};


auto operator*(scalar scalarCoeff, const Operator& rhs)
{
    Operator result = rhs;
    result.getCoefficient() *= scalarCoeff;
    return result;
}

auto operator*(const Field<scalar>& coeffField, const Operator& rhs)
{
    Operator result = rhs;
    result.getCoefficient() *= Coeff(coeffField);
    return result;
}

auto operator*(const Coeff& coeff, const Operator& rhs)
{
    Operator result = rhs;
    result.getCoefficient() *= coeff;
    return result;
}

template<typename CoeffFunction>
    requires std::invocable<CoeffFunction&, size_t>
Operator operator*(CoeffFunction coeffFunc, const Operator& lhs)
{
    // TODO implement
    NF_ERROR_EXIT("Not implemented");
    Operator result = lhs;
    // if (!result.getCoefficient().useSpan)
    // {
    //     result.setField(std::make_shared<Field<scalar>>(result.exec(), result.nCells(), 1.0));
    // }
    // map(result.exec(), result.getCoefficient().values, scaleFunc);
    return result;
}

} // namespace NeoFOAM::dsl
