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
 * Author: Liva
 * 
 */

  #define ASM_FILE
  #include <gdt.h>

  .global lgdt_sub

lgdt_sub:
  pushq $KERNEL_CS
  pushq $jmp                    
  lgdt (%rdi)                   
  lretq                         
jmp:
  movw $KERNEL_DS, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %ss
  movw $0, %ax
  movw %ax, %fs
  movw %ax, %gs
  ret
