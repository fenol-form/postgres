#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile the TPDE JIT provider, building it into a shared library.
#
# Note that this file is recursed into from src/Makefile, not by the
# parent directory..
#
# IDENTIFICATION
#    src/backend/jit/tpde/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/jit/tpde
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

ifneq ($(with_tpde), yes)
    $(error "not building with LLVM support")
endif

PGFILEDESC = "tpdejit - JIT using TPDE"
NAME = tpdejit

ifeq ($(GCC), yes)
LLVM_CFLAGS += -Wno-deprecated-declarations
endif

CFLAGS += $(LLVM_CFLAGS)
CXXFLAGS += $(LLVM_CXXFLAGS)
override CPPFLAGS += $(LLVM_CPPFLAGS)

# Because this module includes C++ files, we need to use a C++
# compiler for linking. Makefile.shlib uses $(COMPILER) to build
# loadable modules.
override COMPILER = $(CXX) $(CXXFLAGS)

# Infrastructure
OBJS += \
	dummy.o \
# 	llvmjit_error.o \
# 	llvmjit_inline.o \
# 	llvmjit_wrap.o \
# 	SectionMemoryManager.o

# Code generation
# OBJS += \
# 	llvmjit_deform.o \
# 	llvmjit_expr.o

all: all-shared-lib # llvmjit_types.bc

install: all installdirs install-lib # install-types

installdirs: installdirs-lib

uninstall: uninstall-lib uninstall-types

include $(top_srcdir)/src/Makefile.shlib

clean distclean: clean-lib
	rm -f $(OBJS)
