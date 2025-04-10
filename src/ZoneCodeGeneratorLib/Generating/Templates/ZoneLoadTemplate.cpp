#include "ZoneLoadTemplate.h"

#include "Domain/Computations/MemberComputations.h"
#include "Domain/Computations/StructureComputations.h"
#include "Internal/BaseTemplate.h"

#include <cassert>
#include <iostream>
#include <sstream>

class ZoneLoadTemplate::Internal final : BaseTemplate
{
    enum class MemberLoadType
    {
        ARRAY_POINTER,
        DYNAMIC_ARRAY,
        EMBEDDED,
        EMBEDDED_ARRAY,
        POINTER_ARRAY,
        SINGLE_POINTER
    };

    static std::string LoaderClassName(StructureInformation* asset)
    {
        std::ostringstream str;
        str << "Loader_" << asset->m_definition->m_name;
        return str.str();
    }

    static std::string MarkerClassName(StructureInformation* asset)
    {
        std::ostringstream str;
        str << "Marker_" << asset->m_definition->m_name;
        return str.str();
    }

    static std::string VariableDecl(const DataDefinition* def)
    {
        std::ostringstream str;
        str << def->GetFullName() << "* var" << MakeSafeTypeName(def) << ";";
        return str.str();
    }

    static std::string PointerVariableDecl(const DataDefinition* def)
    {
        std::ostringstream str;
        str << def->GetFullName() << "** var" << MakeSafeTypeName(def) << "Ptr;";
        return str.str();
    }

    void PrintHeaderPtrArrayLoadMethodDeclaration(const DataDefinition* def) const
    {
        LINE("void LoadPtrArray_" << MakeSafeTypeName(def) << "(bool atStreamStart, size_t count);")
    }

    void PrintHeaderArrayLoadMethodDeclaration(const DataDefinition* def) const
    {
        LINE("void LoadArray_" << MakeSafeTypeName(def) << "(bool atStreamStart, size_t count);")
    }

    void PrintHeaderLoadMethodDeclaration(const StructureInformation* info) const
    {
        LINE("void Load_" << MakeSafeTypeName(info->m_definition) << "(bool atStreamStart);")
    }

    void PrintHeaderTempPtrLoadMethodDeclaration(const StructureInformation* info) const
    {
        LINE("void LoadPtr_" << MakeSafeTypeName(info->m_definition) << "(bool atStreamStart);")
    }

    void PrintHeaderAssetLoadMethodDeclaration(const StructureInformation* info) const
    {
        LINE("void LoadAsset_" << MakeSafeTypeName(info->m_definition) << "(" << info->m_definition->GetFullName() << "** pAsset);")
    }

    void PrintHeaderGetNameMethodDeclaration(const StructureInformation* info) const
    {
        LINE("static std::string GetAssetName(" << info->m_definition->GetFullName() << "* pAsset);")
    }

    void PrintHeaderMainLoadMethodDeclaration(const StructureInformation* info) const
    {
        LINE("XAssetInfo<" << info->m_definition->GetFullName() << ">* Load(" << info->m_definition->GetFullName() << "** pAsset);")
    }

    void PrintHeaderConstructor() const
    {
        LINE(LoaderClassName(m_env.m_asset) << "(Zone* zone, IZoneInputStream* stream);")
    }

    void PrintVariableInitialization(const DataDefinition* def) const
    {
        LINE("var" << def->m_name << " = nullptr;")
    }

    void PrintPointerVariableInitialization(const DataDefinition* def) const
    {
        LINE("var" << def->m_name << "Ptr = nullptr;")
    }

