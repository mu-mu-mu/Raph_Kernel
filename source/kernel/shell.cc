/*
 *
 * Copyright (c) 2016 Raphine Project
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
 * Author: Yuchiki
 * 
 */


/*this shell-like program will be substituted later*/
#include <global.h>
#include <tty.h>
#include <string.h>
#include <shell.h>
#include <cpu.h>

void Shell::Setup() {
  _com_buf.SetFunction(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), make_uptr(new ClassFunction<Shell, void *>(this, &Shell::HandleComBuf, nullptr)));
  _main_thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kIndependent);
  _main_thread->CreateOperator().SetFunc(make_uptr(new ClassFunction<Shell, void *>(this, &Shell::HandleChar, nullptr)));
  _liner.Setup(this);
}

void Shell::Register(const char *name, void (*func)(int argc, const char *argv[])) {
  if (_next_buf == kBufSize){
    kernel_panic("Shell", "command buffer is full");
  }else{
    int slen = strlen(name);
    if (slen < kNameSize){
      strncpy(_name_func_mapping[_next_buf].name, name, slen);
      _name_func_mapping[_next_buf].func = func;
      _next_buf++;
    } else {
      kernel_panic("Shell", "command name is too long");
    }
  }
}

void Shell::Exec(const char *name, int argc, const char **argv) {
  for (int i = 0; i < _next_buf; i++) {
    if (strncmp(name, _name_func_mapping[i].name, strlen(_name_func_mapping[i].name)) == 0) {
      _name_func_mapping[i].func(argc, argv);
      return;
    }
  }

  gtty->Printf("unknown command: <%s>\n", name);
}

void Shell::HandleComBuf(void *) {
  char c;
  if (_com_buf.Pop(c)) {
    _com_buf.Block();
    _c = c;
    _main_thread->CreateOperator().Schedule();
  }
}

void Shell::Execute(uptr<ExecContainer> ec) {
  if (ec->argc > 0) {
    ec->shell->Exec(ec->name, ec->argc, ec->argv);
  }
}

void Shell::Liner::ReadCh(char c) {
  if (c == '\n') {
    auto ec = make_uptr(new ExecContainer(_shell));
    ec = Tokenize(ec, _command);
    gtty->Printf("> %s\n", _command);
    _shell->Execute(ec);
    Reset();
  } else if (c == '\b') {
    // backspace
    if (_next_command > 0) {
      _next_command--;
      _command[_next_command] = '\0';
      gtty->PrintShell(_command);
    }
  } else if (_next_command != kCommandSize - 1) {
    _command[_next_command] = c;
    _next_command++;
    _command[_next_command] = '\0';
    gtty->PrintShell(_command);
  }
}

uptr<Shell::ExecContainer> Shell::Liner::Tokenize(uptr<Shell::ExecContainer> ec, char *command) {
  strcpy(ec->name, command);
  bool inToken = false;
  for (int i = 0; i < kCommandSize -1; i++) {
    if (ec->name[i] == '\0') return ec;
    if (ec->name[i] == '#') {
      ec->name[i] = '\0';
      return ec;
    }
    if (inToken) {
      if (ec->name[i] == ' ') {
        ec->name[i] = '\0';
        inToken = false;
      }
    } else {
      if (ec->name[i] == ' ') {
        ec->name[i] = '\0';
      } else {
        if (ec->argc < kArgumentMax) {
          ec->argv[ec->argc] = ec->name + i;
          ec->argc++;
          ec->argv[ec->argc] = nullptr;
        }
        inToken = true;
      }
    }
  }
  return ec;
}

void Shell::Liner::Reset() {
    _command[0] = '\0';
    _next_command = 0;
    gtty->PrintShell("");
}
