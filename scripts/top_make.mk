ifneq ($(TARGETS),)
# We end up here if called from selftests/Makefile
# Invoke our "module target" to get everything built
all:
	$(Q)$(MAKE) -C $(abs_objtree) M=tools/testing/selftests/ktf

clean:
	$(Q)$(MAKE) -C $(abs_objtree) M=tools/testing/selftests/ktf clean

run_tests:
	@echo "running tests"
	$(MAKE) BUILD=$(abs_objtree)/tools/testing/selftests -f scripts/runtests.mk $@

endif
