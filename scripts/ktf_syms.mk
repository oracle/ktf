ktf_symfile=$(shell (cd $(srctree)/$(src) && ls ktf_syms.txt 2> /dev/null || true))
ktf_syms = $(ktf_symfile:%.txt=%.h)

ifneq ($(ktf_symfile),)

$(obj)/self.o: $(obj)/$(ktf_syms)

ktf_scripts = $(srctree)/$(src)/../scripts

$(obj)/$(ktf_syms): $(srctree)/$(src)/ktf_syms.txt $(ktf_scripts)/resolve
	@echo "  KTFSYMS $@"
	$(Q)$(ktf_scripts)/resolve $(ccflags-y) $< $@

clean-files += $(ktf_syms)

endif
