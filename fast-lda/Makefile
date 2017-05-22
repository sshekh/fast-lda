# (C) Copyright 2004, David M. Blei (blei [at] cs [dot] cmu [dot] edu)

# This file is part of LDA-C.

# LDA-C is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your
# option) any later version.

# LDA-C is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

CC=icc
PARTIAL=-c
OBJ=-o
OUTNAME=-olda

# CC should be either gcc (default), icc (intel linux) or icl (intel windows)
#_CRT_SECURE_NO_WARNINGS: silence warnings about scanf, fopen and other stuff.
ifeq ($(CC),gcc)
	CFLAGS= -O3 -Wall -g -std=c99 -D_CRT_SECURE_NO_WARNINGS -march=core-avx2
	LDFLAGS= -lm
else
	ifeq ($(CC),icl)
		CFLAGS= /O3 /Wall /Z7 /std=c99 /DWIN32 /D_CRT_SECURE_NO_WARNINGS /arch:CORE-AVX2 /nologo
		LDFLAGS=
		PARTIAL=/c
		OBJ=/Fo
		OUTNAME=/Fe:lda.exe
	else
		CFLAGS= -O3 -Wall -g -std=c99 -DMKL_ILP64 -I${MKLROOT}/include
		LDFLAGS= ${MKLROOT}/lib/libmkl_scalapack_ilp64.a ${MKLROOT}/lib/libmkl_cdft_core.a ${MKLROOT}/lib/libmkl_intel_ilp64.a ${MKLROOT}/lib/libmkl_sequential.a ${MKLROOT}/lib/libmkl_core.a ${MKLROOT}/lib/libmkl_blacs_mpich_ilp64.a -lpthread -lm -ldl
	endif
endif



LOBJECTS= rdtsc-helper.o lda-estimate-helper.o lda-run.o lda-estimate.o lda-model.o lda-inference.o utils.o cokus.o lda-alpha.o fp.o

%.o : %.c
	$(CC) $(CFLAGS) $(XCFLAGS) $(PARTIAL) $< $(OBJ)$@

lda:	$(LOBJECTS)
	$(CC) $(CFLAGS) $(LOBJECTS) $(LDFLAGS) $(OUTNAME)

clean:
	-rm -f *.o *.ilk *.pdb lda.exe lda
