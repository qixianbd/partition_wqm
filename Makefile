TARGET =	partition
SUPER_PACKAGES_PATH = machsuif
LIBS =		-lfl -ldfa -lsuif1 -luseful -lmachine -lcfg -lm -lfl
LDFLAGS = -lfl
SRCS =		da_cfg.cpp  		\
		loop_block.cpp  	\
		main.cpp  		\
		super_block_cfg.cpp	\
		super_block.cpp  	\
		spawn_pos_trace.cpp	\
		misc.cpp		\
		spmt_instr.cpp		\
		threshold.cpp		\
		super_block_path.cpp	\
		thread.cpp		\
		spmt_utility.cpp	\
		XmlDocument.cpp		\
		XmlElement.cpp		\
		node.cpp		\
		helper.cpp		\
		min_cut_fun.cpp		\
		min_cut.cpp		\
		spmt_fun.cpp	\
		loop_cut.cpp	\
		route.cpp 

OBJS =		da_cfg.o		\
		loop_block.o		\
		main.o			\
		super_block_cfg.o	\
		super_block.o		\
		spawn_pos_trace.o	\
		misc.o			\
		spmt_instr.o		\
		threshold.o		\
		super_block_path.o	\
		thread.o		\
		spmt_utility.o		\
		XmlDocument.o		\
		XmlElement.o		\
		node.o			\
		helper.o			\
		min_cut_fun.o		\
		min_cut.o		\
		spmt_fun.o	\
		loop_cut.o	\
		route.o

include	$(SUIFHOME)/src/machsuif/Makefile.defs

all: 		prog

install-bin: 	install-prog

include $(SUIFHOME)/Makefile.std
include $(SUIFHOME)/src/machsuif/Makefile.std
