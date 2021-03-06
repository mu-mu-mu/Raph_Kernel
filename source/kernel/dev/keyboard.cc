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

#include "keyboard.h"
#include <ptr.h>
#include <function.h>
#include <shell.h>
#include <global.h>

void Keyboard::Setup() {
  _buf.SetFunction(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), make_uptr(new ClassFunction<Keyboard, void *>(this, &Keyboard::Handle, nullptr)));
  SetupSub();
}

void Keyboard::Handle(void *) {
  char data;
  if(!_buf.Pop(data)){
    return;
  }
  shell->PushCh(data);
}
