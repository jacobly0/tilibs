EXTRA_DIST = romdump.z80
CLEANFILES = *.bin *.lst *.8xp

SPASMFLAGS = -N -T

if BUILD_Z80_ROM_DUMPERS
all: dump84p.8xp dump84pcu.8xp
endif

dump84p.bin: romdump.z80
	$(SPASM) $(SPASMFLAGS) -A -DTI84P $(srcdir)/romdump.z80 dump84p.bin
dump84p.8xp: dump84p.bin
	$(TIPACK) -n 'ROMDUMP' dump84p.bin -p -o dump84p.8xp

dump84pcu.bin: romdump.z80
	$(SPASM) $(SPASMFLAGS) -A -DTI84PC $(srcdir)/romdump.z80 dump84pcu.bin
dump84pcu.8xp: dump84pcu.bin
	$(TIPACK) -n 'ROMDUMP' dump84pcu.bin -p -o dump84pcu.8xp
