/*
 *
 * Copyright (c) 2015 Raphine Project
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
 * Author: Liva
 *
 */

#include <apic.h>
#include <cpu.h>
#include <gdt.h>
#include <global.h>
#include <idt.h>
#include <multiboot.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <raph_acpi.h>
#include <thread.h>
#include <timer.h>
#include <tty.h>
#include <shell.h>
#include <measure.h>
#include <mem/kstack.h>
#include <elf.h>
#include <syscall.h>

#include <dev/hpet.h>
#include <dev/pci.h>
#include <dev/usb/usb.h>
#include <dev/framebuffer.h>
#include <dev/pciid.h>
#include <dev/8042.h>
#include <dev/storage/storage.h>
#include <dev/storage/ramdisk.h>

#include <fs.h>
#include <v6fs.h>

#include <dev/netdev.h>
#include <dev/eth.h>
#include <net/arp.h>
#include <net/udp.h>
#include <net/dhcp.h>

AcpiCtrl *acpi_ctrl = nullptr;
ApicCtrl *apic_ctrl = nullptr;
MultibootCtrl *multiboot_ctrl = nullptr;
PhysmemCtrl *physmem_ctrl = nullptr;
MemCtrl *system_memory_space = nullptr;
Gdt *gdt = nullptr;
Idt *idt = nullptr;
Shell *shell = nullptr;
PciCtrl *pci_ctrl = nullptr;
NetDevCtrl *netdev_ctrl = nullptr;
// ArpTable *arp_table = nullptr;

MultibootCtrl _multiboot_ctrl;
AcpiCtrl _acpi_ctrl;
ApicCtrl _apic_ctrl;
CpuCtrl _cpu_ctrl;
Gdt _gdt;
Idt _idt;
PhysmemCtrl _physmem_ctrl;
Hpet _htimer;
FrameBuffer _framebuffer;
Shell _shell;
AcpicaPciCtrl _acpica_pci_ctrl;
NetDevCtrl _netdev_ctrl;
MemCtrl _system_memory_space;
// ArpTable _arp_table;

CpuId network_cpu;
CpuId pstack_cpu;

static uint32_t rnd_next = 1;

#include <dev/storage/ahci/ahci-raph.h>
AhciChannel *g_channel = nullptr;

void register_membench2_callout();

static void halt(int argc, const char *argv[]) { acpi_ctrl->Shutdown(); }

static void reset(int argc, const char *argv[]) { acpi_ctrl->Reset(); }

static void lspci(int argc, const char *argv[]) {
  MCFG *mcfg = acpi_ctrl->GetMCFG();
  if (mcfg == nullptr) {
    gtty->Printf("[Pci] error: could not find MCFG table.\n");
    return;
  }

  const char *search = nullptr;
  if (argc > 2) {
    gtty->Printf("[lspci] error: invalid argument\n");
    return;
  } else if (argc == 2) {
    search = argv[1];
  }

  PciData::Table table;
  table.Init();

  for (int i = 0;
       i * sizeof(MCFGSt) < mcfg->header.Length - sizeof(ACPISDTHeader); i++) {
    if (i == 1) {
      gtty->Printf("[Pci] info: multiple MCFG tables.\n");
      break;
    }
    for (int j = mcfg->list[i].pci_bus_start; j <= mcfg->list[i].pci_bus_end;
         j++) {
      for (int k = 0; k < 32; k++) {
        int maxf =
            ((pci_ctrl->ReadPciReg<uint16_t>(j, k, 0, PciCtrl::kHeaderTypeReg) &
              PciCtrl::kHeaderTypeRegFlagMultiFunction) != 0)
                ? 7
                : 0;
        for (int l = 0; l <= maxf; l++) {
          if (pci_ctrl->ReadPciReg<uint16_t>(j, k, l, PciCtrl::kVendorIDReg) ==
              0xffff) {
            continue;
          }
          table.Search(j, k, l, search);
        }
      }
    }
  }
}

static bool parse_ipaddr(const char *c, uint8_t *addr) {
  int i = 0;
  while (true) {
    if (i != 3 && *c == '.') {
      i++;
    } else if (i == 3 && *c == '\0') {
      return true;
    } else if (*c >= '0' && *c <= '9') {
      addr[i] *= 10;
      addr[i] += *c - '0';
    } else {
      return false;
    }
    c++;
  }
}

