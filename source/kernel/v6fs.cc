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
 * 
 */

#include "v6fs.h"

V6FileSystem::V6FileSystem(Storage &storage) {
  _storage.Read(_sb, kBlockSize);
}

IoReturnState VirtualFileSystem::V6FileSystem::InodeCtrl::Alloc(uint32_t &inum, uint16_t type) {
  for (uint32_t i = 1; i < _sb.ninodes; i++) {
    Inode inode;
    _storage.Read(inode, _sb.inodestart + sizeof(Inode) * i);
    if (inode.type == 0) {
      // a free inode
      inode.Init(type);
      _storage.Write(inode, _sb.inodestart + sizeof(Inode) * i);
      inum = i;
      return IoReturnState::kOk;
    }
  }
  return IoReturnState::kErrNoHwResource;
}

void VirtualFileSystem::V6FileSystem::InodeCtrl::GetStatOfInode(uint32_t inum, Stat &stat) {
  assert(inum < _sb.ninodes);
  Inode inode;
  _storage.Read(inode, _sb.inodestart + sizeof(Inode) * inum);
  stat.type = inode.type;
  stat.ino = inum;
  stat.nlink = inode.nlink;
  stat.size = inode.size;
  return;
}


