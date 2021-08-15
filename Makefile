# Please Enter Your RTS SDK PATH {Run for v4.1 | v4.1.1, Build: rts3916_nand_base}
#SDK_ROOT = ../../../../sdk/rts39xx_sdk_v4.1

# ==============================================================================================

# TOOLCHAIN = /opt/rsdk-6.5.0-5281-EL-4.9-u1.0-m32fut-191025p1
# SDK_DIR = ${SDK_ROOT}/out/edimax_rts3916_ic3340idn/staging/usr

# CROSS_COMPILE = ${TOOLCHAIN}/bin/mips-linux-

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

# TOOLCHAIN_INCLUDE_PATH = ${TOOLCHAIN}/mips-linux-uclibc/lib/gcc/mips-linux-uclibc/6.5.0/include
# TOOLCHAIN_LIB_PATH = ${TOOLCHAIN}/mips-linux-uclibc/lib

# SDK_INCLUDE_DIR = ${SDK_DIR}/include
# SDK_LIB_DIR = ${SDK_DIR}/lib

JANSSON_DIR			= /usr/local
JANSSON_INC_DIR 	= $(JANSSON_DIR)/include

INCLUDE_CFLAGS	= -I.
INCLUDE_CFLAGS	+= -I$(JANSSON_INC_DIR)
# INCLUDE_CFLAGS	+= -I$(SDK_INCLUDE_DIR)

MY_CFLAGS 		:= -O2 -Wall -Wno-unused-variable

CFLAGS			+= $(MY_CFLAGS) $(INCLUDE_CFLAGS)


MY_LDFLAGS		= -lpthread -lm
MY_LDFLAGS		+= -ljansson -L$(JANSSON_DIR)/lib
LDFLAGS			+= $(MY_LDFLAGS)


%.o:%.c
	@(echo "")
	@(echo ">>>>> [C] compiling $< ...")
	@$(CC) -MM -MF $(@:.o=.d) $(CFLAGS) $<
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@echo ""
	@echo ">>>>> [CPP] compiling $< ..."
	@$(CC) -MM -MF $(@:.o=.d) -std=c++98 $(CFLAGS) $<
	@$(CC) -std=c++98 $(CFLAGS) -c $< -o $@


C_SRCS			= mobile_comm.c  mc_common.c  wd360_svc_process.c  json_common.c
C_OBJS			= $(C_SRCS:.c=.o)

EXEC_MOBILE_COMM	= mobile_comm

OBJS_ALL		= $(C_OBJS)
EXEC_ALL 		= $(EXEC_MOBILE_COMM)
DEPS_ALL		= $(OBJS_ALL:.o=.d)


all: $(EXEC_ALL)

$(EXEC_MOBILE_COMM): $(OBJS_ALL)
	@(echo "")
	@(echo ">>>>> Building $@ ...")
	$(CXX) -o $@ $^ $(LDFLAGS)
	@(echo "..... Succeeded!!")
	@(echo "")



# install:
# 	./install_falldt.sh

clean:
	@rm -f $(EXEC_ALL)
	@rm -f $(OBJS_ALL) $(DEPS_ALL)
	# ./install_falldt.sh --clean

.PHONY: clean all install $(EXEC_ALL)

sinclude $(DEPS_ALL)