static void setip(int argc, const char *argv[]) {
  if (argc < 3) {
    gtty->Printf("invalid argument\n");
    return;
  }
  NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[1]);
  if (info == nullptr) {
    gtty->Printf("no ethernet interface(%s).\n", argv[1]);
    return;
  }
  NetDev *dev = info->device;

  uint8_t addr[4] = {0, 0, 0, 0};
  if (!strcmp(argv[2], "static")) {
    if (argc != 4) {
      gtty->Printf("invalid argument\n");
      return;
    }
    if (!parse_ipaddr(argv[3], addr)) {
      gtty->Printf("invalid ip v4 addr.\n");
      return;
    }
    dev->AssignIpv4Address(*(reinterpret_cast<uint32_t *>(addr)));
  } else if (!strcmp(argv[2], "dhcp")) {
    if (argc != 3) {
      gtty->Printf("invalid argument\n");
      return;
    }

    DhcpCtrl::GetCtrl().AssignAddr(dev);
  } else {
    gtty->Printf("invalid argument\n");
    return;
  }
}

static void ifconfig(int argc, const char *argv[]) {
  uptr<Array<const char *>> list = netdev_ctrl->GetNamesOfAllDevices();
  gtty->Printf("\n");
  for (size_t l = 0; l < list->GetLen(); l++) {
    gtty->Printf("%s", (*list)[l]);
    NetDev *dev = netdev_ctrl->GetDeviceInfo((*list)[l])->device;
    dev->UpdateLinkStatus();
    gtty->Printf("  link: %s\n", dev->IsLinkUp() ? "up" : "down");
  }
}

void setup_arp_reply(NetDev *dev);
void send_arp_packet(NetDev *dev, uint8_t *ipaddr);

static void bench(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "snd") == 0) {
    if (argc == 2) {
      gtty->Printf("specify ethernet interface.\n");
      return;
    } else if (argc == 3) {
      gtty->Printf("specify ip v4 addr.\n");
      return;
    } else if (argc != 4) {
      gtty->Printf("invalid arguments\n");
      return;
    }
    NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[2]);
    if (info == nullptr) {
      gtty->Printf("no ethernet interface(%s).\n", argv[2]);
      return;
    }
    NetDev *dev = info->device;

    uint8_t addr[4] = {0, 0, 0, 0};
    if (!parse_ipaddr(argv[3], addr)) {
      gtty->Printf("invalid ip v4 addr.\n");
      return;
    }

    send_arp_packet(dev, addr);
  } else if (strcmp(argv[1], "set_reply") == 0) {
    if (argc == 2) {
      gtty->Printf("specify ethernet interface.\n");
      return;
    }
    NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[2]);
    if (info == nullptr) {
      gtty->Printf("no ethernet interface(%s).\n", argv[2]);
      return;
    }
    setup_arp_reply(info->device);
  } /* else if (strcmp(argv[1], "qemu") == 0) {
    uint8_t ip1_[] = {10, 0, 2, 9};
    uint8_t ip2_[] = {10, 0, 2, 15};
    memcpy(ip1, ip1_, 4);
    memcpy(ip2, ip2_, 4);
    sdevice = (argc == 2) ? "eth0" : argv[2];
    rdevice = (argc == 2) ? "eth0" : argv[2];
    if (!netdev_ctrl->Exists(rdevice)) {
      gtty->Printf("no ethernet interface(%s).\n", rdevice);
      return;
    }
    } */ else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

static void setflag(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "spinlock_timeout") == 0) {
    if (argc == 2) {
      gtty->Printf("invalid argument.\n");
      return;
    }
    if (strcmp(argv[2], "true") == 0) {
      SpinLock::_spinlock_timeout = true;
    } else if (strcmp(argv[2], "false") == 0) {
      SpinLock::_spinlock_timeout = false;
    } else {
      gtty->Printf("invalid argument.\n");
      return;
    }
  } else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

static void udp_setup(int argc, const char *argv[]) {
  UdpCtrl::GetCtrl().SetupServer();
}

