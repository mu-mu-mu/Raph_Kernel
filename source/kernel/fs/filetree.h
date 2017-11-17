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

#include <fs/inode.h>
#include <fs/fs.h>
#include <stdlib.h>
#include <io.h>

class FileTree {
public:
  virtual void Init() = 0;
  virtual void ReleaseInode() = 0;
  virtual IoReturnState LookupInodeFromPath(InodeContainer &inode, const char *path, bool parent) = 0;
  
};
  
class DiskFileTree : public FileTree {
public:
  DiskFileTree() = delete;
  DiskFileTree(DiskFileSystem *dfs) : _dfs(dfs), _inode_ctrl(dfs) {
  }
  virtual void Init() {
    _inode_ctrl.Init(this);
  }
  virtual void ReleaseInode() {
    _inode_ctrl.ReleaseInode();
  }
  virtual IoReturnState LookupInodeFromPath(InodeContainer &inode, const char *path, bool parent);
  static const size_t kMaxDirectoryNameLength = 14;
private:
  
  class InodeCtrl {
  public:
    InodeCtrl() = delete;
    InodeCtrl(DiskFileSystem *fs) : _dfs(fs) {
    }
    void Init(DiskFileTree *vfs) {
      for (int i = 0; i < kNodesNum; i++) {
        _nodes[i].Init(_dfs, vfs);
      }
    }
    void ReleaseInode() {
    }
    IoReturnState Alloc(InodeContainer::FileType type, InodeContainer &inode) __attribute__((warn_unused_result));
    IoReturnState Get(InodeContainer &icontainer, uint32_t inum) __attribute__((warn_unused_result));
  private:
    static const int kNodesNum = 50;
    SpinLock _lock;
    Inode _nodes[kNodesNum];
    DiskFileSystem *_dfs;
  };
  
  static const char *GetNextPathNameFromPath(const char *path, char *name);
  IoReturnState ReadDataFromInode(uint8_t *data, InodeContainer &inode, size_t offset, size_t &size) __attribute__((warn_unused_result));
  
  DiskFileSystem *_dfs;
  InodeCtrl _inode_ctrl;
};
