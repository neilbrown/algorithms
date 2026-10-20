
mkfiles = $(wildcard *.mk)
mdfiles = $(wildcard *.mdc)
mdkfiles = $(mdfiles:.mdc=.mk)
missingmk = $(filter-out $(mkfiles),$(mdkfiles))
ifneq "$(mkfiles)" ""
include $(mkfiles)
endif

bootstrap:
	touch boot-strap/* # make sure mdcode.mdc isn't newer
	$(MAKE) -C boot-strap -f mdcode.mk VPATH=..
	cp boot-strap/md2c .
	./md2c mdcode.mdc
	@echo =========================
	@echo 'Please run "make" again'

md2c = $(wildcard md2c)
ifeq "$(md2c)" "md2c"
ifneq "$(missingmk)" ""
-include $(missingmk)
endif
endif

%.mk : %.mdc md2c
	./md2c $*.mdc