static void arp_scan(int argc, const char *argv[]) {
  auto devices = netdev_ctrl->GetNamesOfAllDevices();
  for (size_t i = 0; i < devices->GetLen(); i++) {
    auto dev = netdev_ctrl->GetDeviceInfo((*devices)[i])->device;
    if (dev->GetStatus() != NetDev::LinkStatus::kUp) {
      gtty->Printf("skip %s (Link Down)\n", (*devices)[i]);
      continue;
    }
    uint32_t my_addr_int_;
    if (!dev->GetIpv4Address(my_addr_int_) || my_addr_int_ == 0) {
      gtty->Printf("skip %s (no IP)\n", (*devices)[i]);
      continue;
    }
    gtty->Printf("ARP scan with %s\n", (*devices)[i]);
    dev->SetReceiveCallback(
        network_cpu,
        make_uptr(new Function<NetDev *>(
            [](NetDev *eth) {
              NetDev::Packet *rpacket;
              if (!eth->ReceivePacket(rpacket)) {
                return;
              }
              uint32_t my_addr_int;
              assert(eth->GetIpv4Address(my_addr_int));
              uint8_t my_addr[4];
              my_addr[0] = (my_addr_int >> 0) & 0xff;
              my_addr[1] = (my_addr_int >> 8) & 0xff;
              my_addr[2] = (my_addr_int >> 16) & 0xff;
              my_addr[3] = (my_addr_int >> 24) & 0xff;
              // received packet
              if (rpacket->GetBuffer()[12] == 0x08 &&
                  rpacket->GetBuffer()[13] == 0x06 &&
                  rpacket->GetBuffer()[21] == 0x02) {
                // ARP Reply
                uint32_t target_addr_int = (rpacket->GetBuffer()[31] << 24) |
                                           (rpacket->GetBuffer()[30] << 16) |
                                           (rpacket->GetBuffer()[29] << 8) |
                                           rpacket->GetBuffer()[28];
                arp_table->Set(target_addr_int, rpacket->GetBuffer() + 22, eth);
              }
              if (rpacket->GetBuffer()[12] == 0x08 &&
                  rpacket->GetBuffer()[13] == 0x06 &&
                  rpacket->GetBuffer()[21] == 0x01 &&
                  (memcmp(rpacket->GetBuffer() + 38, my_addr, 4) == 0)) {
                // ARP Request
                uint8_t data[] = {
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Target MAC Address
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source MAC Address
                    0x08, 0x06,                          // Type: ARP
                    // ARP Packet
                    0x00, 0x01,  // HardwareType: Ethernet
                    0x08, 0x00,  // ProtocolType: IPv4
                    0x06,        // HardwareLength
                    0x04,        // ProtocolLength
                    0x00, 0x02,  // Operation: ARP Reply
                    0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00,                    // Source Hardware Address
                    0x00, 0x00, 0x00, 0x00,  // Source Protocol Address
                    0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00,                    // Target Hardware Address
                    0x00, 0x00, 0x00, 0x00,  // Target Protocol Address
                };
                memcpy(data, rpacket->GetBuffer() + 6, 6);
                static_cast<DevEthernet *>(eth)->GetEthAddr(data + 6);
                memcpy(data + 22, data + 6, 6);
                memcpy(data + 28, my_addr, 4);
                memcpy(data + 32, rpacket->GetBuffer() + 22, 6);
                memcpy(data + 38, rpacket->GetBuffer() + 28, 4);

                uint32_t len = sizeof(data) / sizeof(uint8_t);
                NetDev::Packet *tpacket;
                kassert(eth->GetTxPacket(tpacket));
                memcpy(tpacket->GetBuffer(), data, len);
                tpacket->len = len;
                eth->TransmitPacket(tpacket);
              }
              eth->ReuseRxBuffer(rpacket);
            },
            dev)));
    for (int j = 1; j < 255; j++) {
      uint32_t my_addr_int;
      assert(dev->GetIpv4Address(my_addr_int));
      uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kShared);
      do {
        auto t_op = thread->CreateOperator();
        struct Container {
          NetDev *eth;
          uint8_t target_addr[4];
        };
        auto container_ = make_uptr(new Container);
        container_->eth = dev;
        container_->target_addr[0] = (my_addr_int >> 0) & 0xff;
        container_->target_addr[1] = (my_addr_int >> 8) & 0xff;
        container_->target_addr[2] = (my_addr_int >> 16) & 0xff;
        container_->target_addr[3] = j;
        t_op.SetFunc(make_uptr(new Function<uptr<Container>>([](uptr<Container> container) {
                uint8_t data[] = {
                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // Target MAC Address
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source MAC Address
                  0x08, 0x06,                          // Type: ARP
                  // ARP Packet
                  0x00, 0x01,                          // HardwareType: Ethernet
                  0x08, 0x00,                          // ProtocolType: IPv4
                  0x06,                                // HardwareLength
                  0x04,                                // ProtocolLength
                  0x00, 0x01,                          // Operation: ARP Request
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source Hardware Address
                  0x00, 0x00, 0x00, 0x00,              // Source Protocol Address
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Target Hardware Address
                  // Target Protocol Address
                  0x00, 0x00, 0x00, 0x00};
                static_cast<DevEthernet *>(container->eth)->GetEthAddr(data + 6);
                memcpy(data + 22, data + 6, 6);
                uint32_t my_addr;
                assert(container->eth->GetIpv4Address(my_addr));
                data[28] = (my_addr >> 0) & 0xff;
                data[29] = (my_addr >> 8) & 0xff;
                data[30] = (my_addr >> 16) & 0xff;
                data[31] = (my_addr >> 24) & 0xff;
                memcpy(data + 38, container->target_addr, 4);
                uint32_t len = sizeof(data) / sizeof(uint8_t);
                NetDev::Packet *tpacket;
                kassert(container->eth->GetTxPacket(tpacket));
                memcpy(tpacket->GetBuffer(), data, len);
                tpacket->len = len;
                container->eth->TransmitPacket(tpacket);
              },
              container_)));
        t_op.Schedule();
      } while(0);
      thread->Join();
    }
  }
  uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kShared);
  do {
    auto t_op = thread->CreateOperator();
    t_op.SetFunc(make_uptr(new Function<void *>([](void *) {
            auto devices_ = netdev_ctrl->GetNamesOfAllDevices();
            for (size_t i = 0; i < devices_->GetLen(); i++) {
              auto dev = netdev_ctrl->GetDeviceInfo((*devices_)[i])->device;
              dev->SetReceiveCallback(network_cpu,
                                      make_uptr(new GenericFunction<>()));
            }
          }, nullptr)));
    t_op.Schedule(3 * 1000 * 1000);
  } while(0);
  thread->Join();
}

