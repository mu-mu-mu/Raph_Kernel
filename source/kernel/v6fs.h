/*
 *
 * Copyright (c) 2017 Raphine Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Liva
 * UNIX v6 file system driver (inspired from xv6 source code)
 * 
 */

#pragma once

#include <fs.h>

class V6FileSystem : public DiskFileSystem{
public:
  V6FileSystem() = delete;
  V6FileSystem(Storage &storage) : _storage(storage), _inode_ctrl(_sb) {
  }
  virtual ~V6FileSystem() {
  }
private:
  static const int kBlockSize = 512;
  static const int kNumOfDirectBlocks = 12;
  static const int kNumEntriesOfIndirectBlock = kBlockSize / sizeof(uint32_t);
  struct SuperBlock {
    uint32_t size;
    uint32_t nblocks;
    uint32_t ninodes;
    uint32_t nlog;
    uint32_t logstart;
    uint32_t inodestart;
    uint32_t bmapstart;
  } __attribute__((__packed__)) _sb;
  enum class Type : uint16_t {
    kDirectory = 1,
    kFile = 2,
    kDevice = 3,
  };
  struct Inode {
    int16_t type;
    int16_t major;
    int16_t minor;
    int16_t nlink;
    uint32_t size;
    uint32_t addrs[kNumOfDirectBlocks];
    uint32_t indirect;
    void Init(uint16_t type) {
      type = type;
      major = 0;
      minor = 0;
      nlink = 0;
      size = 0;
      bzero(addrs, sizeof(addrs));
      indirect = 0;
    }
  } __attribute__((__packed__));
  static Type ConvType(VirtualFileSystem::FileType type) {
    switch(type) {
    case VirtualFileSystem::FileType::kDirectory:
      return Type::kDirectory;
    case VirtualFileSystem::FileType::kFile:
      return Type::kFile;
    case VirtualFileSystem::FileType::kDevice:
      return Type::kDevice;
    };
  }
  class InodeCtrl {
  public:
    InodeCtrl() = delete;
    InodeCtrl(SuperBlock &sb) : _sb(sb) {
    }
    IoReturnState Alloc(uint32_t &inum, uint16_t type) __attribute__((warn_unused_result));
    void GetStatOfInode(uint32_t inum, Stat &stat);
  private:
    SuperBlock &_sb;
  } _inode_ctrl;
  virtual IoReturnState AllocInode(uint32_t &inum, VirtualFileSystem::FileType type) __attribute__((warn_unused_result)) override {
    return _inode_ctrl.Alloc(inum, ConvType(type));
  } _inode_ctrl;
  virtual void GetStatOfInode(uint32_t inum, Stat &stat) override {
    _inode_ctrl.GetStatOfInode(inum, stat);
  }
  virtual uint32_t GetRootInodeNum() override {
    return 0;
  }
  Storage &_storage;
};
