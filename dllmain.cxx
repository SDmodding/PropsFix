#define SDK_SD_MINIMAL_BUILD
#include <SDK/_Includes.hpp>
#include <SDK/Libs/MinHook.h>
#pragma comment(lib, "SDK/Libs/MinHook.lib")

//==================================================================================

namespace UFG
{
    namespace SceneObjectProperties
    {
        typedef CSceneObjectProperties*(__fastcall* Fn_Create)(qSymbol*, void*, qPropertySet*);
        Fn_Create g_Create;

        CSceneObjectProperties* __fastcall Create(qSymbol* p_ObjName, void* p_SceneLayer, qPropertySet* p_ParentPropertySet)
        {
            CSceneObjectProperties* _SceneObjectProperties = g_Create(p_ObjName, p_SceneLayer, p_ParentPropertySet);
            switch (p_ParentPropertySet->mName)
            {
                // Police
                case 0x34317ACA: // A
                case 0x39725C13: // B
                case 0x3DB341A4: // C
                case 0x123CA7F1: // Buddy (DLC)
                case 0x0C02DD39: // Old
                case 0xEA57FD95: // Young
                {
                    qPropertySet* _WritablePropertySet = _SceneObjectProperties->GetWritableProperties();
                    if (!_WritablePropertySet) {
                        break;
                    }
                    qPropertyList* _PropList = qPropertyList::Create();
                    if (_PropList)
                    {
                        _PropList->AddSymbol(0x804388F3);
                        _WritablePropertySet->SetPropertyList(0x1EAE8392, _PropList);
                    }

                }
                break;
            }

            return _SceneObjectProperties;
        }
    }

    typedef void(__fastcall* Fn_CreateAndAttachProp)(qPropertySet*, CSimObject*, bool);
    Fn_CreateAndAttachProp g_CreateAndAttachProp;

    void __fastcall CreateAndAttachProp(qPropertySet* p_PropertySet, CSimObject* p_SimObjectToAttach, bool p_DeletePhysicsFails)
    {
        static constexpr uint32_t _AttachOffsetNameUID = 0x687FFFF3;

        uint32_t* _AttachOffsetNameUIDPtr = nullptr;
        qSymbol* _AttachOffsetPtr = p_PropertySet->GetSymbol(_AttachOffsetNameUID);
        if (_AttachOffsetPtr) 
        {
            CStreamedResourceComponent* m_StreamedResource = p_SimObjectToAttach->GetComponentOfType<CStreamedResourceComponent>(SimObjectStreamedResource_TypeUID);
            if (m_StreamedResource && !m_StreamedResource->GetAssetPropertySet(*_AttachOffsetPtr))
            {
                qProperty* m_Properties = p_PropertySet->mProperties.GetPointer();
                for (uint16_t i = 0; p_PropertySet->mNumProperties > i; ++i)
                {
                    qProperty* m_Property = &m_Properties[i];
                    if (m_Property->mNameUID == _AttachOffsetNameUID)
                    {
                        _AttachOffsetNameUIDPtr = &m_Property->mNameUID;
                        *_AttachOffsetNameUIDPtr = UINT32_MAX; // We modify the NameUID so the game fails to find it and spawn the prop even without knowing the offset for current model.
                        break;
                    }
                }
            }
        }
        
        g_CreateAndAttachProp(p_PropertySet, p_SimObjectToAttach, false);

        if (_AttachOffsetNameUIDPtr) {
            *_AttachOffsetNameUIDPtr = _AttachOffsetNameUID;
        }
    }
    typedef void(__fastcall* Fn_LoadGlobalPropertySets)();
    Fn_LoadGlobalPropertySets g_LoadGlobalPropertySets;

    void __fastcall LoadGlobalPropertySets()
    {
        g_LoadGlobalPropertySets();

        // Simply remove props for those PropertySets...
        qSymbol _PropertySetsToNuke[] = {
            0x729CE849, // PCPolice_A
            0x7FDFCE90, // PCPolice_B
            0x7B1ED327, // PCPolice_C
        };

        for (qSymbol _PropertySetNameUID : _PropertySetsToNuke)
        {
            qPropertySet* _PropertySet = UFG::PropertySetManager::GetPropertySet(_PropertySetNameUID);
            if (!_PropertySet) {
                continue;
            }

            qPropertyList* _PropList = _PropertySet->GetPropertyList(0x1EAE8392);
            if (!_PropList) {
                continue;
            }

            for (uint32_t i = 0; _PropList->mNumElements > i; ++i)
            { 
                qSymbol* _PropNameUID = _PropList->GetSymbol(i);
                if (_PropNameUID) {
                    *_PropNameUID = UINT32_MAX;
                }
            }
        }

        // Changed Prop for correct one...
        qPropertySet* _PoliceAmbient = UFG::PropertySetManager::GetPropertySet(0x309915CE);
        if (_PoliceAmbient)
        {
            qPropertyList* _PropList = _PoliceAmbient->GetPropertyList(0x1EAE8392);
            if (_PropList)
            {
                qSymbol* _PropNameUID = _PropList->GetSymbol(0);
                if (_PropNameUID) {
                    *_PropNameUID = 0x804388F3;
                }
            }
        }
    }
}

//==================================================================================

int __stdcall DllMain(HMODULE p_Module, DWORD p_Reason, void* p_Reserved)
{
    if (p_Reason == DLL_PROCESS_ATTACH)
    {
        if (!SDK::Utils::IsValidExecutable())
        {
            MessageBoxA(0, "This is not valid executable.\nPlease visit: https://github.com/SDmodding \nAnd check README.md", "Props Fix", MB_OK | MB_ICONERROR);
            return 0;
        }

        DisableThreadLibraryCalls(p_Module);

        MH_Initialize();
        MH_CreateHook(UFG_RVA_PTR(0x23A450), UFG::SceneObjectProperties::Create, (void**)&UFG::SceneObjectProperties::g_Create);
        MH_CreateHook(UFG_RVA_PTR(0x523EE0), UFG::CreateAndAttachProp, (void**)&UFG::g_CreateAndAttachProp);
        MH_CreateHook(UFG_RVA_PTR(0x424CA0), UFG::LoadGlobalPropertySets, (void**)&UFG::g_LoadGlobalPropertySets);
    }

    return 1;
}