ArpTable *arp_table = nullptr;

void udpsend(int argc, const char *argv[]) {
  if (argc != 4) {
    gtty->Printf("invalid argument.\n");
    return;
  }

  uint8_t target_addr[4] = {0, 0, 0, 0};
  if (!parse_ipaddr(argv[1], target_addr)) {
    gtty->Printf("invalid ip v4 addr.\n");
    return;
  }
  UdpCtrl::GetCtrl().SendStr(&target_addr, atoi(argv[2]), argv[3]);
}

static void show(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "module") == 0) {
    if (argc != 2) {
      gtty->Printf("invalid argument.\n");
      return;
    }
    multiboot_ctrl->ShowModuleInfo();
  } else if (strcmp(argv[1], "info") == 0) {
    gtty->Printf("[kernel] info: Hardware Information\n");
    gtty->Printf("available cpu thread num: %d\n", cpu_ctrl->GetHowManyCpus());
    gtty->Printf("\n");
    gtty->Printf("[kernel] info: Build Information\n");
    multiboot_ctrl->ShowBuildTimeStamp();
  } else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

static void wait_until_linkup(int argc, const char *argv[]) {
  if (argc != 2) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[1]);
  if (info == nullptr) {
    gtty->Printf("no such device.\n");
    return;
  }
  NetDev *dev_ = info->device;
  uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kShared);
  do {
    Thread::Operator t_op = thread->CreateOperator();
    t_op.SetFunc(make_uptr(new Function<NetDev *>([](NetDev *dev) {
            if (!dev->IsLinkUp()) {
              ThreadCtrl::GetCurrentThreadOperator().Schedule(1000 * 1000);
            }
          }, dev_)));
    t_op.Schedule();
  } while(0);
  thread->Join();
}

