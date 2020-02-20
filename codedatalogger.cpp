/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifdef DEBUGGER

#include "codedatalogger.h"

#include "snes9x.h"
#include "port.h"
#include "memmap.h"

#include <vector>
#include <map>
#include <cstdio>
#include <cstring>

namespace {

bool readInt(FILE* fp, uint32_t* value) {
  char bytes[4];
  if (fread(bytes, 4, 1, fp) != 1) return false;
  *value = READ_DWORD(bytes);
  return true;
}

bool readLeb128(FILE* fp, unsigned int* value) {
  unsigned int v = 0;
  int shift = 0;
  while (true) {
    const int c = fgetc(fp);
    if (c == EOF) return false;
    v |= static_cast<uint8_t>(c & 0x7f) << shift;
    if ((c & 0x80) == 0) break;
    shift += 7;
  }
  *value = v;
  return true;
}

bool expectString(FILE* fp, const char* value) {
  size_t size = 0;
  if (!readLeb128(fp, &size)) return false;
  if (strlen(value) != size) return false;

  std::string s = std::string(size, 0);
  if (fread(&s[0], sizeof(char), size, fp) != size) return false;

  return memcmp(s.c_str(), value, size) == 0;
}

bool readString(FILE* fp, char* buffer, size_t capacity) {
  size_t size = 0;
  if (!readLeb128(fp, &size)) return false;
  if (capacity <= size) return false;

  if (fread(buffer, sizeof(char), size, fp) != size) return false;
  buffer[size] = '\0';
  return true;
}

bool writeInt(FILE* fp, uint32_t value) {
  char bytes[4];
  WRITE_DWORD(bytes, value);
  return fwrite(bytes, 4, 1, fp) == 1;
}

bool writeLeb128(FILE* fp, unsigned int value) {
  do {
    uint8_t c = value & 0x7f;
    value >>= 7;
    if (value != 0) c |= 0x80;
    if (fputc(c, fp) == EOF) return false;
  } while (value != 0);
  return true;
}

bool writeString(FILE* fp, const char* value) {
  const size_t size = strlen(value);
  if (!writeLeb128(fp, size)) return false;
  return fwrite(value, sizeof(char), size, fp) == size;
}

}

const char* CCodeDataLog::kBlockNames[] = {
  "CARTROM", "CARTRAM", "WRAM", "APURAM", "SGB_CARTROM", "SGB_CARTRAM", "SGB_WRAM", "SGB_HRAM"
};

void CCodeDataLog::New(const CMemory* memory) {
  blocks[eCDLog_AddrType_CARTROM].resize(memory->CalculatedSize);
  blocks[eCDLog_AddrType_CARTRAM].resize(memory->SRAMSize ? (1 << (memory->SRAMSize + 3)) * 128 : 0);
  blocks[eCDLog_AddrType_WRAM].resize(0x20000);
  blocks[eCDLog_AddrType_APURAM].resize(0x10000);

  for (int i = 0; i < eCDLog_AddrType_NUM; i++) {
    if (!blocks[i].empty()) std::fill(blocks[i].begin(), blocks[i].end(), eCDLog_Flags_None);
  }
}

void CCodeDataLog::Close() {
  for (int i = 0; i < eCDLog_AddrType_NUM; i++) {
    blocks[i].clear();
  }
}

bool CCodeDataLog::Load(const char *path) {
  CCodeDataLog other;
  if (!other.LoadAsIs(path)) return false;
  if (CountActiveBlocks() != other.CountActiveBlocks()) return false;
  for (uint32_t i = 0; i < eCDLog_AddrType_NUM; i++) {
    if (blocks[i].size() != other.blocks[i].size()) return false;
  }

  for (uint32_t i = 0; i < eCDLog_AddrType_NUM; i++) {
    if (blocks[i].empty()) continue;
    for (size_t addr = 0; addr < blocks[i].size(); addr++) {
      blocks[i][addr] |= other.blocks[i][addr];
    }
  }
  return true;
}

bool CCodeDataLog::LoadAsIs(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) return false;

  std::map<std::string, eCDLog_AddrType> addrTypeDict;
  for (uint32_t i = 0; i < eCDLog_AddrType_NUM; i++) {
    addrTypeDict[kBlockNames[i]] = eCDLog_AddrType(i);
  }

  uint32_t count = 0;
  if (!expectString(fp, "BIZHAWK-CDL-2")) goto fail;
  if (!expectString(fp, "SNES           ")) goto fail;
  if (!readInt(fp, &count)) goto fail;

  Close();
  for (int i = 0; i < static_cast<int>(count); i++) {
    char name[32];
    uint32_t size = 0;
    std::vector<uint8_t> block;

    if (!readString(fp, name, sizeof(name))) goto fail;
    if (!readInt(fp, &size)) goto fail;

    std::map<std::string, eCDLog_AddrType>::const_iterator it = addrTypeDict.find(name);
    if (it == addrTypeDict.end()) continue;
    const eCDLog_AddrType type = it->second;

    blocks[type].resize(size);
    if (fread(&blocks[type][0], 1, size, fp) != size) goto fail;
  }

  fclose(fp);
  return true;

fail:
  fclose(fp);
  Close();
  return false;
}

bool CCodeDataLog::Save(const char *path) {
  FILE *fp = fopen(path, "wb");
  if (!fp) return false;

  const int count = CountActiveBlocks();
  if (!writeString(fp, "BIZHAWK-CDL-2")) goto fail;
  if (!writeString(fp, "SNES           ")) goto fail;
  if (!writeInt(fp, count)) goto fail;
  fflush(fp);

  for (int i = 0; i < eCDLog_AddrType_NUM; i++) {
    if (blocks[i].empty()) continue;
    if (!writeString(fp, kBlockNames[i])) goto fail;
    if (!writeInt(fp, blocks[i].size())) goto fail;
    if (fwrite(&blocks[i][0], 1, blocks[i].size(), fp) != blocks[i].size()) goto fail;
  }

  fclose(fp);
  return true;

fail:
  fclose(fp);
  return false;
}

#endif