    void PrintConstructorMethod()
    {
        LINE(LoaderClassName(m_env.m_asset) << "::" << LoaderClassName(m_env.m_asset) << "(Zone* zone, IZoneInputStream* stream)")

        m_intendation++;
        LINE_START(": AssetLoader(" << m_env.m_asset->m_asset_enum_entry->m_name << ", zone, stream)")
        if (m_env.m_has_actions)
        {
            LINE_MIDDLE(", m_actions(zone)")
        }
        LINE_END("")
        m_intendation--;

        LINE("{")
        m_intendation++;

        LINE("m_asset_info = nullptr;")
        PrintVariableInitialization(m_env.m_asset->m_definition);
        PrintPointerVariableInitialization(m_env.m_asset->m_definition);
        LINE("")

        for (auto* type : m_env.m_used_types)
        {
            if (type->m_info && !type->m_info->m_definition->m_anonymous && !type->m_info->m_is_leaf && !StructureComputations(type->m_info).IsAsset())
            {
                PrintVariableInitialization(type->m_type);
            }
        }
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_info && type->m_pointer_array_reference_exists && !type->m_is_context_asset)
            {
                PrintPointerVariableInitialization(type->m_type);
            }
        }

        m_intendation--;
        LINE("}")
    }

    void PrintLoadPtrArrayMethod_Loading(const DataDefinition* def, StructureInformation* info) const
    {
        LINE("*" << MakeTypePtrVarName(def) << " = m_stream->Alloc<" << def->GetFullName() << ">(" << def->GetAlignment() << ");")

        if (info && !info->m_is_leaf)
        {
            LINE(MakeTypeVarName(info->m_definition) << " = *" << MakeTypePtrVarName(def) << ";")
            LINE("Load_" << MakeSafeTypeName(def) << "(true);")
        }
        else
        {
            LINE("m_stream->Load<" << def->GetFullName() << ">(*" << MakeTypePtrVarName(def) << ");")
        }
    }

    void PrintLoadPtrArrayMethod_PointerCheck(const DataDefinition* def, StructureInformation* info, const bool reusable)
    {
        LINE("if (*" << MakeTypePtrVarName(def) << ")")
        LINE("{")
        m_intendation++;

        if (info && StructureComputations(info).IsAsset())
        {
            LINE(LoaderClassName(info) << " loader(m_zone, m_stream);")
            LINE("loader.Load(" << MakeTypePtrVarName(def) << ");")
        }
        else
        {
            if (reusable)
            {
                LINE("if(*" << MakeTypePtrVarName(def) << " == PTR_FOLLOWING)")
                LINE("{")
                m_intendation++;

                PrintLoadPtrArrayMethod_Loading(def, info);

                m_intendation--;
                LINE("}")
                LINE("else")
                LINE("{")
                m_intendation++;

                LINE("*" << MakeTypePtrVarName(def) << " = m_stream->ConvertOffsetToPointer(*" << MakeTypePtrVarName(def) << ");")

                m_intendation--;
                LINE("}")
            }
            else
            {
                PrintLoadPtrArrayMethod_Loading(def, info);
            }
        }

        m_intendation--;
        LINE("}")
    }

    void PrintLoadPtrArrayMethod(const DataDefinition* def, StructureInformation* info, const bool reusable)
    {
        LINE("void " << LoaderClassName(m_env.m_asset) << "::LoadPtrArray_" << MakeSafeTypeName(def) << "(const bool atStreamStart, const size_t count)")
        LINE("{")
        m_intendation++;

        LINE("assert(" << MakeTypePtrVarName(def) << " != nullptr);")
        LINE("")

        LINE("if(atStreamStart)")
        m_intendation++;
        LINE("m_stream->Load<" << def->GetFullName() << "*>(" << MakeTypePtrVarName(def) << ", count);")
        m_intendation--;

        LINE("")
        LINE(def->GetFullName() << "** var = " << MakeTypePtrVarName(def) << ";")
        LINE("for(size_t index = 0; index < count; index++)")
        LINE("{")
        m_intendation++;

        LINE(MakeTypePtrVarName(def) << " = var;")
        PrintLoadPtrArrayMethod_PointerCheck(def, info, reusable);
        LINE("")
        LINE("var++;")

        m_intendation--;
        LINE("}")
        m_intendation--;
        LINE("}")
    }

    void PrintLoadArrayMethod(const DataDefinition* def, StructureInformation* info)
    {
        LINE("void " << LoaderClassName(m_env.m_asset) << "::LoadArray_" << MakeSafeTypeName(def) << "(const bool atStreamStart, const size_t count)")
        LINE("{")
        m_intendation++;

        LINE("assert(" << MakeTypeVarName(def) << " != nullptr);")
        LINE("")
        LINE("if(atStreamStart)")
        m_intendation++;
        LINE("m_stream->Load<" << def->GetFullName() << ">(" << MakeTypeVarName(def) << ", count);")
        m_intendation--;

        LINE("")
        LINE(def->GetFullName() << "* var = " << MakeTypeVarName(def) << ";")
        LINE("for(size_t index = 0; index < count; index++)")
        LINE("{")
        m_intendation++;

        LINE(MakeTypeVarName(info->m_definition) << " = var;")
        LINE("Load_" << info->m_definition->m_name << "(false);")
        LINE("var++;")

        m_intendation--;
        LINE("}")

        m_intendation--;
        LINE("}")
    }

    void LoadMember_Asset(StructureInformation* info,
                          MemberInformation* member,
                          const DeclarationModifierComputations& modifier,
                          const MemberLoadType loadType) const
    {
        if (loadType == MemberLoadType::SINGLE_POINTER)
        {
            LINE(LoaderClassName(member->m_type) << " loader(m_zone, m_stream);")
            LINE("loader.Load(&" << MakeMemberAccess(info, member, modifier) << ");")
        }
        else if (loadType == MemberLoadType::POINTER_ARRAY)
        {
            LoadMember_PointerArray(info, member, modifier);
        }
        else
        {
            assert(false);
            LINE("#error unsupported loadType " << static_cast<int>(loadType) << " for asset")
        }
    }

    void LoadMember_String(StructureInformation* info,
                           MemberInformation* member,
                           const DeclarationModifierComputations& modifier,
                           const MemberLoadType loadType) const
    {
        if (loadType == MemberLoadType::SINGLE_POINTER)
        {
            if (member->m_member->m_type_declaration->m_is_const)
            {
                LINE("varXString = &" << MakeMemberAccess(info, member, modifier) << ";")
            }
            else
            {
                LINE("varXString = const_cast<const char**>(&" << MakeMemberAccess(info, member, modifier) << ");")
            }
            LINE("LoadXString(false);")
        }
        else if (loadType == MemberLoadType::POINTER_ARRAY)
        {
            LINE("varXString = " << MakeMemberAccess(info, member, modifier) << ";")
            if (modifier.IsArray())
            {
                LINE("LoadXStringArray(false, " << modifier.GetArraySize() << ");")
            }
            else
            {
                LINE("LoadXStringArray(true, " << MakeEvaluation(modifier.GetPointerArrayCountEvaluation()) << ");")
            }
        }
        else
        {
            assert(false);
            LINE("#error unsupported loadType " << static_cast<int>(loadType) << " for string")
        }
    }

    void LoadMember_ArrayPointer(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        const MemberComputations computations(member);
        if (member->m_type && !member->m_type->m_is_leaf && !computations.IsInRuntimeBlock())
        {
            LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")
            LINE("LoadArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true, "
                              << MakeEvaluation(modifier.GetArrayPointerCountEvaluation()) << ");")

            if (member->m_type->m_post_load_action)
            {
                LINE("")
                LINE(MakeCustomActionCall(member->m_type->m_post_load_action.get()))
            }

            if (member->m_post_load_action)
            {
                LINE("")
                LINE(MakeCustomActionCall(member->m_post_load_action.get()))
            }
        }
        else
        {
            LINE("m_stream->Load<" << MakeTypeDecl(member->m_member->m_type_declaration.get())
                                   << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers()) << ">(" << MakeMemberAccess(info, member, modifier)
                                   << ", " << MakeEvaluation(modifier.GetArrayPointerCountEvaluation()) << ");")
        }
    }

    void LoadMember_PointerArray(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        LINE(MakeTypePtrVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")
        if (modifier.IsArray())
        {
            LINE("LoadPtrArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(false, " << modifier.GetArraySize() << ");")
        }
        else
        {
            LINE("LoadPtrArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true, "
                                 << MakeEvaluation(modifier.GetPointerArrayCountEvaluation()) << ");")
        }
    }

    void LoadMember_EmbeddedArray(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        const MemberComputations computations(member);
        std::string arraySizeStr;

        if (modifier.HasDynamicArrayCount())
            arraySizeStr = MakeEvaluation(modifier.GetDynamicArrayCountEvaluation());
        else
            arraySizeStr = std::to_string(modifier.GetArraySize());

        if (!member->m_is_leaf)
        {
            LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")

            if (computations.IsAfterPartialLoad())
            {
                LINE("LoadArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true, " << arraySizeStr << ");")
            }
            else
            {
                LINE("LoadArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(false, " << arraySizeStr << ");")
            }

            if (member->m_type->m_post_load_action)
            {
                LINE("")
                LINE(MakeCustomActionCall(member->m_type->m_post_load_action.get()))
            }

            if (member->m_post_load_action)
            {
                LINE("")
                LINE(MakeCustomActionCall(member->m_post_load_action.get()))
            }
        }
        else if (computations.IsAfterPartialLoad())
        {
            LINE("m_stream->Load<" << MakeTypeDecl(member->m_member->m_type_declaration.get())
                                   << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers()) << ">(" << MakeMemberAccess(info, member, modifier)
                                   << ", " << arraySizeStr << ");")
        }
    }

    void LoadMember_DynamicArray(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        if (member->m_type && !member->m_type->m_is_leaf)
        {
            LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")
            LINE("LoadArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true, "
                              << MakeEvaluation(modifier.GetDynamicArraySizeEvaluation()) << ");")
        }
        else
        {
            LINE("m_stream->Load<" << MakeTypeDecl(member->m_member->m_type_declaration.get())
                                   << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers()) << ">(" << MakeMemberAccess(info, member, modifier)
                                   << ", " << MakeEvaluation(modifier.GetDynamicArraySizeEvaluation()) << ");")
        }
    }

    void LoadMember_Embedded(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        const MemberComputations computations(member);
        if (!member->m_is_leaf)
        {
            LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = &" << MakeMemberAccess(info, member, modifier) << ";")

            if (computations.IsAfterPartialLoad())
            {
                LINE("Load_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true);")
            }
            else
            {
                LINE("Load_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(false);")
            }

            if (member->m_type->m_post_load_action)
            {
                LINE("")
                LINE(MakeCustomActionCall(member->m_type->m_post_load_action.get()))
            }

            if (member->m_post_load_action)
            {
                LINE("")
                LINE(MakeCustomActionCall(member->m_post_load_action.get()))
            }
        }
        else if (computations.IsAfterPartialLoad())
        {
            LINE("m_stream->Load<" << MakeTypeDecl(member->m_member->m_type_declaration.get())
                                   << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers()) << ">(&" << MakeMemberAccess(info, member, modifier)
                                   << ");")
        }
    }

    void LoadMember_SinglePointer(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        const MemberComputations computations(member);
        if (member->m_type && !member->m_type->m_is_leaf && !computations.IsInRuntimeBlock())
        {
            LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")
            LINE("Load_" << MakeSafeTypeName(member->m_type->m_definition) << "(true);")

            if (member->m_type->m_post_load_action)
            {
                LINE("")
                LINE(MakeCustomActionCall(member->m_type->m_post_load_action.get()))
            }

            if (member->m_post_load_action)
            {
                LINE("")
                LINE(MakeCustomActionCall(member->m_post_load_action.get()))
            }
        }
        else
        {
            LINE("m_stream->Load<" << MakeTypeDecl(member->m_member->m_type_declaration.get())
                                   << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers()) << ">(" << MakeMemberAccess(info, member, modifier)
                                   << ");")
        }
    }

    void LoadMember_TypeCheck(StructureInformation* info,
                              MemberInformation* member,
                              const DeclarationModifierComputations& modifier,
                              const MemberLoadType loadType) const
    {
        if (member->m_is_string)
        {
            LoadMember_String(info, member, modifier, loadType);
        }
        else if (member->m_type && StructureComputations(member->m_type).IsAsset())
        {
            LoadMember_Asset(info, member, modifier, loadType);
        }
        else
        {
            switch (loadType)
            {
            case MemberLoadType::ARRAY_POINTER:
                LoadMember_ArrayPointer(info, member, modifier);
                break;

            case MemberLoadType::SINGLE_POINTER:
                LoadMember_SinglePointer(info, member, modifier);
                break;

            case MemberLoadType::EMBEDDED:
                LoadMember_Embedded(info, member, modifier);
                break;

            case MemberLoadType::POINTER_ARRAY:
                LoadMember_PointerArray(info, member, modifier);
                break;

            case MemberLoadType::DYNAMIC_ARRAY:
                LoadMember_DynamicArray(info, member, modifier);
                break;

            case MemberLoadType::EMBEDDED_ARRAY:
                LoadMember_EmbeddedArray(info, member, modifier);
                break;

            default:
                LINE("// t=" << static_cast<int>(loadType))
                break;
            }
        }
    }

    static bool LoadMember_ShouldMakeAlloc(StructureInformation* info,
                                           MemberInformation* member,
                                           const DeclarationModifierComputations& modifier,
                                           const MemberLoadType loadType)
    {
        if (loadType != MemberLoadType::ARRAY_POINTER && loadType != MemberLoadType::POINTER_ARRAY && loadType != MemberLoadType::SINGLE_POINTER)
        {
            return false;
        }

        if (loadType == MemberLoadType::POINTER_ARRAY)
        {
            return !modifier.IsArray();
        }

        if (member->m_is_string)
        {
            return false;
        }

        if (member->m_type && StructureComputations(member->m_type).IsAsset())
        {
            return false;
        }

        return true;
    }

    void LoadMember_Alloc(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberLoadType loadType)
    {
        if (!LoadMember_ShouldMakeAlloc(info, member, modifier, loadType))
        {
            LoadMember_TypeCheck(info, member, modifier, loadType);
            return;
        }

        const MemberComputations computations(member);
        if (computations.IsInTempBlock())
        {
            LINE(member->m_member->m_type_declaration->m_type->GetFullName() << "* ptr = " << MakeMemberAccess(info, member, modifier) << ";")
        }

        const auto typeDecl = MakeTypeDecl(member->m_member->m_type_declaration.get());
        const auto followingReferences = MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers());

        // This used to use `alignof()` to calculate alignment but due to inconsistencies between compilers and bugs discovered in MSVC
        // (Alignment specified via `__declspec(align())` showing as correct via intellisense but is incorrect when compiled for types that have a larger
        // alignment than the specified value) this was changed to make ZoneCodeGenerator calculate what is supposed to be used as alignment when allocating.
        // This is more reliable when being used with different compilers and the value used can be seen in the source code directly
        if (member->m_alloc_alignment)
        {
            LINE(MakeMemberAccess(info, member, modifier)
                 << " = m_stream->Alloc<" << typeDecl << followingReferences << ">(" << MakeEvaluation(member->m_alloc_alignment.get()) << ");")
        }
        else
        {
            LINE(MakeMemberAccess(info, member, modifier)
                 << " = m_stream->Alloc<" << typeDecl << followingReferences << ">(" << modifier.GetAlignment() << ");")
        }

        if (computations.IsInTempBlock())
        {
            LINE("")
            LINE(member->m_member->m_type_declaration->m_type->GetFullName() << "** toInsert = nullptr;")
            LINE("if(ptr == PTR_INSERT)")
            m_intendation++;
            LINE("toInsert = m_stream->InsertPointer<" << member->m_member->m_type_declaration->m_type->GetFullName() << ">();")
            m_intendation--;
            LINE("")
        }

        LoadMember_TypeCheck(info, member, modifier, loadType);

        if (computations.IsInTempBlock())
        {
            LINE("")
            LINE("if(toInsert != nullptr)")
            m_intendation++;
            LINE("*toInsert = " << MakeTypeVarName(info->m_definition) << "->" << member->m_member->m_name << ";")
            m_intendation--;
        }
    }

    static bool LoadMember_ShouldMakeReuse(StructureInformation* info,
                                           MemberInformation* member,
                                           const DeclarationModifierComputations& modifier,
                                           const MemberLoadType loadType)
    {
        if (loadType != MemberLoadType::ARRAY_POINTER && loadType != MemberLoadType::SINGLE_POINTER && loadType != MemberLoadType::POINTER_ARRAY)
        {
            return false;
        }

        if (loadType == MemberLoadType::POINTER_ARRAY && modifier.IsArray())
        {
            return false;
        }

        return true;
    }

    void LoadMember_Reuse(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberLoadType loadType)
    {
        if (!LoadMember_ShouldMakeReuse(info, member, modifier, loadType) || !member->m_is_reusable)
        {
            LoadMember_Alloc(info, member, modifier, loadType);
            return;
        }

        const MemberComputations computations(member);
        if (computations.IsInTempBlock())
        {
            LINE("if(" << MakeMemberAccess(info, member, modifier) << " == PTR_FOLLOWING || " << MakeMemberAccess(info, member, modifier) << " == PTR_INSERT)")
            LINE("{")
            m_intendation++;

            LoadMember_Alloc(info, member, modifier, loadType);

            m_intendation--;
            LINE("}")
            LINE("else")
            LINE("{")
            m_intendation++;

            LINE(MakeMemberAccess(info, member, modifier) << " = m_stream->ConvertOffsetToAlias(" << MakeMemberAccess(info, member, modifier) << ");")

            m_intendation--;
            LINE("}")
        }
        else
        {
            LINE("if(" << MakeMemberAccess(info, member, modifier) << " == PTR_FOLLOWING)")
            LINE("{")
            m_intendation++;

            LoadMember_Alloc(info, member, modifier, loadType);

            m_intendation--;
            LINE("}")
            LINE("else")
            LINE("{")
            m_intendation++;

            LINE(MakeMemberAccess(info, member, modifier) << " = m_stream->ConvertOffsetToPointer(" << MakeMemberAccess(info, member, modifier) << ");")

            m_intendation--;
            LINE("}")
        }
    }

    static bool LoadMember_ShouldMakePointerCheck(StructureInformation* info,
                                                  MemberInformation* member,
                                                  const DeclarationModifierComputations& modifier,
                                                  MemberLoadType loadType)
    {
        if (loadType != MemberLoadType::ARRAY_POINTER && loadType != MemberLoadType::POINTER_ARRAY && loadType != MemberLoadType::SINGLE_POINTER)
        {
            return false;
        }

        if (loadType == MemberLoadType::POINTER_ARRAY)
        {
            return !modifier.IsArray();
        }

        if (member->m_is_string)
        {
            return false;
        }

        return true;
    }

    void LoadMember_PointerCheck(StructureInformation* info,
                                 MemberInformation* member,
                                 const DeclarationModifierComputations& modifier,
                                 const MemberLoadType loadType)
    {
        if (LoadMember_ShouldMakePointerCheck(info, member, modifier, loadType))
        {
            LINE("if (" << MakeMemberAccess(info, member, modifier) << ")")
            LINE("{")
            m_intendation++;

            LoadMember_Reuse(info, member, modifier, loadType);

            m_intendation--;
            LINE("}")
        }
        else
        {
            LoadMember_Reuse(info, member, modifier, loadType);
        }
    }

    void LoadMember_Block(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, MemberLoadType loadType)
    {
        const MemberComputations computations(member);

        const auto notInDefaultNormalBlock = computations.IsNotInDefaultNormalBlock();
        if (notInDefaultNormalBlock)
        {
            LINE("m_stream->PushBlock(" << member->m_fast_file_block->m_name << ");")
        }

        LoadMember_PointerCheck(info, member, modifier, loadType);

        if (notInDefaultNormalBlock)
        {
            LINE("m_stream->PopBlock();")
        }
    }

    void LoadMember_ReferenceArray(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier)
    {
        auto first = true;
        for (const auto& entry : modifier.GetArrayEntries())
        {
            if (first)
            {
                first = false;
            }
            else
            {
                LINE("")
            }

            LoadMember_Reference(info, member, entry);
        }
    }

    void LoadMember_Reference(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier)
    {
        if (modifier.IsDynamicArray())
        {
            LoadMember_Block(info, member, modifier, MemberLoadType::DYNAMIC_ARRAY);
        }
        else if (modifier.IsSinglePointer())
        {
            LoadMember_Block(info, member, modifier, MemberLoadType::SINGLE_POINTER);
        }
        else if (modifier.IsArrayPointer())
        {
            LoadMember_Block(info, member, modifier, MemberLoadType::ARRAY_POINTER);
        }
        else if (modifier.IsPointerArray())
        {
            LoadMember_Block(info, member, modifier, MemberLoadType::POINTER_ARRAY);
        }
        else if (modifier.IsArray() && modifier.GetNextDeclarationModifier() == nullptr)
        {
            LoadMember_Block(info, member, modifier, MemberLoadType::EMBEDDED_ARRAY);
        }
        else if (modifier.GetDeclarationModifier() == nullptr)
        {
            LoadMember_Block(info, member, modifier, MemberLoadType::EMBEDDED);
        }
        else if (modifier.IsArray())
        {
            LoadMember_ReferenceArray(info, member, modifier);
        }
        else
        {
            assert(false);
            LINE("#error LoadMemberReference failed @ " << member->m_member->m_name)
        }
    }

    void LoadMember_Condition_Struct(StructureInformation* info, MemberInformation* member)
    {
        LINE("")
        if (member->m_condition)
        {
            LINE("if(" << MakeEvaluation(member->m_condition.get()) << ")")
            LINE("{")
            m_intendation++;

            LoadMember_Reference(info, member, DeclarationModifierComputations(member));

            m_intendation--;
            LINE("}")
        }
        else
        {
            LoadMember_Reference(info, member, DeclarationModifierComputations(member));
        }
    }

    void LoadMember_Condition_Union(StructureInformation* info, MemberInformation* member)
    {
        const MemberComputations computations(member);

        if (computations.IsFirstMember())
        {
            LINE("")
            if (member->m_condition)
            {
                LINE("if(" << MakeEvaluation(member->m_condition.get()) << ")")
                LINE("{")
                m_intendation++;

                LoadMember_Reference(info, member, DeclarationModifierComputations(member));

                m_intendation--;
                LINE("}")
            }
            else
            {
                LoadMember_Reference(info, member, DeclarationModifierComputations(member));
            }
        }
        else if (computations.IsLastMember())
        {
            if (member->m_condition)
            {
                LINE("else if(" << MakeEvaluation(member->m_condition.get()) << ")")
                LINE("{")
                m_intendation++;

                LoadMember_Reference(info, member, DeclarationModifierComputations(member));

                m_intendation--;
                LINE("}")
            }
            else
            {
                LINE("else")
                LINE("{")
                m_intendation++;

                LoadMember_Reference(info, member, DeclarationModifierComputations(member));

                m_intendation--;
                LINE("}")
            }
        }
        else
        {
            if (member->m_condition)
            {
                LINE("else if(" << MakeEvaluation(member->m_condition.get()) << ")")
                LINE("{")
                m_intendation++;

                LoadMember_Reference(info, member, DeclarationModifierComputations(member));

                m_intendation--;
                LINE("}")
            }
            else
            {
                LINE("#error Middle member of union must have condition (" << member->m_member->m_name << ")")
            }
        }
    }

    void PrintLoadMemberIfNeedsTreatment(StructureInformation* info, MemberInformation* member)
    {
        const MemberComputations computations(member);
        if (computations.ShouldIgnore())
            return;

        if (member->m_is_string || computations.ContainsNonEmbeddedReference() || member->m_type && !member->m_type->m_is_leaf
            || computations.IsAfterPartialLoad())
        {
            if (info->m_definition->GetType() == DataDefinitionType::UNION)
                LoadMember_Condition_Union(info, member);
            else
                LoadMember_Condition_Struct(info, member);
        }
    }

    void PrintLoadMethod(StructureInformation* info)
    {
        const StructureComputations computations(info);
        LINE("void " << LoaderClassName(m_env.m_asset) << "::Load_" << info->m_definition->m_name << "(const bool atStreamStart)")
        LINE("{")
        m_intendation++;

        LINE("assert(" << MakeTypeVarName(info->m_definition) << " != nullptr);")

        auto* dynamicMember = computations.GetDynamicMember();
        if (!(info->m_definition->GetType() == DataDefinitionType::UNION && dynamicMember))
        {
            LINE("")
            LINE("if(atStreamStart)")
            m_intendation++;

            if (dynamicMember == nullptr)
            {
                LINE("m_stream->Load<" << info->m_definition->GetFullName() << ">(" << MakeTypeVarName(info->m_definition)
                                       << "); // Size: " << info->m_definition->GetSize())
            }
            else
            {
                LINE("m_stream->LoadPartial<" << info->m_definition->GetFullName() << ">(" << MakeTypeVarName(info->m_definition) << ", offsetof("
                                              << info->m_definition->GetFullName() << ", " << dynamicMember->m_member->m_name << "));")
            }

            m_intendation--;
        }
        else
        {
            LINE("assert(atStreamStart);")
        }

        if (computations.IsAsset())
        {
            LINE("")
            LINE("m_stream->PushBlock(" << m_env.m_default_normal_block->m_name << ");")
        }
        else if (info->m_block)
        {
            LINE("")
            LINE("m_stream->PushBlock(" << info->m_block->m_name << ");")
        }

        for (const auto& member : info->m_ordered_members)
        {
            PrintLoadMemberIfNeedsTreatment(info, member.get());
        }

        if (info->m_block || computations.IsAsset())
        {
            LINE("")
            LINE("m_stream->PopBlock();")
        }

        m_intendation--;
        LINE("}")
    }

    void PrintLoadPtrMethod(StructureInformation* info)
    {
        const bool inTemp = info->m_block && info->m_block->m_type == FastFileBlockType::TEMP;
        LINE("void " << LoaderClassName(m_env.m_asset) << "::LoadPtr_" << MakeSafeTypeName(info->m_definition) << "(const bool atStreamStart)")
        LINE("{")
        m_intendation++;

        LINE("assert(" << MakeTypePtrVarName(info->m_definition) << " != nullptr);")
        LINE("")

        LINE("if(atStreamStart)")
        m_intendation++;
        LINE("m_stream->Load<" << info->m_definition->GetFullName() << "*>(" << MakeTypePtrVarName(info->m_definition) << ");")
        m_intendation--;

        LINE("")
        if (inTemp)
        {
            LINE("m_stream->PushBlock(" << m_env.m_default_temp_block->m_name << ");")
            LINE("")
        }

        LINE("if(*" << MakeTypePtrVarName(info->m_definition) << " != nullptr)")
        LINE("{")
        m_intendation++;

        if (inTemp)
        {
            LINE("if(*" << MakeTypePtrVarName(info->m_definition) << " == PTR_FOLLOWING || *" << MakeTypePtrVarName(info->m_definition) << " == PTR_INSERT)")
        }
        else
        {
            LINE("if(*" << MakeTypePtrVarName(info->m_definition) << " == PTR_FOLLOWING)")
        }
        LINE("{")
        m_intendation++;

        if (inTemp)
        {
            LINE(info->m_definition->GetFullName() << "* ptr = *" << MakeTypePtrVarName(info->m_definition) << ";")
        }
        LINE("*" << MakeTypePtrVarName(info->m_definition) << " = m_stream->Alloc<" << info->m_definition->GetFullName() << ">("
                 << info->m_definition->GetAlignment() << ");")

        if (inTemp)
        {
            LINE("")
            LINE(info->m_definition->GetFullName() << "** toInsert = nullptr;")
            LINE("if(ptr == PTR_INSERT)")
            m_intendation++;
            LINE("toInsert = m_stream->InsertPointer<" << info->m_definition->GetFullName() << ">();")
            m_intendation--;
        }

        auto startLoadSection = true;

        if (!info->m_is_leaf)
        {
            if (startLoadSection)
            {
                startLoadSection = false;
                LINE("")
            }
            LINE(MakeTypeVarName(info->m_definition) << " = *" << MakeTypePtrVarName(info->m_definition) << ";")
            LINE("Load_" << MakeSafeTypeName(info->m_definition) << "(true);")
        }
        else
        {
            LINE("#error Ptr method cannot have leaf type")
        }

        if (info->m_post_load_action)
        {
            LINE("")
            LINE(MakeCustomActionCall(info->m_post_load_action.get()))
        }

        if (StructureComputations(info).IsAsset())
        {
            LINE("")
            LINE("LoadAsset_" << MakeSafeTypeName(info->m_definition) << "(" << MakeTypePtrVarName(info->m_definition) << ");")
        }

        if (inTemp)
        {
            if (!startLoadSection)
            {
                LINE("")
            }

            LINE("if(toInsert != nullptr)")
            m_intendation++;
            LINE("*toInsert = *" << MakeTypePtrVarName(info->m_definition) << ";")
            m_intendation--;
        }

        m_intendation--;
        LINE("}")
        LINE("else")
        LINE("{")
        m_intendation++;

        if (inTemp)
        {
            LINE("*" << MakeTypePtrVarName(info->m_definition) << " = m_stream->ConvertOffsetToAlias(*" << MakeTypePtrVarName(info->m_definition) << ");")
        }
        else
        {
            LINE("*" << MakeTypePtrVarName(info->m_definition) << " = m_stream->ConvertOffsetToPointer(*" << MakeTypePtrVarName(info->m_definition) << ");")
        }

        m_intendation--;
        LINE("}")

        m_intendation--;
        LINE("}")

        if (inTemp)
        {
            LINE("")
            LINE("m_stream->PopBlock();")
        }

        m_intendation--;
        LINE("}")
    }

    void PrintLoadAssetMethod(StructureInformation* info)
    {
        LINE("void " << LoaderClassName(m_env.m_asset) << "::LoadAsset_" << MakeSafeTypeName(info->m_definition) << "(" << info->m_definition->GetFullName()
                     << "** pAsset)")
        LINE("{")
        m_intendation++;

        LINE("assert(pAsset != nullptr);")
        LINE("")
        LINE(MarkerClassName(m_env.m_asset) << " marker(m_zone);")
        LINE("marker.Mark(*pAsset);")
        LINE("")
        LINE("auto* reallocatedAsset = m_zone->GetMemory()->Alloc<" << info->m_definition->GetFullName() << ">();")
        LINE("std::memcpy(reallocatedAsset, *pAsset, sizeof(" << info->m_definition->GetFullName() << "));")
        LINE("")
        LINE("m_asset_info = reinterpret_cast<XAssetInfo<" << info->m_definition->GetFullName()
                                                           << ">*>(LinkAsset(GetAssetName(*pAsset), reallocatedAsset, marker.GetDependencies(), "
                                                              "marker.GetUsedScriptStrings(), marker.GetIndirectAssetReferences()));")
        LINE("*pAsset = m_asset_info->Asset();")

        m_intendation--;
        LINE("}")
    }

    void PrintMainLoadMethod()
    {
        LINE("XAssetInfo<" << m_env.m_asset->m_definition->GetFullName() << ">* " << LoaderClassName(m_env.m_asset) << "::Load("
                           << m_env.m_asset->m_definition->GetFullName() << "** pAsset)")
        LINE("{")
        m_intendation++;

        LINE("assert(pAsset != nullptr);")
        LINE("")
        LINE("m_asset_info = nullptr;")
        LINE("")
        LINE(MakeTypePtrVarName(m_env.m_asset->m_definition) << " = pAsset;")
        LINE("LoadPtr_" << MakeSafeTypeName(m_env.m_asset->m_definition) << "(false);")
        LINE("")
        LINE("if(m_asset_info == nullptr && *pAsset != nullptr)")
        m_intendation++;
        LINE("m_asset_info = reinterpret_cast<XAssetInfo<" << m_env.m_asset->m_definition->GetFullName() << ">*>(GetAssetInfo(GetAssetName(*pAsset)));")
        m_intendation--;
        LINE("")
        LINE("return m_asset_info;")

        m_intendation--;
        LINE("}")
    }

    void PrintGetNameMethod()
    {
        LINE("std::string " << LoaderClassName(m_env.m_asset) << "::GetAssetName(" << m_env.m_asset->m_definition->GetFullName() << "* pAsset)")
        LINE("{")
        m_intendation++;

        if (!m_env.m_asset->m_name_chain.empty())
        {
            LINE_START("return pAsset")

            auto first = true;
            for (auto* member : m_env.m_asset->m_name_chain)
            {
                if (first)
                {
                    first = false;
                    LINE_MIDDLE("->" << member->m_member->m_name)
                }
                else
                {
                    LINE_MIDDLE("." << member->m_member->m_name)
                }
            }
            LINE_END(";")
        }
        else
        {
            LINE("return \"" << m_env.m_asset->m_definition->m_name << "\";")
        }

        m_intendation--;
        LINE("}")
    }