static void load_script(uptr<Array<uint8_t>> data) {
  size_t old_index = 0;
  size_t index = 0;
  while (index <= data->GetLen()) {
    if (index == data->GetLen() ||
        (*data)[index] == '\r' ||
        (*data)[index] == '\n' ||
         (*data)[index] == '\0') {
      if (old_index != index) {
        char buffer[index - old_index + 1];
        memcpy(buffer,
               reinterpret_cast<char *>(data->GetRawPtr()) + old_index,
               index - old_index);
        buffer[index - old_index] = '\0';
        auto ec = make_uptr(new Shell::ExecContainer(shell));
        ec = shell->Tokenize(ec, buffer);
        int timeout = 0;
        if (strlen(buffer) != 0) {
          gtty->Printf("> %s\n", buffer);
          if (strcmp(ec->argv[0], "wait") == 0) {
            if (ec->argc == 2) {
              int t = 0;
              for (size_t l = 0; l < strlen(ec->argv[1]); l++) {
                if ('0' > ec->argv[1][l] || ec->argv[1][l] > '9') {
                  gtty->Printf("invalid argument.\n");
                  t = 0;
                  break;
                }
                t = t * 10 + ec->argv[1][l] - '0';
              }
              timeout = t * 1000 * 1000;
            } else {
              gtty->Printf("invalid argument.\n");
            }
          } else if (strcmp(ec->argv[0], "wait_until_linkup") == 0) {
            wait_until_linkup(ec->argc, ec->argv);
          } else {
            shell->Execute(ec);
          }
        }
        if (timeout != 0) {
          ThreadCtrl::GetCurrentThreadOperator().Schedule(timeout);
          ThreadCtrl::WaitCurrentThread();
        }
      }
      if (index == data->GetLen() - 1 || index == data->GetLen() || (*data)[index] == '\0') {
        break;
      }
      old_index = index + 1;
    }
    index++;
  }
}

static void load(int argc, const char *argv[]) {
  if (argc != 2) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "script.sh") == 0) {
    load_script(multiboot_ctrl->LoadFile(argv[1]));
  } else if (strcmp(argv[1], "test.elf") == 0) {
    auto buf_ = multiboot_ctrl->LoadFile(argv[1]);
    uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kIndependent);
    do {
      auto t_op = thread->CreateOperator();
      t_op.SetFunc(make_uptr(new Function<uptr<Array<uint8_t>>>([](uptr<Array<uint8_t>> buf) {
              Loader loader;
              ElfObject obj(loader, buf->GetRawPtr());
              if (obj.Init() != BinObjectInterface::ErrorState::kOk) {
                gtty->Printf("error while loading app\n");
              } else {
                obj.Execute();
              }
            }, buf_)));
      t_op.Schedule();
    } while(0);
    thread->Join();
  } else if (strcmp(argv[1], "rump.bin") == 0) {
    auto buf_ = multiboot_ctrl->LoadFile(argv[1]);
    uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kIndependent);
    do {
      auto t_op = thread->CreateOperator();
      t_op.SetFunc(make_uptr(new Function<uptr<Array<uint8_t>>>([](uptr<Array<uint8_t>> buf) {
              Ring0Loader loader;
              RaphineRing0AppObject obj(loader, buf->GetRawPtr());
              if (obj.Init() != BinObjectInterface::ErrorState::kOk) {
                gtty->Printf("error while loading app\n");
              } else {
                obj.Execute();
              }
            }, buf_)));
      t_op.Schedule();
    } while(0);
    thread->Join();
  } else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

static void remote_load(int argc, const char *argv[]) {
  if (argc != 2) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "test.elf") == 0) {
    SystemCallCtrl::GetCtrl().SetMode(SystemCallCtrl::Mode::kRemote);
    auto buf_ = multiboot_ctrl->LoadFile(argv[1]);
    uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kShared);
    do {
      auto t_op = thread->CreateOperator();
      t_op.SetFunc(make_uptr(new Function<uptr<Array<uint8_t>>>([](uptr<Array<uint8_t>> buf) {
              Loader loader;
              ElfObject obj(loader, buf->GetRawPtr());
              if (obj.Init() != BinObjectInterface::ErrorState::kOk) {
                gtty->Printf("error while loading app\n");
              } else {
                obj.Execute();
              }
            }, buf_)));
      t_op.Schedule();
    } while(0);
    thread->Join();
  } else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

