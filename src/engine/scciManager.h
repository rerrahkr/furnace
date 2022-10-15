#ifndef _SCCI_MANAGER_H
#define _SCCI_MANAGER_H

#include <memory>
#include <unordered_map>
#include <deque>
#include <vector>

#ifdef HAVE_SCCI
#include <Windows.h>
#include "../../extern/SCCI/scci.h"
#include "../../extern/SCCI/SCCIDefines.h"
#endif

enum DivSystem;
class DivDispatch;

class SCCIManager {
  static std::unique_ptr<SCCIManager> instance_;
  bool hasLoadedLib_;

  SCCIManager() noexcept:
    hasLoadedLib_(false) {}

#ifdef HAVE_SCCI
  HMODULE hScci_=nullptr;
  SoundInterfaceManager* siMan_=nullptr;
  std::vector<SoundChip*> unusedSC_;
  std::unordered_map<DivDispatch*, SoundChip*> connections_;
  static const std::unordered_map<DivSystem, SC_CHIP_TYPE> TYPE_MAP_;
#endif

public:
  /**
   * get manager instance.
   * @return a reference to instance of SCCI manager.
  */
  static SCCIManager& instance();

  /**
   * load scci.dll.
   * @return whether loading was success.
  */
  bool loadLibrary();

  /**
   * relase scci.dll.
   * @return whether releasing was success.
  */
  bool releaseLibrary();

  /**
   * initialize SCCI maneger.
   * @return whether an initialization was success.
  */
  bool initializeChips();

  /**
   * deinitialize SCCI manager.
   * @return whether a deinitialization was success.
  */
  bool deinitializeChips();

  /**
   * reset all chips.
   * @return whether reset was success.
  */
  bool reset();

  /**
   * check whether an engine is connected to real chips.
   * @param dispatch a connection source.
   * @return whether an engine is connected to real chips.
  */
  bool hasAttached(DivDispatch* dispatch) const;

  /**
   * try to connect an engine to a real chip.
   * @param sys an enum of system.
   * @param dispath a dispatch whitch is connection source.
   * @return whether a connection has been established.
  */
  bool attach(DivSystem sys, DivDispatch* dispatch);

  /**
   * try to disconnect an engine from a real chip.
   * @param dispatch a dispatch which is connection source.
   * @return whether disconnecting was success.
  */
  bool detach(DivDispatch* dispatch);

  using addr_t=unsigned long;
  using data_t=long long;

  /**
   * send data to a real chip.
   * @param dispatch a dispacth which is connection source.
   * @param addr an address.
   * @param data a data
   * @return whether writing data was success.
  */
  bool write(DivDispatch* dispatch, addr_t addr, data_t data);

  ~SCCIManager();
  SCCIManager(const SCCIManager&)=delete;
  SCCIManager& operator=(const SCCIManager&)=delete;
  SCCIManager(SCCIManager&&)=delete;
  SCCIManager& operator=(SCCIManager&&)=delete;
};

#endif
