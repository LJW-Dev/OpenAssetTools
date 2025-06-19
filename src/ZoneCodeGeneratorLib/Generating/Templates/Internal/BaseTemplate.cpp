#include "BaseTemplate.h"

#include "Domain/Computations/MemberComputations.h"
#include "Domain/Definition/ArrayDeclarationModifier.h"

#include <sstream>

BaseTemplate::BaseTemplate(std::ostream& stream, const RenderingContext& context)
    : m_out(stream),
      m_env(context),
      m_intendation(0u)
{
}

void BaseTemplate::DoIntendation() const
{
    for (auto i = 0u; i < m_intendation; i++)
        m_out << INTENDATION;
}

std::string BaseTemplate::Upper(std::string str)
{
    for (auto& c : str)
        c = static_cast<char>(toupper(c));

    return str;
}

std::string BaseTemplate::Lower(std::string str)
{
    for (auto& c : str)
        c = static_cast<char>(tolower(c));

    return str;
}

void BaseTemplate::MakeSafeTypeNameInternal(const DataDefinition* def, std::ostringstream& str)
{
    for (const auto& c : def->m_name)
    {
        if (isspace(c))
            str << "_";
        else
            str << c;
    }
}

void BaseTemplate::MakeTypeVarNameInternal(const DataDefinition* def, std::ostringstream& str)
{
    str << "var";
    MakeSafeTypeNameInternal(def, str);
}

void BaseTemplate::MakeTypeWrittenVarNameInternal(const DataDefinition* def, std::ostringstream& str)
{
    str << "var";
    MakeSafeTypeNameInternal(def, str);
    str << "Written";
}

void BaseTemplate::MakeTypePtrVarNameInternal(const DataDefinition* def, std::ostringstream& str)
{
    str << "var";
    MakeSafeTypeNameInternal(def, str);
    str << "Ptr";
}

void BaseTemplate::MakeTypeWrittenPtrVarNameInternal(const DataDefinition* def, std::ostringstream& str)
{
    str << "var";
    MakeSafeTypeNameInternal(def, str);
    str << "PtrWritten";
}

void BaseTemplate::MakeArrayIndicesInternal(const DeclarationModifierComputations& modifierComputations, std::ostringstream& str)
{
    for (const auto index : modifierComputations.GetArrayIndices())
    {
        str << "[" << index << "]";
    }
}

std::string BaseTemplate::MakeTypeVarName(const DataDefinition* def)
{
    std::ostringstream str;
    MakeTypeVarNameInternal(def, str);
    return str.str();
}

std::string BaseTemplate::MakeTypeWrittenVarName(const DataDefinition* def)
{
    std::ostringstream str;
    MakeTypeWrittenVarNameInternal(def, str);
    return str.str();
}

std::string BaseTemplate::MakeTypePtrVarName(const DataDefinition* def)
{
    std::ostringstream str;
    MakeTypePtrVarNameInternal(def, str);
    return str.str();
}

std::string BaseTemplate::MakeTypeWrittenPtrVarName(const DataDefinition* def)
{
    std::ostringstream str;
    MakeTypeWrittenPtrVarNameInternal(def, str);
    return str.str();
}

std::string BaseTemplate::MakeSafeTypeName(const DataDefinition* def)
{
    std::ostringstream str;
    MakeSafeTypeNameInternal(def, str);
    return str.str();
}

std::string BaseTemplate::MakeMemberAccess(const StructureInformation* info, const MemberInformation* member, const DeclarationModifierComputations& modifier)
{
    std::ostringstream str;
    MakeTypeVarNameInternal(info->m_definition, str);
    str << "->" << member->m_member->m_name;
    MakeArrayIndicesInternal(modifier, str);

    return str.str();
}

std::string
    BaseTemplate::MakeWrittenMemberAccess(const StructureInformation* info, const MemberInformation* member, const DeclarationModifierComputations& modifier)
{
    std::ostringstream str;
    MakeTypeWrittenVarNameInternal(info->m_definition, str);
    str << "->" << member->m_member->m_name;
    MakeArrayIndicesInternal(modifier, str);

    return str.str();
}

