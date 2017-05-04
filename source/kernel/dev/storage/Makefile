OBJS = ahci.o ramdisk.o storage.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean ahci.o

default: ../storage.o

-include $(DEPS)

../storage.o: $(OBJS)
	ld -r $^ -o $@

ahci.o:
	@$(MAKE) -C ahci

clean:
	$(MAKE) -C ahci clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
