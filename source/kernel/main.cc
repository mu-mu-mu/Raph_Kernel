/*
 *
 * Copyright (c) 2015 Project Raphine
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

#include "global.h"
#include "spinlock.h"
#include "acpi.h"
#include "apic.h"
#include "multiboot.h"
#include "polling.h"
#include "mem/physmem.h"
#include "mem/paging.h"
#include "idt.h"
#include "timer.h"

#include "tty.h"
#include "dev/hpet.h"

#include "dev/vga.h"
#include "dev/pci.h"

#include "net/netctrl.h"
#include "net/socket.h"

SpinLockCtrl *spinlock_ctrl;
MultibootCtrl *multiboot_ctrl;
AcpiCtrl *acpi_ctrl;
ApicCtrl *apic_ctrl;
PhysmemCtrl *physmem_ctrl;
PagingCtrl *paging_ctrl;
VirtmemCtrl *virtmem_ctrl;
PollingCtrl *polling_ctrl;
Idt *idt;
Timer *timer;

Tty *gtty;

PCICtrl *pci_ctrl;

static uint32_t rnd_next = 1;

#include <dev/e1000/bem.h>
bE1000 *eth;
uint64_t cnt;

extern "C" int main() {
  SpinLockCtrl _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  
  MultibootCtrl _multiboot_ctrl;
  multiboot_ctrl = &_multiboot_ctrl;

  AcpiCtrl _acpi_ctrl;
  acpi_ctrl = &_acpi_ctrl;

  ApicCtrl _apic_ctrl;
  apic_ctrl = &_apic_ctrl;
  
  Idt _idt;
  idt = &_idt;

  VirtmemCtrl _virtmem_ctrl;
  virtmem_ctrl = &_virtmem_ctrl;
  
  PhysmemCtrl _physmem_ctrl;
  physmem_ctrl = &_physmem_ctrl;
  
  PagingCtrl _paging_ctrl;
  paging_ctrl = &_paging_ctrl;

  PollingCtrl _polling_ctrl;
  polling_ctrl = &_polling_ctrl;
  
  Hpet _htimer;
  timer = &_htimer;

  Vga _vga;
  gtty = &_vga;
  
  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, PagingCtrl::kPageSize * 1);
  extern int kKernelEndAddr;
  kassert(paging_ctrl->MapPhysAddrToVirtAddr(reinterpret_cast<virt_addr>(&kKernelEndAddr) - PagingCtrl::kPageSize * 3, paddr, PagingCtrl::kPageSize * 1, PDE_WRITE_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT));

  multiboot_ctrl->Setup();
  
  // acpi_ctl->Setup() は multiboot_ctrl->Setup()から呼ばれる

  if (timer->Setup()) {
    gtty->Printf("s","[timer] info: HPET supported.\n");
  } else {
    kernel_panic("timer", "HPET not supported.\n");
  }

  rnd_next = timer->ReadMainCnt();

  // timer->Sertup()より後
  apic_ctrl->Setup();
  
  cnt = 0;

  idt->Setup();

  InitNetCtrl();

  InitDevices<PCICtrl, Device>();

  extern int kKernelEndAddr;
  // stackは16K
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr)));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 1) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 2) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 3) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 4) + 1));
  kassert(!paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - 4096 * 5));

  gtty->Printf("s", "[cpu] info: #", "d", apic_ctrl->GetApicId(), "s", " started.\n");

  apic_ctrl->StartAPs();

  gtty->Printf("s", "\n\n[kernel] info: initialization completed\n");

  polling_ctrl->HandleAll();
  while(true) {
    asm volatile("hlt;nop;hlt;");
  }

  DismissNetCtrl();

  return 0;
}

extern "C" int main_of_others() {
  // according to mp spec B.3, system should switch over to Symmetric I/O mode
  apic_ctrl->BootAP();
  gtty->Printf("s", "[cpu] info: #", "d", apic_ctrl->GetApicId(), "s", " started.\n");
  uint8_t ip[] = {
    //192, 168, 100, 120,
    10, 0, 2, 5,
  };
  if (apic_ctrl->GetApicId() == 1) {
    ARPSocket socket;
    uint32_t ipaddr;
    uint8_t macaddr[6];
    if(socket.Open() < 0) {
      gtty->Printf("s", "[error] failed to open socket\n");
    }
    socket.SetIPAddr(0x0a000205);
//    socket.SetIPAddr(0xc0a86475);

    while(true) {
      int32_t rval = socket.ReceivePacket(0, &ipaddr, macaddr);
      if(rval == ARPSocket::kOpARPReply) {
        uint64_t l = ((uint64_t)(timer->ReadMainCnt() - cnt) * (uint64_t)timer->GetCntClkPeriod()) / 1000;
        gtty->Printf(
                     "s", "[arp] reply received; ",
                     "x", macaddr[0], "s", ":",
                     "x", macaddr[1], "s", ":",
                     "x", macaddr[2], "s", ":",
                     "x", macaddr[3], "s", ":",
                     "x", macaddr[4], "s", ":",
                     "x", macaddr[5], "s", " is ",
                     "d", (ipaddr >> 24) & 0xff, "s", ".",
                     "d", (ipaddr >> 16) & 0xff, "s", ".",
                     "d", (ipaddr >> 8) & 0xff, "s", ".",
                     "d", (ipaddr >> 0) & 0xff, "s", "\n");
        gtty->Printf("s", "latency:", "d", l, "s", "us\n");
      } else if(rval == ARPSocket::kOpARPRequest) {
        gtty->Printf(
                     "s", "[arp] request received; ",
                     "x", macaddr[0], "s", ":",
                     "x", macaddr[1], "s", ":",
                     "x", macaddr[2], "s", ":",
                     "x", macaddr[3], "s", ":",
                     "x", macaddr[4], "s", ":",
                     "x", macaddr[5], "s", " is ",
                     "d", (ipaddr >> 24) & 0xff, "s", ".",
                     "d", (ipaddr >> 16) & 0xff, "s", ".",
                     "d", (ipaddr >> 8) & 0xff, "s", ".",
                     "d", (ipaddr >> 0) & 0xff, "s", "\n");

        if(socket.TransmitPacket(ARPSocket::kOpARPReply, ipaddr, macaddr) < 0) {
	      gtty->Printf("s", "[arp] failed to transmit reply\n");
	    } else {
	      gtty->Printf("s", "[arp] reply sent\n");
	    }
      }
    }
  } else if (apic_ctrl->GetApicId() == 2) {
    volatile bool ready;
    while(true) {
      ready = apic_ctrl->IsBootupAll();
      if (ready) {
        break;
      }
    }
    while(true) {
      eth->UpdateLinkStatus();
      volatile bE1000::LinkStatus status = eth->GetStatus();
      if (status == bE1000::LinkStatus::Up) {
        break;
      }
    }

    ARPSocket socket;
    if(socket.Open() < 0) {
      gtty->Printf("s", "[error] failed to open socket\n");
    } else {
      socket.SetIPAddr(0x0a000205);
//      socket.SetIPAddr(0xc0a86475);
      cnt = timer->ReadMainCnt();
      if(socket.TransmitPacket(ARPSocket::kOpARPRequest, /*0xc0a86475*/0x0a00020f) < 0) {
	    gtty->Printf("s", "[arp] failed to transmit request\n");
	  } else {
	    gtty->Printf("s", "[arp] request sent\n");
	  }
    }
  }
  while(1) {
    asm volatile("hlt;");
  }
  return 0;
}

void kernel_panic(char *class_name, char *err_str) {
  gtty->Printf("s", "\n[","s",class_name,"s","] error: ","s",err_str);
  while(1) {
    asm volatile("hlt;");
  }
}

extern "C" void __cxa_pure_virtual()
{
  kernel_panic("", "");
}

#define RAND_MAX 0x7fff

uint32_t rand() {
  rnd_next = rnd_next * 1103515245 + 12345;
  /* return (unsigned int)(rnd_next / 65536) % 32768;*/
  return (uint32_t)(rnd_next >> 16) & RAND_MAX;
}