std::string BaseTemplate::MakeTypeDecl(const TypeDeclaration* decl)
{
    std::ostringstream str;
    if (decl->m_is_const)
    {
        str << "const ";
    }

    str << decl->m_type->GetFullName();
    return str.str();
}

std::string BaseTemplate::MakeFollowingReferences(const std::vector<DeclarationModifier*>& modifiers)
{
    std::ostringstream str;
    for (const auto* modifier : modifiers)
    {
        if (modifier->GetType() == DeclarationModifierType::ARRAY)
        {
            const auto* array = dynamic_cast<const ArrayDeclarationModifier*>(modifier);
            str << "[" << array->m_size << "]";
        }
        else
        {
            str << "*";
        }
    }

    return str.str();
}

std::string BaseTemplate::MakeArrayIndices(const DeclarationModifierComputations& modifierComputations)
{
    std::ostringstream str;
    MakeArrayIndicesInternal(modifierComputations, str);
    return str.str();
}

std::string BaseTemplate::MakeCustomActionCall(const CustomAction* action)
{
    std::ostringstream str;
    str << "m_actions." << action->m_action_name << "(";

    auto first = true;
    for (const auto* def : action->m_parameter_types)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            str << ", ";
        }

        MakeTypeVarNameInternal(def, str);
    }

    str << ");";
    return str.str();
}

std::string BaseTemplate::MakeArrayCount(const ArrayDeclarationModifier* arrayModifier)
{
    if (arrayModifier->m_dynamic_count_evaluation)
    {
        return MakeEvaluation(arrayModifier->m_dynamic_count_evaluation.get());
    }

    return std::to_string(arrayModifier->m_size);
}

void BaseTemplate::MakeOperandStatic(const OperandStatic* op, std::ostringstream& str)
{
    if (op->m_enum_member != nullptr)
    {
        str << op->m_enum_member->m_name;
    }
    else
    {
        str << op->m_value;
    }
}

void BaseTemplate::MakeOperandDynamic(const OperandDynamic* op, std::ostringstream& str)
{
    MakeTypeVarNameInternal(op->m_structure->m_definition, str);

    if (!op->m_referenced_member_chain.empty())
    {
        str << "->";
        const auto lastEntry = op->m_referenced_member_chain.end() - 1;
        for (auto i = op->m_referenced_member_chain.begin(); i != lastEntry; ++i)
        {
            MemberComputations computations(*i);
            str << (*i)->m_member->m_name;

            if (computations.ContainsNonEmbeddedReference())
                str << "->";
            else
                str << ".";
        }

        str << (*lastEntry)->m_member->m_name;
    }

    for (const auto& arrayIndex : op->m_array_indices)
    {
        str << "[";
        MakeEvaluationInternal(arrayIndex.get(), str);
        str << "]";
    }
}

void BaseTemplate::MakeOperation(const Operation* operation, std::ostringstream& str)
{
    if (operation->Operand1NeedsParenthesis())
    {
        str << "(";
        MakeEvaluationInternal(operation->m_operand1.get(), str);
        str << ")";
    }
    else
        MakeEvaluationInternal(operation->m_operand1.get(), str);

    str << " " << operation->m_operation_type->m_syntax << " ";

    if (operation->Operand2NeedsParenthesis())
    {
        str << "(";
        MakeEvaluationInternal(operation->m_operand2.get(), str);
        str << ")";
    }
    else
        MakeEvaluationInternal(operation->m_operand2.get(), str);
}

void BaseTemplate::MakeEvaluationInternal(const IEvaluation* evaluation, std::ostringstream& str)
{
    if (evaluation->GetType() == EvaluationType::OPERATION)
        MakeOperation(dynamic_cast<const Operation*>(evaluation), str);
    else if (evaluation->GetType() == EvaluationType::OPERAND_STATIC)
        MakeOperandStatic(dynamic_cast<const OperandStatic*>(evaluation), str);
    else if (evaluation->GetType() == EvaluationType::OPERAND_DYNAMIC)
        MakeOperandDynamic(dynamic_cast<const OperandDynamic*>(evaluation), str);
}

std::string BaseTemplate::MakeEvaluation(const IEvaluation* evaluation)
{
    std::ostringstream str;
    MakeEvaluationInternal(evaluation, str);
    return str.str();
}
