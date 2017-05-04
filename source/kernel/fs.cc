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

#include "fs.h"

IoReturnState VirtualFileSystem::InodeCtrl::Get(InodeContainer &icontainer, uint32_t inum) {
  Locker locker(_lock);
  Inode *empty == nullptr;
  for (int i = 0; i < kNodesNum; i++) {
    if (_nodes[i]._ref != 0 && _nodes[i]._inum == inum) {
      inode = &_nodes[i];
      inode->_ref++;
      return IoReturnState::kOk;
    } else if (_nodes[i]._ref == 0) {
      empty = &_nodes[i];
    }
  }
  if (empty == nullptr) {
    return IoReturnState::kErrNoSwResource;
  }
  icontainer.Set(empty);
  empty->_inum = inum;
  empty->_flags = 0;
  return IoReturnState::kOk;
}
