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
#include <io.h>

class FileTree {
public:
  virtual void Init() = 0;
  virtual void ReleaseInode() = 0;
  virtual IoReturnState LookupInodeFromPath(InodeContainer &inode, const char *path, bool parent) = 0;
  virtual IoReturnState GetStatOfInode(InodeNumber inum, InodeContainer::Stat &stat) __attribute__((warn_unused_result)) = 0;
  /**
   * @brief read data from inode
   * @param buf buffer for storing data
   * @param inum target inode number
   * @param offset offset in target inode file
   * @param size size of the data to be read. This function overwrites the value with the actually size read.
   * 
   * Caller must allocate 'data'. The size of 'data' must be larger than 'size'.
   * TODO: check if we can remove this function
   */
  virtual IoReturnState ReadDataFromInode(uint8_t *data, InodeNumber inum, size_t offset, size_t &size) __attribute__((warn_unused_result)) = 0;
  /**
   * @brief lookup directory and return inode
   * @param dinode inode of directory
   * @param name entry name to lookup
   * @param offset offset of entry (returned by this function)
   * @param inode found inode (returned by this function)
   */
  virtual IoReturnState DirLookup(InodeNumber dinode, char *name, int &offset, InodeNumber &inode) __attribute__((warn_unused_result)) = 0;
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
  //TODO: wrapper?
  IoReturnState DirLookup(InodeNumber dinode, char *name, int &offset, InodeNumber &inode) {
    return _dfs->DirLookup(dinode,name,offset,inode);
  }
  IoReturnState ReadDataFromInode(uint8_t *data, InodeNumber inum, size_t offset, size_t &size) {
    return _dfs->ReadDataFromInode(data,inum,offset,size);
  }
  IoReturnState GetStatOfInode(InodeNumber inum, InodeContainer::Stat &stat) {
    return _dfs->GetStatOfInode(inum,stat);
  }
  static const size_t kMaxDirectoryNameLength = 14;
private:
  
  class InodeCtrl {
  public:
    InodeCtrl() = delete;
    InodeCtrl(DiskFileSystem *fs) : _dfs(fs) {
    }
    void Init(DiskFileTree *dft) {
      for (int i = 0; i < kNodesNum; i++) {
        _nodes[i].Init(dft);
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