public:
    Internal(std::ostream& stream, RenderingContext* context)
        : BaseTemplate(stream, context)
    {
    }

    void Header()
    {
        LINE("// ====================================================================")
        LINE("// This file has been generated by ZoneCodeGenerator.")
        LINE("// Do not modify. ")
        LINE("// Any changes will be discarded when regenerating.")
        LINE("// ====================================================================")
        LINE("")
        LINE("#pragma once")
        LINE("")
        LINE("#include \"Loading/AssetLoader.h\"")
        LINE("#include \"Game/" << m_env.m_game << "/" << m_env.m_game << ".h\"")
        if (m_env.m_has_actions)
        {
            LINE("#include \"Game/" << m_env.m_game << "/XAssets/" << Lower(m_env.m_asset->m_definition->m_name) << "/"
                                    << Lower(m_env.m_asset->m_definition->m_name) << "_actions.h\"")
        }
        LINE("#include <string>")
        LINE("")
        LINE("namespace " << m_env.m_game)
        LINE("{")
        m_intendation++;
        LINE("class " << LoaderClassName(m_env.m_asset) << " final : public AssetLoader")
        LINE("{")
        m_intendation++;

        LINE("XAssetInfo<" << m_env.m_asset->m_definition->GetFullName() << ">* m_asset_info;")
        if (m_env.m_has_actions)
        {
            LINE("Actions_" << m_env.m_asset->m_definition->m_name << " m_actions;")
        }
        LINE(VariableDecl(m_env.m_asset->m_definition))
        LINE(PointerVariableDecl(m_env.m_asset->m_definition))
        LINE("")

        // Variable Declarations: type varType;
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_info && !type->m_info->m_definition->m_anonymous && !type->m_info->m_is_leaf && !StructureComputations(type->m_info).IsAsset())
            {
                LINE(VariableDecl(type->m_type))
            }
        }
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_pointer_array_reference_exists && !type->m_is_context_asset)
            {
                LINE(PointerVariableDecl(type->m_type))
            }
        }

        LINE("")

        // Method Declarations
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_pointer_array_reference_exists)
            {
                PrintHeaderPtrArrayLoadMethodDeclaration(type->m_type);
            }
        }
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_array_reference_exists && type->m_info && !type->m_info->m_is_leaf && type->m_non_runtime_reference_exists)
            {
                PrintHeaderArrayLoadMethodDeclaration(type->m_type);
            }
        }
        for (auto* type : m_env.m_used_structures)
        {
            if (type->m_non_runtime_reference_exists && !type->m_info->m_is_leaf && !StructureComputations(type->m_info).IsAsset())
            {
                PrintHeaderLoadMethodDeclaration(type->m_info);
            }
        }
        PrintHeaderLoadMethodDeclaration(m_env.m_asset);
        PrintHeaderTempPtrLoadMethodDeclaration(m_env.m_asset);
        PrintHeaderAssetLoadMethodDeclaration(m_env.m_asset);
        LINE("")
        m_intendation--;
        LINE("public:")
        m_intendation++;
        PrintHeaderConstructor();
        PrintHeaderMainLoadMethodDeclaration(m_env.m_asset);
        PrintHeaderGetNameMethodDeclaration(m_env.m_asset);

        m_intendation--;
        LINE("};")
        m_intendation--;
        LINE("}")
    }

    void Source()
    {
        LINE("// ====================================================================")
        LINE("// This file has been generated by ZoneCodeGenerator.")
        LINE("// Do not modify. ")
        LINE("// Any changes will be discarded when regenerating.")
        LINE("// ====================================================================")
        LINE("")
        LINE("#include \"" << Lower(m_env.m_asset->m_definition->m_name) << "_load_db.h\"")
        LINE("#include \"" << Lower(m_env.m_asset->m_definition->m_name) << "_mark_db.h\"")
        LINE("#include <cassert>")
        LINE("#include <cstring>")
        LINE("")

        if (!m_env.m_referenced_assets.empty())
        {
            LINE("// Referenced Assets:")
            for (auto* type : m_env.m_referenced_assets)
            {
                LINE("#include \"../" << Lower(type->m_type->m_name) << "/" << Lower(type->m_type->m_name) << "_load_db.h\"")
            }
            LINE("")
        }
        LINE("using namespace " << m_env.m_game << ";")
        LINE("")
        PrintConstructorMethod();

        for (auto* type : m_env.m_used_types)
        {
            if (type->m_pointer_array_reference_exists)
            {
                LINE("")
                PrintLoadPtrArrayMethod(type->m_type, type->m_info, type->m_pointer_array_reference_is_reusable);
            }
        }
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_array_reference_exists && type->m_info && !type->m_info->m_is_leaf && type->m_non_runtime_reference_exists)
            {
                LINE("")
                PrintLoadArrayMethod(type->m_type, type->m_info);
            }
        }
        for (auto* type : m_env.m_used_structures)
        {
            if (type->m_non_runtime_reference_exists && !type->m_info->m_is_leaf && !StructureComputations(type->m_info).IsAsset())
            {
                LINE("")
                PrintLoadMethod(type->m_info);
            }
        }
        LINE("")
        PrintLoadMethod(m_env.m_asset);
        LINE("")
        PrintLoadPtrMethod(m_env.m_asset);
        LINE("")
        PrintLoadAssetMethod(m_env.m_asset);
        LINE("")
        PrintMainLoadMethod();
        LINE("")
        PrintGetNameMethod();
    }
};

std::vector<CodeTemplateFile> ZoneLoadTemplate::GetFilesToRender(RenderingContext* context)
{
    std::vector<CodeTemplateFile> files;

    auto assetName = context->m_asset->m_definition->m_name;
    for (auto& c : assetName)
        c = static_cast<char>(tolower(c));

    {
        std::ostringstream str;
        str << assetName << '/' << assetName << "_load_db.h";
        files.emplace_back(str.str(), TAG_HEADER);
    }

    {
        std::ostringstream str;
        str << assetName << '/' << assetName << "_load_db.cpp";
        files.emplace_back(str.str(), TAG_SOURCE);
    }

    return files;
}

void ZoneLoadTemplate::RenderFile(std::ostream& stream, const int fileTag, RenderingContext* context)
{
    Internal internal(stream, context);

    if (fileTag == TAG_HEADER)
    {
        internal.Header();
    }
    else if (fileTag == TAG_SOURCE)
    {
        internal.Source();
    }
    else
    {
        std::cout << "Unknown tag for ZoneLoadTemplate: " << fileTag << "\n";
    }
}
