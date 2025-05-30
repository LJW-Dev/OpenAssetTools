#pragma once

#include "DeclarationModifier.h"
#include "Domain/Evaluation/IEvaluation.h"

#include <memory>

class ArrayDeclarationModifier final : public DeclarationModifier
{
public:
    explicit ArrayDeclarationModifier(int size);

    [[nodiscard]] DeclarationModifierType GetType() const override;

    int m_size;

    /**
     * \brief The array size is not actually given by \c m_size but instead is dynamic.
     */
    std::unique_ptr<IEvaluation> m_dynamic_size_evaluation;

    /**
     * \brief The array has a size that is given by \c m_size but only a certain dynamic amount is handled by generated count.
     */
    std::unique_ptr<IEvaluation> m_dynamic_count_evaluation;
};
