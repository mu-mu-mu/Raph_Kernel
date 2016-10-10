/*-
 * Copyright (c) 1997, Stefan Esser <se@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 *
 */

#ifndef _FREEBSD_PCIREG_H_
#define _FREEBSD_PCIREG_H_

#include "../../../dev/pci.h"

#define PCIR_COMMAND PciCtrl::kCommandReg
#define PCIR_REVID PciCtrl::kRegRevisionId
#define PCIR_SUBVEND_0 PciCtrl::kSubsystemVendorIdReg
#define PCIR_SUBDEV_0 PciCtrl::kSubsystemIdReg

#define	PCIY_PMG	0x01	/* PCI Power Management */
#define	PCIY_AGP	0x02	/* AGP */
#define	PCIY_VPD	0x03	/* Vital Product Data */
#define	PCIY_SLOTID	0x04	/* Slot Identification */
#define	PCIY_MSI	0x05	/* Message Signaled Interrupts */
#define	PCIY_CHSWP	0x06	/* CompactPCI Hot Swap */
#define	PCIY_PCIX	0x07	/* PCI-X */
#define	PCIY_HT		0x08	/* HyperTransport */
#define	PCIY_VENDOR	0x09	/* Vendor Unique */
#define	PCIY_DEBUG	0x0a	/* Debug port */
#define	PCIY_CRES	0x0b	/* CompactPCI central resource control */
#define	PCIY_HOTPLUG	0x0c	/* PCI Hot-Plug */
#define	PCIY_SUBVENDOR	0x0d	/* PCI-PCI bridge subvendor ID */
#define	PCIY_AGP8X	0x0e	/* AGP 8x */
#define	PCIY_SECDEV	0x0f	/* Secure Device */
#define	PCIY_EXPRESS	0x10	/* PCI Express */
#define	PCIY_MSIX	0x11	/* MSI-X */
#define	PCIY_SATA	0x12	/* SATA */
#define	PCIY_PCIAF	0x13	/* PCI Advanced Features */
#define	PCIY_EA		0x14	/* PCI Extended Allocation */

#define PCIR_BARS 0x10
#define PCIR_BAR(x) (PCIR_BARS + (x) * 4)
#define PCIR_CIS 0x28