void beep(int argc, const char *argv[]) {
  uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kShared);
  do {
    auto t_op = thread->CreateOperator();
    t_op.SetFunc(make_uptr(new Function<void *>([](void *) {
            static int i = 0;
            if (i < 6) {
              uint16_t sound[6] = {905, 761, 452, 570, 508, 380};
              uint8_t a = 0xb6;
              outb(0x43, a);
              uint8_t l = sound[i] & 0x00FF;
              outb(0x42, l);
              uint8_t h = (sound[i] >> 8) & 0x00FF;
              outb(0x42, h);
              uint8_t on = inb(0x61);
              outb(0x61, (on | 0x03) & 0x0f);
              i++;
              ThreadCtrl::GetCurrentThreadOperator().Schedule(110000);
            } else {
              uint8_t off = inb(0x61);
              outb(0x61, off & 0xd);
            }
          }, nullptr)));
    t_op.Schedule();
  } while(0);
  thread->Join();
}

static void membench(int argc, const char *argv[]) {
  register_membench2_callout();
}

bool cat_sub(VirtualFileSystem *vfs, const char *path) {
  InodeContainer inode;
  if (vfs->LookupInodeFromPath(inode, path, false) != IoReturnState::kOk)
    return false;
  VirtualFileSystem::Stat st;
  if (inode.GetStatOfInode(st) != IoReturnState::kOk) return false;
  size_t s = st.size;
  uint8_t buf[s];
  if (inode.ReadData(buf, 0, s) != IoReturnState::kOk) return false;
  for (size_t i = 0; i < s; i++) {
    gtty->Printf("%c", buf[i]);
  }
  gtty->Printf("\n");
  return true;
}

void cat(int argc, const char *argv[]) {
  if (argc < 2) {
    gtty->Printf("usage: cat <filename>\n");
    return;
  }
  Storage *storage;
  if (StorageCtrl::GetCtrl().GetDevice("ram0", storage) != IoReturnState::kOk) {
    kernel_panic("storage", "not found");
  }
  V6FileSystem *v6fs = new V6FileSystem(*storage);
  VirtualFileSystem *vfs = new VirtualFileSystem(v6fs);
  vfs->Init();
  if (!cat_sub(vfs, argv[1])) {
    gtty->Printf("cat: %s: No such file or directory\n", argv[1]);
  }
}

void freebsd_main();

extern "C" int main() {
  _system_memory_space.GetKernelVirtmemCtrl()->Init();

  multiboot_ctrl = new (&_multiboot_ctrl) MultibootCtrl;

  acpi_ctrl = new (&_acpi_ctrl) AcpiCtrl;

  apic_ctrl = new (&_apic_ctrl) ApicCtrl;

  cpu_ctrl = new (&_cpu_ctrl) CpuCtrl;

  gdt = new (&_gdt) Gdt;

  idt = new (&_idt) Idt;

  system_memory_space = new (&_system_memory_space) MemCtrl;

  physmem_ctrl = new (&_physmem_ctrl) PhysmemCtrl;

  timer = new (&_htimer) Hpet;

  gtty = new (&_framebuffer) FrameBuffer;

  shell = new (&_shell) Shell;

  netdev_ctrl = new (&_netdev_ctrl) NetDevCtrl();

  // arp_table = new (&_arp_table) ArpTable();

  physmem_ctrl->Init();
  
  multiboot_ctrl->Setup();

  system_memory_space->GetKernelVirtmemCtrl()->InitKernelMemorySpace();

  system_memory_space->Init();

  gtty->Init();

  multiboot_ctrl->ShowMemoryInfo();

  KernelStackCtrl::Init();

  apic_ctrl->Init();

  acpi_ctrl->Setup();

  if (timer->Setup()) {
    gtty->Printf("[timer] info: HPET supported.\n");
  } else {
    kernel_panic("timer", "HPET not supported.\n");
  }

  apic_ctrl->Setup();

  cpu_ctrl->Init();

  network_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);

  pstack_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);

  rnd_next = timer->ReadTime().GetRaw();

  ThreadCtrl::Init();

  idt->SetupGeneric();

  apic_ctrl->BootBSP();

  gdt->SetupProc();

  // 各コアは最低限の初期化ののち、ThreadCtrlに制御が移さなければならない
  // 特定のコアで専用の処理をさせたい場合は、ThreadCtrlに登録したジョブとして
  // 実行する事
  apic_ctrl->StartAPs();

  system_memory_space->GetKernelVirtmemCtrl()->ReleaseLowMemory();

  gtty->Setup();

  idt->SetupProc();

  pci_ctrl = new (&_acpica_pci_ctrl) AcpicaPciCtrl;

  pci_ctrl->Probe();

  acpi_ctrl->SetupAcpica();

  UsbCtrl::Init();

  freebsd_main();

  AttachDevices<PciCtrl, LegacyKeyboard, Ramdisk, Device>();
  
  SystemCallCtrl::Init();

  gtty->Printf("\n\n[kernel] info: initialization completed\n");

  arp_table = new ArpTable;
  UdpCtrl::Init();
  DhcpCtrl::Init();

  shell->Setup();
  shell->Register("halt", halt);
  shell->Register("reset", reset);
  shell->Register("bench", bench);
  shell->Register("beep", beep);
  shell->Register("lspci", lspci);
  shell->Register("ifconfig", ifconfig);
  shell->Register("setip", setip);
  shell->Register("show", show);
  shell->Register("load", load);
  shell->Register("remote_load", remote_load);
  shell->Register("setflag", setflag);
  shell->Register("udpsend", udpsend);
  shell->Register("arp_scan", arp_scan);
  shell->Register("udp_setup", udp_setup);
  shell->Register("membench", membench);
  shell->Register("cat", cat);

  auto thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kIndependent);
  auto t_op = thread->CreateOperator();
  t_op.SetFunc(make_uptr(new Function<void *>([](void *) {
          load_script(multiboot_ctrl->LoadFile("init.sh"));
        }, nullptr)));
  t_op.Schedule();

  ThreadCtrl::GetCurrentCtrl().Run();

  return 0;
}

