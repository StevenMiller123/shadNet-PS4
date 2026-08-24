// Extracted from https://github.com/red-prig/GoldHEN_Plugins_Repository/blob/dmem/plugin_src/dmem/source/main.c

#include <Common.h>

extern size_t Detour_GetInstructionSize(Detour* This, uint64_t Address, size_t MinSize);
extern void Detour_WriteJump64(Detour* This, void* Address, uint64_t Destination);
extern void Detour_WriteJump32(Detour* This, void* Address, uint64_t Destination);

//jmp -7 -> 0xEB 0xF9
uint8_t JumpInstructions16[2] = { 0xEB , 0xF9 };

void Detour_WriteJump16(Detour* This, void* Address) {
    sceKernelMprotect((void*)Address, 2, VM_PROT_ALL);
    memcpy(Address, JumpInstructions16, sizeof(JumpInstructions16));
}

void* Detour_DetourFunction16(Detour* This, uint64_t FunctionPtr, void* HookPtr) {
    if (!FunctionPtr || !HookPtr) {
#if (DEBUG) == 1
        klog("[Detour] %s: FunctionPtr or HookPtr NULL (%p -> %p)\n", __FUNCTION__, (void*)FunctionPtr, HookPtr);
#endif
        return NULL;
    }

    size_t InstructionSize = Detour_GetInstructionSize(This, FunctionPtr, sizeof(JumpInstructions16));

#if (DEBUG) == 1
    klog("[Detour] %s: - InstructionSize: %zu\n", __FUNCTION__, InstructionSize);
#endif

    if (InstructionSize < sizeof(JumpInstructions16)) {
#if (DEBUG) == 1
        klog("[Detour] %s: Hooking Requires a minimum of %d bytes to write jump!\n", __FUNCTION__, (int)sizeof(This->JumpInstructions64));
#endif
        return NULL;
    }

    This->TrampolinePtr = malloc(sizeof(This->JumpInstructions64));

    if (This->TrampolinePtr == 0) {
#if (DEBUG) == 1
        klog("[Detour] %s: malloc failed.\n", __FUNCTION__);
#endif
        return 0;
    }

    Detour_WriteJump64(This, This->TrampolinePtr, (uint64_t)HookPtr);

    // Save Pointers for later
    This->FunctionPtr = (void*)FunctionPtr;
    This->HookPtr = HookPtr;

    // Set protection.
    sceKernelMprotect((void*)(FunctionPtr - 5), InstructionSize + 5, VM_PROT_ALL);

    //Allocate Executable memory for stub and write instructions to stub and a jump back to original execution.
    This->StubSize = (InstructionSize + sizeof(This->JumpInstructions64));

    int res = sceKernelMmap(0, This->StubSize, VM_PROT_ALL, 0x1000 | 0x2, -1, 0, &This->StubPtr);

    if (res < 0 || This->StubPtr == 0) {
#if (DEBUG) == 1
        klog("[Detour] %s: sceKernelMmap failed (0x%X).\n", __FUNCTION__, res);
#endif
        return 0;
    }

    memcpy(This->StubPtr, (void*)FunctionPtr, InstructionSize);
    Detour_WriteJump64(This, (void*)((uint64_t)This->StubPtr + InstructionSize), (uint64_t)(FunctionPtr + InstructionSize));

    //write back jump
    memset((void*)FunctionPtr, 0x90, InstructionSize);
    Detour_WriteJump16(This, (void*)FunctionPtr);

    // Write jump from function to hook.
    Detour_WriteJump32(This, (void*)(FunctionPtr - 5), (uint64_t)This->TrampolinePtr);
    
#if (DEBUG) == 1
    klog("[Detour] %s: Detour Written Successfully! (FunctionPtr: %p - HookPtr: %p - HookPtrTrampoline: %p - StubPtr: %p - StubSize: %zu)\n", __FUNCTION__, This->FunctionPtr, This->HookPtr, This->TrampolinePtr, This->StubPtr, This->StubSize);
#endif

    return This->StubPtr;
};

#define HOOK16(name) do { \
    klog("%s:%d HOOK16() Create " #name "\n", __FUNCTION__, __LINE__);  \
    Detour_Construct( (&(Detour_##name)), DetourMode_x32);                                 \
    Detour_DetourFunction16( (&(Detour_##name)), (uint64_t)name, (void *)(&(name##_hook)) ); \
} while (0)
