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
 * Author: Liva,mumumu
 * UNIX v6 file system driver (inspired from xv6 source code)
 * 
 */

#pragma once

#include <fs/filetree.h>
#include <fs/inode.h>
#include <io.h>

class DiskFileSystem {
public:
  virtual ~DiskFileSystem() {
  }
  virtual IoReturnState AllocInode(InodeNumber &inum, InodeContainer::FileType type) __attribute__((warn_unused_result)) = 0;
  virtual IoReturnState GetStatOfInode(InodeNumber inum, InodeContainer::Stat &stat) __attribute__((warn_unused_result)) = 0;
  virtual InodeNumber GetRootInodeNum() = 0;
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
private:
};
