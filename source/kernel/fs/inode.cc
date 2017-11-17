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

#include <fs/inode.h>
#include <fs/filetree.h>
#include <fs/fs.h>

/**
 * @brief initialize and set inode
 * @param inode target inode
 * @param inum inode number
 *
 * This function must be called from DiskFileTree::InodeContainer
 */
void InodeContainer::InitInode(Inode *inode, InodeNumber inum) {
  if (_node != nullptr) {
    _node->DecRef();
  }
  _node = inode;
  if (_node != nullptr) {
    Locker locker(_node->_lock);
    if (_node->_ref != 0) {
      kernel_panic("Inode", "unknown error");
    }
    _node->_ref = 1;
    _node->_inum = inum;
    _node->_flags = 0;
  }
}

bool InodeContainer::SetIfSame(Inode *inode, InodeNumber inum) {
  if (inode == nullptr) {
    return false;
  }
  Locker locker(inode->_lock);
  if (inode->_ref != 0 && inode->_inum == inum) { 
    _node = inode;
    _node->_ref++;
    return true;
  } else {
    return false;
  }
}

IoReturnState InodeContainer::GetStatOfInode(InodeContainer::Stat &st) {
  assert(_node != nullptr);
  Locker locker(_node->_lock);
  st.dev = 0; // dummy
  return _node->_fs->GetStatOfInode(_node->_inum, st);
}

IoReturnState InodeContainer::ReadData(uint8_t *data, size_t offset, size_t &size) {
  assert(_node != nullptr);
  Locker locker(_node->_lock);
  return _node->_fs->ReadDataFromInode(data, _node->_inum, offset, size);
}
IoReturnState InodeContainer::DirLookup(char *name, int &offset, InodeNumber &inum) {
  assert(_node != nullptr);
  Locker locker(_node->_lock);
  return _node->_fs->DirLookup(_node->_inum, name, offset, inum);
}