extern "C" int main_of_others() {
  // according to mp spec B.3, system should switch over to Symmetric I/O mode

  apic_ctrl->BootAP();

  gdt->SetupProc();
  idt->SetupProc();

  ThreadCtrl::GetCurrentCtrl().Run();
  return 0;
}

void show_backtrace(size_t *rbp) {
  size_t top_rbp = reinterpret_cast<size_t>(rbp);
  for (int i = 0; i < 10; i++) {
    if (top_rbp >= rbp[0] || top_rbp <= rbp[0] - 4096) {
      break;
    }
    gtty->ErrPrintf("backtrace(%d): rip:%llx,\n", i, rbp[1]);
    rbp = reinterpret_cast<size_t *>(rbp[0]);
  }
}

extern "C" void _kernel_panic(const char *class_name, const char *err_str) {
  if (gtty != nullptr) {
    gtty->DisablePrint();
    gtty->ErrPrintf("\n!!!! Kernel Panic !!!!\n");
    gtty->ErrPrintf("[%s] error: %s\n", class_name, err_str);
    gtty->ErrPrintf("\n");
    gtty->ErrPrintf(">> debugging information >>\n");
    gtty->ErrPrintf("cpuid: %d\n", cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0" : "=r"(rbp));
    show_backtrace(rbp);
  }
  while (true) {
    asm volatile("cli;hlt;");
  }
}

extern "C" void checkpoint(int id, const char *str) {
  if (id < 0 || cpu_ctrl->GetCpuId().GetRawId() == id) {
    gtty->Printf(str);
    gtty->Flush();
  }
}

extern "C" void _checkpoint(const char *func, const int line) {
  gtty->Printf("[%s:%d]", func, line);
  gtty->Flush();
}

extern "C" void abort() {
  if (gtty != nullptr) {
    gtty->DisablePrint();
    gtty->ErrPrintf("system stopped by unexpected error.\n");
    size_t *rbp;
    asm volatile("movq %%rbp, %0" : "=r"(rbp));
    show_backtrace(rbp);
  }
  while (true) {
    asm volatile("cli;hlt");
  }
}

extern "C" void _kassert(const char *file, int line, const char *func) {
  if (gtty != nullptr) {
    gtty->DisablePrint();
    gtty->ErrPrintf("assertion failed at %s l.%d (%s) cpuid: %d\n", file, line,
                    func, cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0" : "=r"(rbp));
    show_backtrace(rbp);
  }
  while (true) {
    asm volatile("cli;hlt");
  }
}

extern "C" void __cxa_pure_virtual() { kernel_panic("", ""); }

extern "C" void __stack_chk_fail() { kernel_panic("", ""); }

#define RAND_MAX 0x7fff

extern "C" uint32_t rand() {
  rnd_next = rnd_next * 1103515245 + 12345;
  /* return (unsigned int)(rnd_next / 65536) % 32768;*/
  return (uint32_t)(rnd_next >> 16) & RAND_MAX;
}
