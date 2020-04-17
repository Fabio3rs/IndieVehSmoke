// dllmain.cpp : Define o ponto de entrada para o aplicativo DLL.
#include "pch.h"
#include <plugin.h>
#include <plugin_sa/game_sa/CVehicle.h>
#include <saving.hpp>
#include <calling.hpp>
#include <assembly.hpp>
#include <ctime>
#include "GSXAPI.h"

static auto CPool_CVehicle__handleOf = injector::thiscall<int(void*, CAutomobile*)>::call<0x00424160>;
CPool<CAutomobile>* pool = nullptr;

struct CSmoke
{
    union
    {
        struct
        {
            float r, g, b, a;
        };
        float rgba[4];
    } color;

    float size, unknown, lifetime;

    CSmoke()
    {
        std::fill(color.rgba, color.rgba + 4, 1.0f);
        size = 1.0;
        unknown = 1.0;
        lifetime = 1.0;
    }
};

static_assert(sizeof(CSmoke) == 28, "sizeof(CSmoke) is not 28 bytes");

struct cardata
{
    bool customSmoke;
    CSmoke sm;

    cardata()
    {
        customSmoke = false;
    }
};


CPool<cardata> carsPool;

bool carsPoolInited = false;

void callback(const GSX::externalCallbackStructure* test)
{
    using namespace GSX;
    if (test->veh)
    {
        switch (test->status)
        {
        case GSX::LOAD_CAR:
        {
            CSmoke* smk = (CSmoke*)getSavedData(test->veh, "vehCustomSmoke");

            if (smk != nullptr)
            {
                int handle = CPool_CVehicle__handleOf(pool, reinterpret_cast<CAutomobile*>(test->veh));
                int phandle = handle >> 8;

                auto& obj = carsPool.m_pObjects[phandle];

                obj.customSmoke = true;
                obj.sm = *smk;
            }
            break;
        }

        case GSX::SAVE_CAR:
        {
            {
                int handle = CPool_CVehicle__handleOf(pool, reinterpret_cast<CAutomobile*>(test->veh));
                int phandle = handle >> 8;

                auto& obj = carsPool.m_pObjects[phandle];

                if (obj.customSmoke)
                {
                    setDataToSaveLater(test->veh, "vehCustomSmoke", sizeof(obj.sm), std::addressof(obj.sm), true);
                }
            }
            break;
        }

        default:
            break;
        }
    }
}

void initCarsPool()
{
    if (!carsPoolInited)
    {
        pool = *injector::ReadMemory<CPool<CAutomobile>**>(0x0055103C + 1);
        carsPool.m_pObjects = static_cast<cardata*>(operator new(sizeof(cardata) * pool->m_nSize));
        carsPool.m_nSize = pool->m_nSize;
        carsPool.m_nFirstFree = pool->m_nFirstFree;
        carsPool.m_bOwnsAllocations = false;
        carsPool.m_byteMap = pool->m_byteMap;
        carsPoolInited = true;

        for (int i = 0; i < carsPool.m_nSize; i++)
        {
            carsPool.m_pObjects[i].customSmoke = false;
        }

        addNotifyCallback(callback);
    }
}

void poolThings()
{
    initCarsPool();
}

// Test function for set via CLEO script
extern "C" __declspec(dllexport) void addEditPlayerCar(CVehicle *veh, float r, float g, float b, float a, float size, float unk, float lifetime)
{
    poolThings();

    int handle = CPool_CVehicle__handleOf(pool, reinterpret_cast<CAutomobile*>(veh));
    int phandle = handle >> 8;

    cardata &obj = carsPool.m_pObjects[phandle];

    obj.customSmoke = true;
    
    obj.sm.color.r = r;
    obj.sm.color.g = g;
    obj.sm.color.b = b;
    obj.sm.color.a = a;
    obj.sm.size = size;
    obj.sm.unknown = unk;
    obj.sm.lifetime = lifetime;
}

extern "C" __declspec(dllexport) void addEditModelCarID(unsigned int p, float r, float g, float b, float a, float size)
{
    /*auto& use = models[p];

    use.active = true;
    use.r = r;
    use.g = g;
    use.b = b;
    use.a = a;
    use.size = size;*/
}


/*
*/

