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

#include <stdint.h>
#include <string.h>
#include <dev/storage/storage.h>
#include <assert.h>
#include <spinlock.h>

struct Stat {
  enum class Type : int16_t {
    kDirectory = 1,
    kFile = 2,
    kDevice = 3,
  };
  Type type;
  
  int32_t dev;
  uint32_t ino;
  int16_t nlink;
  uint32_t size;
};

class DiskFileSystem {
public:
  virtual ~DiskFileSystem() {
  }
  virtual IoReturnState AllocInode(uint32_t &inum, VirtualFileSystem::FileType type) __attribute__((warn_unused_result)) = 0;
  virtual void GetStatOfInode(uint32_t inum, Stat &stat) = 0;
  virtual uint32_t GetRootInodeNum() = 0;
private:
};

class Inode {
public:
  void Init(DiskFileSystem *fs) {
    _fs = fs;
  }
  void GetStatOfInode(Stat &st) {
    st.dev = 0; // dummy
    fs->GetStatOfInode(_inum, st);
  }
  void Get() {
    Locker locker(_lock);
    inode->ref++;
  }
  IoReturnState Put() __attribute__((warn_unused_result)) {
    Locker locker(_lock);
    inode->ref--;
    return IoReturnState::kOk;
  }
private:
  friend InodeCtrl;
  SpinLock _lock;
  uint32_t _inum;
  int _ref = 0;
  int _flags;
  DiskFileSystem *_fs = nullptr;
};

class InodeContainer {
public:
  InodeContainer() = default;
  ~InodeContainer() {
    if (_node != nullptr) {
      _node->Put();
    }
  }
  void Set(Inode *inode) {
    if (_node != nullptr) {
      _node->Put();
    }
    _node = inode;
    if (_node != nullptr) {
      _node->Get();
    }
  }
  void GetStatOfInode(Stat &st) {
    assert(_node != nullptr);
    _node->GetStatOfInode(st);
  }
private:
  Inode *_node = nullptr;
};

class VirtualFileSystem {
public:
  VirtualFileSystem() = delete;
  VirtualFileSystem(DiskFileSystem *dfs) : _dfs(dfs) {
  }
  enum class FileType {
    kDirectory,
    kFile,
    kDevice,
  };
  IoReturnState LookupInodeFromPath(Inode* &inode, char *path, int parent, char *name) {
    //TODO
  }
  IoReturnState DirLookup(InodeContainer &dinode, char *name, int &offset) {
    Stat dinode_stat;
    dinode.GetStatOfInode(dinode_stat);
    if (dinode_stat.type != Stat::Type::kDirectory) {
      return IoReturnState::kErrInvalid;
    }

    for (uint32_t i = 0; i < dinode_stat.size; i += sizeof()) {
    }
  }
  static const size_t kMaxDirectoryNameLength;
private:
  class InodeCtrl {
  public:
    void Init() {
      for (int i = 0; i < kNodesNum; i++) {
        _nodes[i].Init();
      }
    }
    IoReturnState Alloc(FileType type) __attribute__((warn_unused_result)) {
      uint32_t inum;
      IoReturnState state1 = _dfs->AllocInode(inum, type);
      if (state1 != IoReturnState::kOk) {
        return state1;
      }
      Inode *inode;
      IoReturnState state2 = Get(inode, inum);
      return state2;
    }
    IoReturnState Get(InodeContainer &icontainer, uint32_t inum) __attribute__((warn_unused_result));
  private:
    static const int kNodesNum = 50;
    SpinLock _lock;
    Inode _nodes[kNodesNum];
  } _inode_ctrl;
  // if no next path name, return nullptr.
  // the buffer size of 'name' must be 'length + 1'.
  static char *GetNextPathNameFromPath(char *path, char *name) {
    while(*path == '/') {
      path++;
    }
    if (*path == '\0') {
      return nullptr;
    }
    char *start = path;
    while(*path != '/' && *path != '\0') {
      path++;
    }
    size_t len = path - start;
    if (len > kMaxDirectoryNameLength) {
      len = kMaxDirectoryNameLength;
    }
    memmove(name, s, len);
    name[len] = '\0';
    while(*path == '/') {
      path++;
    }
    return path;
  }
  DiskFileSystem *_dfs;
};
