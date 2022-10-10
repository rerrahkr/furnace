#include "scciManager.h"
#include <cassert>
#include "dispatch.h"
#include "song.h"

std::unique_ptr<SCCIManager> SCCIManager::instance_;

SCCIManager& SCCIManager::instance() {
  if (!instance_) {
    instance_.reset(new SCCIManager);
  }
  assert(instance_);
  return *instance_;
}

bool SCCIManager::loadLibrary() {
#ifdef _WIN32
  if (hScci_ && !::FreeLibrary(hScci_)) {
    return false;
  }
#endif

  hScci_=nullptr;
  hasLoadedLib_=false;

#ifdef _WIN32
  hScci_=::LoadLibrary("scci.dll");
  hasLoadedLib_=hScci_!=nullptr;
#endif

  return hasLoadedLib_;
}

bool SCCIManager::releaseLibrary() {
#ifdef _WIN32
  if (hasLoadedLib_) {
    hasLoadedLib_=::FreeLibrary(hScci_);
  }

  if (!hasLoadedLib_) {
    hScci_ = nullptr;
  }
#endif

  return hasLoadedLib_;
}

bool SCCIManager::initializeChips() {
#ifdef _WIN32
  if (!hasLoadedLib_) {
    return false;
  }

  auto getSoundInterfaceManager=reinterpret_cast<SCCIFUNC>(::GetProcAddress(hScci_,"getSoundInterfaceManager"));
  if (!getSoundInterfaceManager) {
    return false;
  }

  siMan_=getSoundInterfaceManager();
  if (!siMan_) {
    return false;
  }

  if (!siMan_->initializeInstance()) {
    deinitializeChips();
    return false;
  }

  int iCnt=siMan_->getInterfaceCount();
  for (int i=0; i<iCnt; i++) {
    SoundInterface* sif=siMan_->getInterface(i);
    int siCnt=sif->getSoundChipCount();
    for (int j=0; j<siCnt; j++) {
      SoundChip* sc=sif->getSoundChip(j);
      SC_CHIP_TYPE c=static_cast<SC_CHIP_TYPE>(sc->getSoundChipType());
      if (unusedSC_.count(c)) {
        unusedSC_[c].push_back(sc);
      } else {
        unusedSC_[c]={sc};
      }
    }
  }

  reset();

  return true;
#else
  return false;
#endif
}

bool SCCIManager::deinitializeChips() {
#ifdef _WIN32
  unusedSC_.clear();
  connections_.clear();

  if (!siMan_) {
    return false;
  }

  if (!siMan_->releaseAllSoundChip()) {
    return false;
  }

  bool result=siMan_->releaseInstance();
  if (result) {
    siMan_=nullptr;
  }
  return result;
#else
  return false;
#endif
}

bool SCCIManager::reset() {
  return siMan_?siMan_->reset():true;
}

bool SCCIManager::attach(DivSystem sys, DivDispatch* dispatch) {
#ifdef _WIN32
  switch (sys) {
    case DIV_SYSTEM_PC98:
    case DIV_SYSTEM_PC98_EXT:
      if (unusedSC_.count(SC_TYPE_YM2608) && !unusedSC_[SC_TYPE_YM2608].empty() && !connections_.count(dispatch)) {
        SoundChip* sc=unusedSC_[SC_TYPE_YM2608].front();
        unusedSC_[SC_TYPE_YM2608].pop_front();
        connections_[dispatch]=sc;
        return true;
      }
    break;

    default:
      break;
  }
#endif

  return false;
}

bool SCCIManager::hasAttached(DivDispatch* dispatch) const {
#ifdef _WIN32
  return connections_.count(dispatch);
#else
  return false;
#endif
}

bool SCCIManager::detach(DivDispatch* dispatch) {
#ifdef _WIN32
  if (connections_.count(dispatch)) {
    SoundChip* sc=connections_[dispatch];
    connections_.erase(dispatch);
    unusedSC_[SC_TYPE_YM2608].push_back(sc);
    return true;
  }
#endif

  return false;
}

bool SCCIManager::write(DivDispatch* dispatch, addr_t addr, data_t data) {
#ifdef _WIN32
  if (connections_.count(dispatch)) {
    return connections_[dispatch]->setRegister(addr,data);
  }
#endif
  return false;
}

SCCIManager::~SCCIManager() {
  deinitializeChips();
  releaseLibrary();
}
