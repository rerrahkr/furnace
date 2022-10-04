#ifndef _SCCI_MANAGER_H
#define _SCCI_MANAGER_H

#include <memory>
#include <unordered_map>
#include <deque>

#ifdef _WIN32
#include <Windows.h>
#include "../../extern/SCCI/scci.h"
#include "../../extern/SCCI/SCCIDefines.h"
#endif

enum DivSystem;
class DivDispatch;

class SCCIManager
{
    SCCIManager() noexcept;

    static inline std::unique_ptr<SCCIManager> instance_;
    bool hasLoadedLib_;

#ifdef _WIN32
    HMODULE hScci_ = nullptr;
    SoundInterfaceManager *siMan_ = nullptr;
    std::unordered_map<SC_CHIP_TYPE, std::deque<SoundChip *>> unusedSC_;
    std::unordered_map<DivDispatch *, SoundChip *> connections_;
#endif

public:
    ~SCCIManager();
    SCCIManager(const SCCIManager &) = delete;
    SCCIManager &operator=(const SCCIManager &) = delete;
    SCCIManager(SCCIManager &&) = delete;
    SCCIManager &operator=(SCCIManager &&) = delete;

    static SCCIManager &instance();

    bool loadLibrary();
    bool releaseLibrary();

    bool initializeChips();
    bool deinitializeChips();

    bool reset();
    bool hasAttached(DivDispatch *dispath) const;
    bool attach(DivSystem sys, DivDispatch *dispatch);
    bool detach(DivDispatch *dispatch);

    using addr_t = unsigned long;
    using data_t = long long;

    bool write(DivDispatch *dispatch, addr_t addr, data_t data);
};

#endif
