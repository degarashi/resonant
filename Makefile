include ./crosscompile/Makefile.mk
BUILD_PATH	= /var/tmp/resonant_build

update:
	$(call UpdateRepository,spinner)
	$(call UpdateRepository,boomstick)

