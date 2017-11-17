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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva,mumumu
 *
 */

#pragma once

#include <assert.h>
#include <spinlock.h>
#include <io.h>

//TODO replace spinlock to job qeueue

using InodeNumber = uint32_t;

class InodeContainer;
class DiskFileTree;
class DiskFileSystem;

class Inode {
public:
  void Init(DiskFileSystem *fs, DiskFileTree *vfs) {
    _fs = fs;
    _vfs = vfs;
  }
  bool IsEmpty() {
    return _ref == 0;
  }
private:
  friend InodeContainer;
  /**
   * @brief decrement reference
   *
   * This function must be called from InodeContainer
   */
  void DecRef() {
    Locker locker(_lock);
    _ref--;
  }
  SpinLock _lock;
  int _ref = 0;
  int _flags;
  DiskFileSystem *_fs = nullptr;
  InodeNumber _inum;
  DiskFileTree *_vfs;
};

/**
 * @brief wrapper class of Inode
 * 
 */
class InodeContainer {
public:
  //TODO:Q Where are these defined at ?
  enum class FileType {
    kDirectory,
    kFile,
    kDevice,
  };

  struct Stat {
    FileType type;
    int32_t dev;
    InodeNumber ino;
    int16_t nlink;
    uint32_t size;
  };

  InodeContainer() = default;
  InodeContainer(const InodeContainer &obj) {
    _node = obj._node;
    Locker locker(_node->_lock);
    _node->_ref++;
  }
  ~InodeContainer() {
    if (_node != nullptr) {
      _node->DecRef();
    }
  }

  void InitInode(Inode *inode, InodeNumber inum);

  bool SetIfSame(Inode *inode, InodeNumber inum);

  IoReturnState GetStatOfInode(Stat &st) __attribute__((warn_unused_result));

  IoReturnState ReadData(uint8_t *data, size_t offset, size_t &size) __attribute__((warn_unused_result));

  IoReturnState DirLookup(char *name, int &offset, InodeNumber &inum) __attribute__((warn_unused_result));

private:
  Inode *_node = nullptr;
};
