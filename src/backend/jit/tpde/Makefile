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

ifeq ($(PORTNAME), win32)
    $(error "TPDE JIT is not supported on Windows")
endif

ifeq ($(PORTNAME), cygwin)
    $(error "TPDE JIT is not supported on Cygwin")
endif

subdir = src/backend/jit/tpde
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

ifneq ($(with_tpde), yes)
    $(error "not building with LLVM support")
endif

PGFILEDESC = "tpdejit - JIT using TPDE"
NAME = tpdejit
SHLIB_NAME = tpdejit
MODULE_big = tpdejit

ifeq ($(GCC), yes)
LLVM_CFLAGS += -Wno-deprecated-declarations
endif

CFLAGS += $(LLVM_CFLAGS)
CXXFLAGS += $(LLVM_CXXFLAGS)
CXXFLAGS += -Wl,--gc-sections
override CPPFLAGS += $(LLVM_CPPFLAGS)
CPPFLAGS += -Wl,--gc-sections
CFLAGS += -Wl,--gc-sections

SHLIB_LINK += $(LLVM_LIBS)

# Because this module includes C++ files, we need to use a C++
# compiler for linking. Makefile.shlib uses $(COMPILER) to build
# loadable modules.
override COMPILER = $(CXX) $(CXXFLAGS)

# tpde specific
OBJS += \
	tpde_expr.o \
	llvm_ir_gen.o \
	tpdejit.o \
	install/lib/libtpde.a \
	install/lib/libtpde_llvm.a \
	install/lib/libfadec.a \
	install/lib/libdisarm64.a \
	install/lib/libspdlogd.a

install/lib/libtpde.a: tpde-lib
install/lib/libtpde_llvm.a: tpde-lib
install/lib/libfadec.a: tpde-lib
install/lib/libdisarm64.a: tpde-lib
install/lib/libspdlogd.a: tpde-lib

# Copied from jit/llvm
OBJS += \
	llvmjit_error.o \
	llvmjit_inline.o \
	llvmjit_wrap.o \
	llvmjit_deform.o

tpde-lib: tpde-cmake
	$(MAKE) tpde tpde_llvm && $(MAKE) install

CMAKE = cmake
CMAKE_FLAGS = -D CMAKE_CXX_COMPILER=clang++ \
	-D CMAKE_C_COMPILER=clang \
	-D TPDE_INCLUDE_TESTS=OFF \
	-D CMAKE_BUILD_TYPE=Debug \
	-D CMAKE_INSTALL_PREFIX=$(top_builddir)/src/backend/jit/tpde/install

tpde-cmake:
	$(CMAKE) -S $(top_srcdir)/src/backend/jit/tpde -B $(top_builddir)/src/backend/jit/tpde $(CMAKE_FLAGS)

all: tpde-lib all-shared-lib llvmjit_types.bc 

install: tpde-lib all installdirs install-lib install-lib-shared install-types

installdirs: tpde-lib installdirs-lib

uninstall: uninstall-lib uninstall-types

install-types: llvmjit_types.bc
	$(INSTALL_DATA) llvmjit_types.bc '$(DESTDIR)$(pkglibdir)'

uninstall-types:
	rm -f '$(DESTDIR)$(pkglibdir)/llvmjit_types.bc'

include $(top_srcdir)/src/Makefile.shlib

clean distclean: clean-lib
	rm -f $(OBJS)
	rm -f llvmjit_types.bc
