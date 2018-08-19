
# ---------------------------------------------------------------------
# Copyright (c) William Fonkou Tambe
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http:#www.gnu.org/licenses/>.
# ---------------------------------------------------------------------


CC ?= gcc

# The options -fdata-sections -ffunction-sections -Wl,--gc-sections
# are used to respectively keep data and functions in separate sections
# which then allow --gc-sections to remove unused sections.
# The option -Wl,-s strip debug information and cannot be used with -g.
CFLAGS = -Werror -Wno-unused-result -g
#CFLAGS = -Werror -Wno-unused-result -fdata-sections -ffunction-sections -Wl,--gc-sections -g
#CFLAGS = -Werror -Wno-unused-result -fdata-sections -ffunction-sections -Wl,--gc-sections -Os -Wl,-s

# Macros enabling features
# in the library mm.
# -rdynamic is needed by backtrace support from execinfo.h
#CFLAGS += -DMMCHECKSIGNATURE
#CFLAGS += -rdynamic -DMMDEBUG
#CFLAGS += -DMMTHREADSAFE

# Macros enabling features
# in the library string.
#CFLAGS += -DSTRINGNOREFCNT

# Macros enabling features
# in the lyrical source code.
#CFLAGS += -DLYRICALUSEINOTIFY

LIBS = lyrical lyricalbackendtext \
	file byt mm pamsyn string parsearg \
	arrayu8 arrayuint bintree mutex

.PHONY: default install uninstall clean package

default:
	@echo targets: x86 x86linux x86cygwin x64 x64linux x64cygwin

x86: src/lyrical.c
	for i in ${LIBS} arrayu32 lyricaldbg32 lyricalbackendx86; do I="$${I} src/lib/$${i}/$${i}.c"; done; \
	${CC} -m32 -DLYRICALX86 ${CFLAGS} -I src/lib/inc -o lyrical src/lyrical.c $${I};
	ls -lh lyrical

x86linux: src/lyrical.c
	for i in ${LIBS} arrayu32 lyricaldbg32 lyricalbackendx86; do I="$${I} src/lib/$${i}/$${i}.c"; done; \
	${CC} -m32 -DLYRICALX86LINUX ${CFLAGS} -I src/lib/inc -o lyrical src/lyrical.c $${I};
	ls -lh lyrical

x86cygwin: src/lyrical.c
	for i in ${LIBS} arrayu32 lyricaldbg32 lyricalbackendx86; do I="$${I} src/lib/$${i}/$${i}.c"; done; \
	${CC} -m32 -DLYRICALX86CYGWIN ${CFLAGS} -I src/lib/inc -o lyrical src/lyrical.c $${I};
	ls -lh lyrical

x64: src/lyrical.c
	for i in ${LIBS} arrayu64 lyricaldbg64 lyricalbackendx64; do I="$${I} src/lib/$${i}/$${i}.c"; done; \
	${CC} -m64 -DLYRICALX64 ${CFLAGS} -I src/lib/inc -o lyrical src/lyrical.c $${I};
	ls -lh lyrical

x64linux: src/lyrical.c
	for i in ${LIBS} arrayu64 lyricaldbg64 lyricalbackendx64; do I="$${I} src/lib/$${i}/$${i}.c"; done; \
	${CC} -m64 -DLYRICALX64LINUX ${CFLAGS} -I src/lib/inc -o lyrical src/lyrical.c $${I};
	ls -lh lyrical

x64cygwin: src/lyrical.c
	for i in ${LIBS} arrayu64 lyricaldbg64 lyricalbackendx64; do I="$${I} src/lib/$${i}/$${i}.c"; done; \
	${CC} -m64 -DLYRICALX64CYGWIN ${CFLAGS} -I src/lib/inc -o lyrical src/lyrical.c $${I};
	ls -lh lyrical

install:
	if [ ! -e /lib/lyrical ]; then ln -snf "$$(pwd)/lib" /lib/lyrical; fi
	if [ ! -e /bin/lyrical ]; then ln -snf "$$(pwd)/lyrical" /bin/lyrical; fi
	mkdir -p /var/lyrical
	chmod ugo+rwxt /var/lyrical

uninstall:
	if [ "$$(readlink /bin/lyrical)" = "$$(pwd)/lyrical" ]; then rm -rf /bin/lyrical; fi
	if [ "$$(readlink /lib/lyrical)" = "$$(pwd)/lib" ]; then rm -rf /lib/lyrical; fi

clean:
	rm -f lyrical

package:
	PKGNAME=$$(basename "$$(pwd)"); cd .. && tar -caf $${PKGNAME}.tar.xz -h $${PKGNAME}