bool getSetData(CVehicle* veh, CSmoke* smk, float r, float g, float b, float a, float size, float unk, float lifetime)
{
    poolThings();

    int handle = CPool_CVehicle__handleOf(pool, reinterpret_cast<CAutomobile*>(veh));
    int phandle = handle >> 8;

    cardata &obj = carsPool.m_pObjects[phandle];

    if (obj.customSmoke)
    {
        *smk = obj.sm;

        smk->color.a = a * obj.sm.color.a;
        smk->size = size * obj.sm.size;
        smk->unknown = unk * obj.sm.unknown;
        smk->lifetime = lifetime * obj.sm.lifetime;
    }
    else
    {
        // Random for testing purposes
        smk->color.r = r * ((rand() % 10) / 10.0);
        smk->color.g = g * ((rand() % 10) / 10.0);
        smk->color.b = b * ((rand() % 10) / 10.0);
        smk->color.a = a;
        smk->size = size;
        smk->unknown = unk;
        smk->lifetime = lifetime * 2.0;
    }

    return true;
}

/*
*  Patches prototipes
*/
typedef void(__thiscall* FxPrtMult_c__FxPrtMult_t) (CSmoke*, float, float, float, float, float, float, float);
FxPrtMult_c__FxPrtMult_t FxPrtMult_c__FxPrtMult_o = (FxPrtMult_c__FxPrtMult_t)nullptr;

typedef void(__thiscall* ExhaustAddParticles_t) (CVehicle*);
ExhaustAddParticles_t ExhaustAddParticles_o = (ExhaustAddParticles_t)nullptr;

typedef injector::function_hooker_thiscall<injector::scoped_callback, 0x006AB344, void(CVehicle*)> fhook_ExhaustParticles;

CVehicle* regesi;
CSmoke* thisptr;
float r, g, b, a, size, unk, lifetime;

float dummy[3];

void doFxPrtMult_c()
{
    bool bresult = getSetData(regesi, thisptr, r, g, b, a, size, unk, lifetime);

    if (!bresult)
    {
        FxPrtMult_c__FxPrtMult_o(thisptr, r, g, b, a, size, unk, lifetime);
    }
}

/*
* Color RGBA, size, particle lifetime 
*/
extern "C" CSmoke * __fastcall FxPrtMult_c__FxPrtMult_c(CSmoke* tthisptr, int edx, float tr, float tg, float tb, float ta, float tsize, float tunk, float tlifetime)
{
    __asm {
        mov regesi, esi

    }
    /*
    tthisptr->color.r = r;
        tthisptr->color.g = g ;
        tthisptr->color.b = b;
        tthisptr->color.a = a;
        tthisptr->size = size;
        tthisptr->unknown = unk;
        tthisptr->lifetime = lifetime;
    */

    thisptr = tthisptr;
    r = tr;
    g = tg;
    b = tb;
    a = ta;
    size = tsize;
    unk = tunk;
    lifetime = tlifetime;

    __asm
    {
        pushad
        pushfd

        call doFxPrtMult_c

        popfd
        popad

    }
    return tthisptr;
}


/*
* Position set and multi dummy foreach
*/
void __fastcall AddExhaustParticles(CVehicle *veh)
{
    regesi = veh;
    dummy[0] = 1.0;
    dummy[1] = -2.0;
    dummy[2] = 0.0;

    // testing
    for (int i = 0; i < 4; i++)
        ExhaustAddParticles_o(veh);
        /*
        {
            for (int i = 0, size = getNumDummiesForVeh(veh); i < size; i++)
            {
                getExhaustDummyV3d(veh, i, dummy);
                ExhaustAddParticles_o(veh);
            }
        }
        */
}

void hook()
{
    srand(time(0));

    /*
    * Hook CSmoke set
    */
    if (FxPrtMult_c__FxPrtMult_o == nullptr)
        FxPrtMult_c__FxPrtMult_o = (FxPrtMult_c__FxPrtMult_t)(injector::MakeCALL(0x006DE629, injector::memory_pointer_raw(FxPrtMult_c__FxPrtMult_c)).get_raw<void*>());


    /*
    * Hooks for position set and multiple dummy support
    */
    injector::MakeNOP(0x006DE2EA, 0x006DE2F7 - 0x006DE2EA);

    injector::MakeInline<0x006DE2EA>([](injector::reg_pack &regs)
        {
            regs.edx = (uint32_t)std::addressof(dummy);
        }
    );


    if (ExhaustAddParticles_o == nullptr)
        ExhaustAddParticles_o = (ExhaustAddParticles_t)(injector::MakeCALL(0x006AB344, injector::memory_pointer_raw(AddExhaustParticles)).get_raw<void*>());
    
    /////////////////////
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        hook();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

