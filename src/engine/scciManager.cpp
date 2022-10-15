#include "scciManager.h"
#include <cassert>
#include "dispatch.h"
#include "song.h"

std::unique_ptr<SCCIManager> SCCIManager::instance_;

const std::unordered_map<DivSystem, SC_CHIP_TYPE> SCCIManager::TYPE_MAP_ = {
  { DIV_SYSTEM_YM2151, SC_TYPE_YM2151 },
  { DIV_SYSTEM_OPZ, SC_TYPE_YM2414 },
  { DIV_SYSTEM_OPN, SC_TYPE_YM2203 },
  { DIV_SYSTEM_OPN_EXT, SC_TYPE_YM2203 },
  { DIV_SYSTEM_PC98, SC_TYPE_YM2608 },
  { DIV_SYSTEM_PC98_EXT, SC_TYPE_YM2608 },
  { DIV_SYSTEM_YM2610B, SC_TYPE_YM2610B },
  { DIV_SYSTEM_YM2610B_EXT, SC_TYPE_YM2610B },
  { DIV_SYSTEM_YM2612, SC_TYPE_YM2612 },
  { DIV_SYSTEM_YM2612_EXT, SC_TYPE_YM2612 },
  { DIV_SYSTEM_YM2612_FRAC, SC_TYPE_YM2612 },
  { DIV_SYSTEM_YM2612_FRAC_EXT, SC_TYPE_YM2612 },
  { DIV_SYSTEM_OPL, SC_TYPE_YM3526 },
  { DIV_SYSTEM_Y8950, SC_TYPE_Y8950 },
  { DIV_SYSTEM_OPL2, SC_TYPE_YM3812 },
  { DIV_SYSTEM_OPLL, SC_TYPE_YM2413 },
  { DIV_SYSTEM_OPL3, SC_TYPE_YMF262 },
  { DIV_SYSTEM_AY8910, SC_TYPE_AY8910 },
  { DIV_SYSTEM_AY8930, SC_TYPE_AY8930 },
};

SCCIManager& SCCIManager::instance() {
  if (!instance_) {
    instance_.reset(new SCCIManager);
  }
  assert(instance_);
  return *instance_;
}

bool SCCIManager::loadLibrary() {
#ifdef HAVE_SCCI
  if (hScci_ && !::FreeLibrary(hScci_)) {
    return false;
  }

  hScci_=nullptr;
#endif

  hasLoadedLib_=false;

#ifdef HAVE_SCCI
  hScci_=::LoadLibrary("scci.dll");
  hasLoadedLib_=hScci_!=nullptr;
#endif

  return hasLoadedLib_;
}

bool SCCIManager::releaseLibrary() {
#ifdef HAVE_SCCI
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
#ifdef HAVE_SCCI
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
      unusedSC_.push_back(sif->getSoundChip(j));
    }
  }

  reset();

  return true;
#else
  return false;
#endif
}

bool SCCIManager::deinitializeChips() {
#ifdef HAVE_SCCI
  reset();

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
#ifdef HAVE_SCCI
  return siMan_?siMan_->reset():true;
#else
  return false;
#endif
}

bool SCCIManager::attach(DivSystem sys, DivDispatch* dispatch) {
#ifdef HAVE_SCCI
  if (!TYPE_MAP_.count(sys)) {
    return false;
  }

  SC_CHIP_TYPE type=SCCIManager::TYPE_MAP_.at(sys);

  // Find chip
  auto itr=std::find_if(unusedSC_.begin(), unusedSC_.end(), [type](SoundChip* sc) {
    return static_cast<SC_CHIP_TYPE>(sc->getSoundChipType()) == type; });
  if (itr != unusedSC_.end()) {
    (*itr)->init();
    connections_[dispatch]=*itr;
    unusedSC_.erase(itr);
    return true;
  }

  // Find compatible chip
  for (size_t i=0; i<unusedSC_.size(); i++) {
    SoundChip* sc=unusedSC_[i];
    SCCI_SOUND_CHIP_INFO* info=sc->getSoundChipInfo();
    for (size_t j=0; j<2; j++) {
      if (static_cast<SC_CHIP_TYPE>(info->iCompatibleSoundChip[j]) == type) {
        sc->init();
        connections_[dispatch]=sc;
        unusedSC_.erase(unusedSC_.begin() + i);
        return true;
      }
    }
  }
#endif

  return false;
}

bool SCCIManager::hasAttached(DivDispatch* dispatch) const {
#ifdef HAVE_SCCI
  return connections_.count(dispatch);
#else
  return false;
#endif
}

bool SCCIManager::detach(DivDispatch* dispatch) {
#ifdef HAVE_SCCI
  if (connections_.count(dispatch)) {
    SoundChip* sc=connections_[dispatch];
    sc->init();
    connections_.erase(dispatch);
    unusedSC_.push_back(sc);
    return true;
  }
#endif

  return false;
}

bool SCCIManager::write(DivDispatch* dispatch, addr_t addr, data_t data) {
#ifdef HAVE_SCCI
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
