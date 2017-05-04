OBJS = em.o ix.o

DEPS= $(filter %.d, $(subst .o,.d, $(OBJS)))

.PHONY: clean em.o ix.o

default: ../intel.o

-include $(DEPS)

../intel.o: $(OBJS)
	ld -r $^ -o $@

em.o:
	@$(MAKE) -C em

ix.o:
	@$(MAKE) -C ix

clean:
	$(MAKE) -C em clean
	$(MAKE) -C ix clean
	-rm -f $(OBJS) $(TEST_OBJS) $(DEPS) *.s *.ii
