# Makefile fragment for deciding which VMM drivers to build

ALL_VMMS = vmware virtualbox kvm

NO_VMMS := $(foreach vmm,$(ALL_VMMS),-$(vmm))
# "no" is a dummy VMM that expands to nothing
VMMS_FILTERED_NONE := $(filter-out no,$(REQUESTED_VMMS))
# "all" expands to everything
VMMS_EXPANDED_ALL := $(patsubst all,$(ALL_VMMS),$(VMMS_FILTERED_NONE))
# "yes" expands to everything
VMMS_EXPANDED := $(patsubst yes,$(ALL_VMMS),$(VMMS_EXPANDED_ALL))
VMMS_INCLUDE := $(filter-out -%,$(VMMS_EXPANDED))
# "-foo" excludes foo from the list
VMMS_EXCLUDE := $(patsubst -%,%,$(filter -%,$(VMMS_EXPANDED)))
VMMS_CHOSEN := $(sort $(filter-out $(VMMS_EXCLUDE),$(VMMS_INCLUDE)))

VMMS := $(filter $(ALL_VMMS),$(VMMS_CHOSEN))
UNKNOWN_VMMS := $(filter-out $(ALL_VMMS),$(VMMS_CHOSEN))

.PHONY: list_chosen_vmms
list_chosen_vmms:
	@[ -n "$(VMMS)" ] && echo $(VMMS) ||:

.PHONY: list_invalid_vmms
list_invalid_vmms:
	@[ -n "$(UNKNOWN_VMMS)" ] && echo $(UNKNOWN_VMMS) ||:
