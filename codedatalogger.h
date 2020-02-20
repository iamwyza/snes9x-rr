/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _CODEDATALOGGER_H_
#define _CODEDATALOGGER_H_

#include <vector>

struct CMemory;

// The following enum and the file formats are compatible with BizHawk CDL.
// <http://tasvideos.org/Bizhawk/CodeDataLogger.html>

enum eCDLog_AddrType {
  eCDLog_AddrType_CARTROM, eCDLog_AddrType_CARTRAM, eCDLog_AddrType_WRAM, eCDLog_AddrType_APURAM,
  eCDLog_AddrType_SGB_CARTROM, eCDLog_AddrType_SGB_CARTRAM, eCDLog_AddrType_SGB_WRAM, eCDLog_AddrType_SGB_HRAM,
  eCDLog_AddrType_NUM
};

enum eCDLog_Flags {
  eCDLog_Flags_None = 0x00,
  eCDLog_Flags_ExecFirst = 0x01,
  eCDLog_Flags_ExecOperand = 0x02,
  eCDLog_Flags_CPUData = 0x04,
  eCDLog_Flags_DMAData = 0x08,
  eCDLog_Flags_CPUXFlag = 0x10, //these values are picky, don't change them
  eCDLog_Flags_CPUMFlag = 0x20, //these values are picky, don't change them
  eCDLog_Flags_BRR = 0x80
};

struct CCodeDataLogStatistics {
  CCodeDataLogStatistics() : TotalBytes(0), TotalBytesOfFlags() {}
  size_t TotalBytes;
  size_t TotalBytesOfFlags[8];
};

#ifdef DEBUGGER

struct CCodeDataLog {
  CCodeDataLog() : active(true) {}

  static const char* kBlockNames[eCDLog_AddrType_NUM];

  std::vector<uint8_t> blocks[eCDLog_AddrType_NUM];
  bool active;

  int CountActiveBlocks() const {
    int count = 0;
    for (int i = 0; i < eCDLog_AddrType_NUM; i++) {
      if (!blocks[i].empty()) count++;
    }
    return count;
  }

  void Set(eCDLog_AddrType addrType, eCDLog_Flags flags, uint32_t addr) {
    if (!active) return;
    if (addr >= blocks[addrType].size()) return;
    blocks[addrType][addr] |= flags;
  }

  void New(const CMemory* memory);
  void Close();
  bool Load(const char *path);
  bool LoadAsIs(const char *path);
  bool Save(const char *path);
};

extern CCodeDataLog CDL;

#endif

#endif
