OBJS = pci.o usb.o nic.o netdev.o storage.o 8042.o keyboard.o pciid.o pciid_table.o hpet.o framebuffer.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean usb.o nic.o storage.o

default: ../dev.o

-include $(DEPS)

../dev.o: $(OBJS)
	ld -r $^ -o $@

pciid_table.o: pciid_table.cc
	g++ $(CXXFLAGS) -c $^

pciid_table.cc: pci.ids.formatted pciid.py
	python pciid.py > pciid_table.cc

pci.ids.formatted : pci.ids
	sed 's/\("\|\?\|\\\)/\\\1/g' pci.ids > pci.ids.formatted

usb.o:
	@$(MAKE) -C usb

nic.o:
	@$(MAKE) -C nic

storage.o:
	@$(MAKE) -C storage

clean:
	$(MAKE) -C nic clean
	$(MAKE) -C storage clean
	$(MAKE) -C usb clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
	-rm -f pciid_table.cc pci.ids.formatted