/* PCI Express definitions */
#define	PCIER_FLAGS		0x2
#define	PCIEM_FLAGS_VERSION		0x000F
#define	PCIEM_FLAGS_TYPE		0x00F0
#define	PCIEM_TYPE_ENDPOINT		0x0000
#define	PCIEM_TYPE_LEGACY_ENDPOINT	0x0010
#define	PCIEM_TYPE_ROOT_PORT		0x0040
#define	PCIEM_TYPE_UPSTREAM_PORT	0x0050
#define	PCIEM_TYPE_DOWNSTREAM_PORT	0x0060
#define	PCIEM_TYPE_PCI_BRIDGE		0x0070
#define	PCIEM_TYPE_PCIE_BRIDGE		0x0080
#define	PCIEM_TYPE_ROOT_INT_EP		0x0090
#define	PCIEM_TYPE_ROOT_EC		0x00a0
#define	PCIEM_FLAGS_SLOT		0x0100
#define	PCIEM_FLAGS_IRQ			0x3e00
#define	PCIER_DEVICE_CAP	0x4
#define	PCIEM_CAP_MAX_PAYLOAD		0x00000007
#define	PCIEM_CAP_PHANTHOM_FUNCS	0x00000018
#define	PCIEM_CAP_EXT_TAG_FIELD		0x00000020
#define	PCIEM_CAP_L0S_LATENCY		0x000001c0
#define	PCIEM_CAP_L1_LATENCY		0x00000e00
#define	PCIEM_CAP_ROLE_ERR_RPT		0x00008000
#define	PCIEM_CAP_SLOT_PWR_LIM_VAL	0x03fc0000
#define	PCIEM_CAP_SLOT_PWR_LIM_SCALE	0x0c000000
#define	PCIEM_CAP_FLR			0x10000000
#define	PCIER_DEVICE_CTL	0x8
#define	PCIEM_CTL_COR_ENABLE		0x0001
#define	PCIEM_CTL_NFER_ENABLE		0x0002
#define	PCIEM_CTL_FER_ENABLE		0x0004
#define	PCIEM_CTL_URR_ENABLE		0x0008
#define	PCIEM_CTL_RELAXED_ORD_ENABLE	0x0010
#define	PCIEM_CTL_MAX_PAYLOAD		0x00e0
#define	PCIEM_CTL_EXT_TAG_FIELD		0x0100
#define	PCIEM_CTL_PHANTHOM_FUNCS	0x0200
#define	PCIEM_CTL_AUX_POWER_PM		0x0400
#define	PCIEM_CTL_NOSNOOP_ENABLE	0x0800
#define	PCIEM_CTL_MAX_READ_REQUEST	0x7000
#define	PCIEM_CTL_BRDG_CFG_RETRY	0x8000	/* PCI-E - PCI/PCI-X bridges */
#define	PCIEM_CTL_INITIATE_FLR		0x8000	/* FLR capable endpoints */
#define	PCIER_DEVICE_STA	0xa
#define	PCIEM_STA_CORRECTABLE_ERROR	0x0001
#define	PCIEM_STA_NON_FATAL_ERROR	0x0002
#define	PCIEM_STA_FATAL_ERROR		0x0004
#define	PCIEM_STA_UNSUPPORTED_REQ	0x0008
#define	PCIEM_STA_AUX_POWER		0x0010
#define	PCIEM_STA_TRANSACTION_PND	0x0020
#define	PCIER_LINK_CAP		0xc
#define	PCIEM_LINK_CAP_MAX_SPEED	0x0000000f
#define	PCIEM_LINK_CAP_MAX_WIDTH	0x000003f0
#define	PCIEM_LINK_CAP_ASPM		0x00000c00
#define	PCIEM_LINK_CAP_L0S_EXIT		0x00007000
#define	PCIEM_LINK_CAP_L1_EXIT		0x00038000
#define	PCIEM_LINK_CAP_CLOCK_PM		0x00040000
#define	PCIEM_LINK_CAP_SURPRISE_DOWN	0x00080000
#define	PCIEM_LINK_CAP_DL_ACTIVE	0x00100000
#define	PCIEM_LINK_CAP_LINK_BW_NOTIFY	0x00200000
#define	PCIEM_LINK_CAP_ASPM_COMPLIANCE	0x00400000
#define	PCIEM_LINK_CAP_PORT		0xff000000
#define	PCIER_LINK_CTL		0x10
#define	PCIEM_LINK_CTL_ASPMC_DIS	0x0000
#define	PCIEM_LINK_CTL_ASPMC_L0S	0x0001
#define	PCIEM_LINK_CTL_ASPMC_L1		0x0002
#define	PCIEM_LINK_CTL_ASPMC		0x0003
#define	PCIEM_LINK_CTL_RCB		0x0008
#define	PCIEM_LINK_CTL_LINK_DIS		0x0010
#define	PCIEM_LINK_CTL_RETRAIN_LINK	0x0020
#define	PCIEM_LINK_CTL_COMMON_CLOCK	0x0040
#define	PCIEM_LINK_CTL_EXTENDED_SYNC	0x0080
#define	PCIEM_LINK_CTL_ECPM		0x0100
#define	PCIEM_LINK_CTL_HAWD		0x0200
#define	PCIEM_LINK_CTL_LBMIE		0x0400
#define	PCIEM_LINK_CTL_LABIE		0x0800
#define	PCIER_LINK_STA		0x12
#define	PCIEM_LINK_STA_SPEED		0x000f
#define	PCIEM_LINK_STA_WIDTH		0x03f0
#define	PCIEM_LINK_STA_TRAINING_ERROR	0x0400
#define	PCIEM_LINK_STA_TRAINING		0x0800
#define	PCIEM_LINK_STA_SLOT_CLOCK	0x1000
#define	PCIEM_LINK_STA_DL_ACTIVE	0x2000
#define	PCIEM_LINK_STA_LINK_BW_MGMT	0x4000
#define	PCIEM_LINK_STA_LINK_AUTO_BW	0x8000
#define	PCIER_SLOT_CAP		0x14
#define	PCIEM_SLOT_CAP_APB		0x00000001
#define	PCIEM_SLOT_CAP_PCP		0x00000002
#define	PCIEM_SLOT_CAP_MRLSP		0x00000004
#define	PCIEM_SLOT_CAP_AIP		0x00000008
#define	PCIEM_SLOT_CAP_PIP		0x00000010
#define	PCIEM_SLOT_CAP_HPS		0x00000020
#define	PCIEM_SLOT_CAP_HPC		0x00000040
#define	PCIEM_SLOT_CAP_SPLV		0x00007f80
#define	PCIEM_SLOT_CAP_SPLS		0x00018000
#define	PCIEM_SLOT_CAP_EIP		0x00020000
#define	PCIEM_SLOT_CAP_NCCS		0x00040000
#define	PCIEM_SLOT_CAP_PSN		0xfff80000
#define	PCIER_SLOT_CTL		0x18
#define	PCIEM_SLOT_CTL_ABPE		0x0001
#define	PCIEM_SLOT_CTL_PFDE		0x0002
#define	PCIEM_SLOT_CTL_MRLSCE		0x0004
#define	PCIEM_SLOT_CTL_PDCE		0x0008
#define	PCIEM_SLOT_CTL_CCIE		0x0010
#define	PCIEM_SLOT_CTL_HPIE		0x0020
#define	PCIEM_SLOT_CTL_AIC		0x00c0
#define	PCIEM_SLOT_CTL_PIC		0x0300
#define	PCIEM_SLOT_CTL_PCC		0x0400
#define	PCIEM_SLOT_CTL_EIC		0x0800
#define	PCIEM_SLOT_CTL_DLLSCE		0x1000
#define	PCIER_SLOT_STA		0x1a
#define	PCIEM_SLOT_STA_ABP		0x0001
#define	PCIEM_SLOT_STA_PFD		0x0002
#define	PCIEM_SLOT_STA_MRLSC		0x0004
#define	PCIEM_SLOT_STA_PDC		0x0008
#define	PCIEM_SLOT_STA_CC		0x0010
#define	PCIEM_SLOT_STA_MRLSS		0x0020
#define	PCIEM_SLOT_STA_PDS		0x0040
#define	PCIEM_SLOT_STA_EIS		0x0080
#define	PCIEM_SLOT_STA_DLLSC		0x0100
#define	PCIER_ROOT_CTL		0x1c
#define	PCIEM_ROOT_CTL_SERR_CORR	0x0001
#define	PCIEM_ROOT_CTL_SERR_NONFATAL	0x0002
#define	PCIEM_ROOT_CTL_SERR_FATAL	0x0004
#define	PCIEM_ROOT_CTL_PME		0x0008
#define	PCIEM_ROOT_CTL_CRS_VIS		0x0010
#define	PCIER_ROOT_CAP		0x1e
#define	PCIEM_ROOT_CAP_CRS_VIS		0x0001
#define	PCIER_ROOT_STA		0x20
#define	PCIEM_ROOT_STA_PME_REQID_MASK	0x0000ffff
#define	PCIEM_ROOT_STA_PME_STATUS	0x00010000
#define	PCIEM_ROOT_STA_PME_PEND		0x00020000
#define	PCIER_DEVICE_CAP2	0x24
#define	PCIEM_CAP2_ARI		0x20
#define	PCIER_DEVICE_CTL2	0x28
#define	PCIEM_CTL2_COMP_TIMEOUT_VAL	0x000f
#define	PCIEM_CTL2_COMP_TIMEOUT_DIS	0x0010
#define	PCIEM_CTL2_ARI			0x0020
#define	PCIEM_CTL2_ATOMIC_REQ_ENABLE	0x0040
#define	PCIEM_CTL2_ATOMIC_EGR_BLOCK	0x0080
#define	PCIEM_CTL2_ID_ORDERED_REQ_EN	0x0100
#define	PCIEM_CTL2_ID_ORDERED_CMP_EN	0x0200
#define	PCIEM_CTL2_LTR_ENABLE		0x0400
#define	PCIEM_CTL2_OBFF			0x6000
#define	PCIEM_OBFF_DISABLE		0x0000
#define	PCIEM_OBFF_MSGA_ENABLE		0x2000
#define	PCIEM_OBFF_MSGB_ENABLE		0x4000
#define	PCIEM_OBFF_WAKE_ENABLE		0x6000
#define	PCIEM_CTL2_END2END_TLP		0x8000
#define	PCIER_DEVICE_STA2	0x2a
#define	PCIER_LINK_CAP2		0x2c
#define	PCIER_LINK_CTL2		0x30
#define	PCIER_LINK_STA2		0x32
#define	PCIER_SLOT_CAP2		0x34
#define	PCIER_SLOT_CTL2		0x38
#define	PCIER_SLOT_STA2		0x3a

#endif /* _FREEBSD_PCIREG_H_ */
