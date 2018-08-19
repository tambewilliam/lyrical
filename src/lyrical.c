
// ---------------------------------------------------------------------
// Copyright (c) William Fonkou Tambe
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------


// Path to the folder containing
// the cached compiled binary.
// Must be terminated with '/'.
#if defined(LYRICALX86)
#define LYRICALCACHEDIR "/var/lyrical/x86/"
#elif defined(LYRICALX86LINUX)
#define LYRICALCACHEDIR "/var/lyrical/x86linux/"
#elif defined(LYRICALX86CYGWIN)
#define LYRICALCACHEDIR "/var/lyrical/x86cygwin/"
#elif defined(LYRICALX64)
#define LYRICALCACHEDIR "/var/lyrical/x64/"
#elif defined(LYRICALX64LINUX)
#define LYRICALCACHEDIR "/var/lyrical/x64linux/"
#elif defined(LYRICALX64CYGWIN)
#define LYRICALCACHEDIR "/var/lyrical/x64cygwin/"
#else
#error "unsupported"
#endif

// Path to the folder containing
// the standard library.
// Must be terminated with '/'.
#define LYRICALLIBDIR "/lib/lyrical/"

// When defined, the files that were
// included in the compilation are
// monitored for modification;
// and a restart is forced if any
// of those files is modified.
//#define LYRICALUSEINOTIFY

#define _GNU_SOURCE
#define __USE_GNU

// Used for exit(), close(),
// getpid() and execve().
#include <unistd.h>

// Used for signal.
#include <signal.h>

// Used for context manipulations.
#include <ucontext.h>

// Used for chmod(), open().
#include <sys/stat.h>

// Used for open(), getpid(), kill().
#include <sys/types.h>
#include <fcntl.h>

// Used for mmap(), mprotect().
#include <sys/mman.h>
#ifndef MAP_STACK
#define MAP_STACK 0x20000
#endif
#ifndef MAP_UNINITIALIZED
#define MAP_UNINITIALIZED 0x4000000
#endif

#ifdef LYRICALUSEINOTIFY
// Used for inotify.
#include <sys/inotify.h>
#endif

// Used for clone().
#include <sched.h>

// Used for tty functions.
#include <termios.h>

// Not all platforms support execinfo.h .
#if defined(__linux__)
// Used for backtrace.
#include <execinfo.h>
#endif

// Used for printf().
#include <stdio.h>

// Used for system(), free() (needed to free
// the memory returned by backtrace_symbols()).
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Not all platforms support sendfile().
#if defined(__linux__)
#include <sys/sendfile.h>
#endif

#include <errno.h>

#include <stdtypes.h>
#include <macros.h>
#include <byt.h>
#include <mm.h>
#include <string.h>
#include <file.h>
#include <parsearg.h>
#include <lyrical.h>
#include <lyricalbackendtext.h>
#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
#include <lyricalbackendx86.h>
#include <lyricaldbg32.h>
#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
#include <lyricalbackendx64.h>
#include <lyricaldbg64.h>
#endif


u8* helpstring = "Usage: lyrical sourcefile <ARGS>\n";

void main (uint argc, u8** arg, u8** env) {
	// Catchable-labels.
	__label__ labelforinotifysetup, labelforskippingexec;
	
	// Variable which will hold
	// the default exit status.
	uint exitstatus = 0; // The shell use 0 for success.
	
	#ifdef LYRICALUSEINOTIFY
	// Save the current list of arguments.
	// ### The use of inotify is to be disabled
	// ### if the array of pointers pointed by
	// ### arg is modified one way or another,
	// ### since that array must be used
	// ### unmodified when calling execvp()
	// ### to do a restart.
	u8** savedarg = arg;
	#endif
	
	struct {
		// When set, a file "log" which
		// contain the result of the compilation
		// in its human readable textual form
		// is also generated.
		uint log;
		
		// When set, a file "dbg" which
		// contain debug information from
		// the compilation is also generated.
		uint dbg;
		
		// Used when option "tcpipv4addr" is used.
		struct {
			uint en;
			struct sockaddr_in addr;
		} tcpipv4svr;
		
	} options;
	
	bytsetz((void*)&options, sizeof(options));
	
	u8* parseargoptionswithvalue[] = {
		"tcpipv4addr",
		0
	};
	
	void parseargcallback (u8* option, u8* value) {
		// I decrease argc since this argument
		// will be removed from the array
		// of arguments pointed by arg.
		--argc;
		
		if (option[0] == 'l' && !option[1]) options.log = 1;
		else if (option[0] == 'g' && !option[1]) options.dbg = 1;
		else if (stringiseq3(option, "tcpipv4addr")) {
			
			options.tcpipv4svr.en = 1;
			
			u8* s = value;
			u8 c;
			// Parse the port number if any.
			while ((c = *s)) {
				if (c == ':') {
					*s = 0;
					break;
				} else ++s;
			}
			
			if (*value) {
				
				if (!inet_aton(value, &options.tcpipv4svr.addr.sin_addr)) {
					fprintf(stderr, "lyrical: invalid ipv4 address\n");
					exit(-1); // The shell use non-null for failing.
				}
				
			} else options.tcpipv4svr.addr.sin_addr.s_addr = htonl(INADDR_ANY);
			
			if (c) {
				options.tcpipv4svr.addr.sin_port = htons(atoi(s+1));
				*s = ':'; //Restore the character that was set null.
			} else options.tcpipv4svr.addr.sin_port = htons(8080);
			
			options.tcpipv4svr.addr.sin_family = AF_INET;
			
		} else {
			fprintf(stderr, "lyrical: invalid option \"%s\"\n", option);
			exit(-1); // The shell use non-null for failing.
		}
	}
	
	// Move to the address of the first argument,
	// which must be the file to compile.
	++arg;
	
	// Parse the command line for any option given.
	arg = parsearglong2(arg, parseargcallback, parseargoptionswithvalue);
	
	// Test if there was
	// any argument given.
	if (!*arg) {
		fprintf(stderr, "%s", helpstring);
		exit(-1); // The shell use non-null for failing.
	}
	
	if (fileis2(*arg) == FILEISNOTEXIST) {
		fprintf(stderr, "input file not found\n");
		exit(-1); // The shell use non-null for failing.
	}
	
	// Since this is going to run
	// on Linux, each user get its
	// own folder under LYRICALCACHEDIR.
	// The string allocation is done
	// here so that it be outside of
	// the memory session memsession.
	static string cachepath;
	cachepath = stringfmt("%s%d", LYRICALCACHEDIR, getuid());
	
	// Path to the file "dbg" within
	// the folder used for caching.
	// A null string is allocated
	// to cachepathdbg so that its
	// memory allocation be outside
	// of the memory session memsession,
	// and useable by pagefaulthandler().
	static string cachepathdbg;
	if (options.dbg) {
		cachepathdbg = stringduplicate2("");
	}
	
	mmsession memsession = mmsessionnew();
	
	lyricalpredeclaredmacro globalmacros[4] = {
		#if defined(LYRICALX86)
		{
			.name = "LYRICALX86",
			.content = "",
		},
		{
			.name = "sint",
			.content = "s32",
		},
		{
			.name = "uint",
			.content = "u32",
		},
		#elif defined(LYRICALX86LINUX)
		{
			.name = "LYRICALX86LINUX",
			.content = "",
		},
		{
			.name = "sint",
			.content = "s32",
		},
		{
			.name = "uint",
			.content = "u32",
		},
		#elif defined(LYRICALX86CYGWIN)
		{
			.name = "LYRICALX86CYGWIN",
			.content = "",
		},
		{
			.name = "sint",
			.content = "s32",
		},
		{
			.name = "uint",
			.content = "u32",
		},
		#elif defined(LYRICALX64)
		{
			.name = "LYRICALX64",
			.content = "",
		},
		{
			.name = "sint",
			.content = "s64",
		},
		{
			.name = "uint",
			.content = "u64",
		},
		#elif defined(LYRICALX64LINUX)
		{
			.name = "LYRICALX64LINUX",
			.content = "",
		},
		{
			.name = "sint",
			.content = "s64",
		},
		{
			.name = "uint",
			.content = "u64",
		},
		#elif defined(LYRICALX64CYGWIN)
		{
			.name = "LYRICALX64CYGWIN",
			.content = "",
		},
		{
			.name = "sint",
			.content = "s64",
		},
		{
			.name = "uint",
			.content = "u64",
		},
		#endif
		{
			.name = 0,
		},
	};
	
	// When set, the first element
	// of the array stdpath is used
	// by installmissingmodule().
	u8* stdpath[] = {
		LYRICALLIBDIR,
		LYRICALLIBDIR"_", // Expected by LYRICALLIBDIR/.install invoked by installmissingmodule().
		(u8*)0,
	};
	
	uint installmissingmodule (u8* modulename) {
		if (stdpath[0]) {
			string s = stringfmt("exec %s.install '%s' 1>&2", stdpath[0], modulename);
			system(s.ptr);
			mmfree(s.ptr);
			return 1;
		}
		return 0;
	}
	
	void errorcallbackfunction (u8* msg) {
		fprintf(stderr, "%s\n", msg);
	}
	
	if (**arg != '/') {
		// I get the absolute path
		// to the file to compile.
		string s = filegetcwd();
		stringappend4(&s, '/');
		stringappend2(&s, *arg);
		stringappend1(&cachepath, s);
		mmfree(s.ptr);
		
	} else stringappend2(&cachepath, *arg);
	
	// Path to the file "log" within
	// the folder used for caching.
	string cachepathlog;
	
	if (options.log) {
		cachepathlog = stringduplicate1(cachepath);
		stringappend2(&cachepathlog, "/log");
	} else cachepathlog = stringnull;
	
	// Path to the file "bin" within
	// the folder used for caching.
	string cachepathbin = stringduplicate1(cachepath);
	stringappend2(&cachepathbin, "/bin");
	
	// Path to the file "src" within
	// the folder used for caching.
	string cachepathsrc = stringduplicate1(cachepath);
	stringappend2(&cachepathsrc, "/src");
	
	// Path to the file "map" within
	// the folder used for caching.
	string cachepathbinmap = stringduplicate1(cachepath);
	stringappend2(&cachepathbinmap, "/map");
	
	if (options.dbg) {
		stringappend1(&cachepathdbg, cachepath);
		stringappend2(&cachepathdbg, "/dbg");
	}
	
	// Function which assess whether
	// compilation is needed.
	uint iscompileneeded () {
		
		if (fileis1(cachepath.ptr) == FILEISFOLDER) {
			// If I get here, the folder
			// for the cached compiled binary
			// exist; I proceed to checking
			// whether its source files were
			// modified since it was last
			// compiled.
			
			// I get the timestamp of the executable.
			uint cachepathbintimestamp = filetimestamp(cachepathbin.ptr);
			
			if (!cachepathbintimestamp) goto deletecachepath;
			
			// I load the file "src" which contain
			// the list of source files which
			// were used to build the executable.
			arrayu8 cachepathsrcloaded = fileread1(cachepathsrc.ptr);
			
			if (!cachepathsrcloaded.ptr) {
				
				mmfree(cachepathsrcloaded.ptr);
				
				goto deletecachepath;
			}
			
			// Check the timestamp of each
			// of the files mentioned in "src".
			
			u8* srcfilepath;
			
			u8* ptr = cachepathsrcloaded.ptr;
			
			while (*ptr) {
				
				srcfilepath = ptr;
				
				do ++ptr; while (*ptr && *ptr != '\n');
				
				if (*ptr) {
					*ptr = 0;
					++ptr;
				}
				
				uint srcfiletimestamp = filetimestamp(srcfilepath);
				
				if (!srcfiletimestamp || srcfiletimestamp > cachepathbintimestamp) {
					
					mmfree(cachepathsrcloaded.ptr);
					
					goto deletecachepath;
				}
			}
			
			mmfree(cachepathsrcloaded.ptr);
			
			// I return 0 to signal that
			// compilation is not needed.
			return 0;
			
		} else goto createcachepath;
		
		deletecachepath:
		
		if (!fileremoverecursive(cachepath.ptr)) {
			fprintf(stderr, "failed to delete %s\n", cachepath.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		createcachepath:
		
		// If I get here, the folder
		// for the cached compiled binary
		// do not exist; I create it.
		
		if (!filemkdirparents(cachepath.ptr)) {
			fprintf(stderr, "failed to create %s\n", cachepath.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// I return 1 to signal that
		// compilation is needed.
		return 1;
	}
	
	// Structure used to describe
	// the data stored to "map".
	#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
	typedef struct {
		u32 executableinstrsz;
		u32 constantstringssz;
		u32 globalvarregionsz;
	} binmap;
	#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
	typedef struct {
		u64 executableinstrsz;
		u64 constantstringssz;
		u64 globalvarregionsz;
	} binmap;
	#endif
	
	// Variable which will be set
	// with the address to jump to
	// in order to start execution.
	void* execpages = 0;
	
	#ifdef LYRICALUSEINOTIFY
	// Variable which will be set to
	// the string containing the list
	// of files that were compiled
	// to generate the executable.
	string execsrc = stringnull;
	#endif
	
	binmap execmap;
	
	// I check whether the file
	// need to be compiled.
	if (iscompileneeded()) {
		
		lyricalcompilearg compilearg = {
			.source = stringfmt(
				((*arg)[0] == '/' ||
					((*arg)[0] == '.' && ((*arg)[1] == '/' ||
						((*arg)[1] == '.' && (*arg)[2] == '/')))) ?
				"%s`include \"%s\"" : "%s`include \"./%s\"",
				(options.tcpipv4svr.en) ? "`include \"stdsck\"\n" : "",
				*arg).ptr,
			#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
			.sizeofgpr = sizeof(u32),
			.nbrofgpr = 7,
			// LYRICALOPSTACKPAGEALLOC may need
			// up to 64*sizeof(u32) == 256 bytes
			// of additional stack space.
			.stackpageallocprovision = 64*sizeof(u32),
			// LYRICALOPAFIP need a space of sizeofgpr
			// in the stack to compute a relative address.
			// When the stack of a function being called
			// is created above the stack pointed by
			// the stack pointer register, it is possible
			// for the data at the bottom of the stack
			// being created to be corrupted by LYRICALOPAFIP.
			// Indeed, LYRICALOPAFIP use the instruction CALL
			// to compute an address from the instruction
			// pointer register address; using the instruction
			// CALL push the instruction pointer register address
			// in the stack, and in doing so, corrupt data
			// at the bottom of the stack being created
			// if a guard space is not used.
			// For similar reason as above, the guard space
			// would be needed by LYRICALOPPAGEALLOC.
			.functioncallargsguardspace = 64,
			.jumpcaseclog2sz = 3, // (1<<3) == 8 bytes should be enough.
			#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
			.sizeofgpr = sizeof(u64),
			.nbrofgpr = 15,
			// LYRICALOPSTACKPAGEALLOC may need
			// up to 64*sizeof(u64) == 512 bytes
			// of additional stack space.
			.stackpageallocprovision = 64*sizeof(u64),
			// LYRICALOPAFIP need a space of sizeofgpr
			// in the stack to compute a relative address.
			// When the stack of a function being called
			// is created above the stack pointed by
			// the stack pointer register, it is possible
			// for the data at the bottom of the stack
			// being created to be corrupted by LYRICALOPAFIP.
			// Indeed, LYRICALOPAFIP use the instruction CALL
			// to compute an address from the instruction
			// pointer register address; using the instruction
			// CALL push the instruction pointer register address
			// in the stack, and in doing so, corrupt data
			// at the bottom of the stack being created
			// if a guard space is not used.
			// For similar reason as above, the guard space
			// would be needed by LYRICALOPPAGEALLOC.
			.functioncallargsguardspace = 64,
			.jumpcaseclog2sz = 4, // (1<<4) == 16 bytes should be enough.
			#endif
			.predeclaredvars = 0,
			.predeclaredmacros = globalmacros,
			.standardpaths = stdpath,
			.installmissingmodule = installmissingmodule,
			.lyxappend = ".stdsckout();",
			.error = errorcallbackfunction,
			.compileflag = LYRICALCOMPILENOFUNCTIONIMPORT | LYRICALCOMPILENOFUNCTIONEXPORT
				| (options.dbg ? LYRICALCOMPILEGENERATEDEBUGINFO : 0)
				| (options.log ? LYRICALCOMPILECOMMENT : 0)
		};
		
		bytsetz((void*)&compilearg.minunusedregcountforop, sizeof(compilearg.minunusedregcountforop));
		// Set the corresponding lyricalop minimum count
		// of unused registers as needed by the backend.
		#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
		#include "x86.minunusedregcountforop.lyrical.c"
		#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
		#include "x64.minunusedregcountforop.lyrical.c"
		#endif
		
		lyricalcompileresult compileresult = lyricalcompile(&compilearg);
		
		mmfree(compilearg.source);
		
		if (!compileresult.rootfunc) {
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// I generate the file "src".
		if (!filewritetruncate1(cachepathsrc.ptr, compileresult.srcfilepaths.ptr,
			stringmmsz(compileresult.srcfilepaths))) {
			fprintf(stderr, "failure to write to %s\n", cachepathsrc.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		#ifdef LYRICALUSEINOTIFY
		execsrc = stringduplicate1(compileresult.srcfilepaths);
		#endif
		
		// I generate the executable binary.
		#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
		lyricalbackendx86result* binresult = lyricalbackendx86(compileresult, LYRICALBACKENDX86PAGEALIGNED);
		#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
		lyricalbackendx64result* binresult = lyricalbackendx64(compileresult, LYRICALBACKENDX64PAGEALIGNED);
		#endif
		
		if (!binresult->execbin.ptr) {
			fprintf(stderr, "failure to generate binary\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		if (binresult->importinfo.ptr) {
			fprintf(stderr, "import support not yet implemented\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		if (binresult->exportinfo.ptr) {
			fprintf(stderr, "export support not yet implemented\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// I store the executable binary to "bin".
		if (!filewritetruncate1(cachepathbin.ptr, binresult->execbin.ptr, arrayu8sz(binresult->execbin))) {
			fprintf(stderr, "failure to write to %s\n", cachepathbin.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		if (options.dbg) {
			// I store the debug information to "dbg".
			if (!filewritetruncate1(cachepathdbg.ptr, binresult->dbginfo.ptr, arrayu8sz(binresult->dbginfo))) {
				fprintf(stderr, "failure to write to %s\n", cachepathdbg.ptr);
				exitstatus = -1; // The shell use non-null for failing.
				goto labelforinotifysetup;
			}
		}
		
		if (options.log) {
			// I generate the compilation log.
			string textresult = lyricalbackendtext(compileresult);
			
			// I store the compilation log to "log".
			if (!filewritetruncate1(cachepathlog.ptr, textresult.ptr, stringmmsz(textresult))) {
				fprintf(stderr, "failure to write to %s\n", cachepathlog.ptr);
				exitstatus = -1; // The shell use non-null for failing.
				goto labelforinotifysetup;
			}
			
			mmfree(textresult.ptr);
		}
		
		execmap.executableinstrsz = binresult->executableinstrsz;
		execmap.constantstringssz = binresult->constantstringssz;
		execmap.globalvarregionsz = binresult->globalvarregionsz;
		
		// I store the executable binary map to "map".
		if (!filewritetruncate1(cachepathbinmap.ptr, &execmap, sizeof(binmap))) {
			fprintf(stderr, "failure to write to %s\n", cachepathbinmap.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// Align execmap values to a pagesize.
		execmap.executableinstrsz = ROUNDUPTOPOWEROFTWO(execmap.executableinstrsz, 0x1000);
		execmap.constantstringssz = ROUNDUPTOPOWEROFTWO(execmap.constantstringssz, 0x1000);
		execmap.globalvarregionsz = ROUNDUPTOPOWEROFTWO(execmap.globalvarregionsz, 0x1000);
		
		#if defined(__linux__)
		// Allocate the pages
		// to which a jump will be
		// done to start execution.
		execpages = mmap(
			0,
			execmap.executableinstrsz + execmap.constantstringssz + execmap.globalvarregionsz,
			PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED,
			0, 0);
		
		if (execpages == (void*)-1) {
			fprintf(stderr, "mmap() failed\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// Copy executable instructions.
		bytcpy(
			execpages,
			binresult->execbin.ptr,
			binresult->executableinstrsz);
		
		// Copy constant strings.
		bytcpy(
			execpages + execmap.executableinstrsz,
			binresult->execbin.ptr + execmap.executableinstrsz,
			binresult->constantstringssz);
		
		// Set page permissions.
		
		if (mprotect(
				execpages,
				execmap.executableinstrsz,
				PROT_EXEC) != 0) {
			
			fprintf(stderr, "failure to set --x memory protection\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		if (mprotect(
				execpages + execmap.executableinstrsz,
				execmap.constantstringssz,
				PROT_READ) != 0) {
			
			fprintf(stderr, "failure to set r-- memory protection\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		if (mprotect(
				execpages + execmap.executableinstrsz + execmap.constantstringssz,
				execmap.globalvarregionsz,
				PROT_READ|PROT_WRITE) != 0) {
			
			fprintf(stderr, "failure to set rw- memory protection\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		#else
		// Allocate the pages
		// to which a jump will be
		// done to start execution.
		execpages = mmap(
			0,
			execmap.executableinstrsz + execmap.constantstringssz + execmap.globalvarregionsz,
			PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED,
			0, 0);
		
		if (execpages == (void*)-1) {
			fprintf(stderr, "mmap() failed\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// Copy executable instructions.
		bytcpy(
			execpages,
			binresult->execbin.ptr,
			binresult->executableinstrsz);
		
		// Copy constant strings.
		bytcpy(
			execpages + execmap.executableinstrsz,
			binresult->execbin.ptr + execmap.executableinstrsz,
			binresult->constantstringssz);
		#endif
		
		#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
		lyricalbackendx86resultfree(binresult);
		#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
		lyricalbackendx64resultfree(binresult);
		#endif
		
		lyricalfree(compileresult);
		
	} else {
		// If I get here, compilation is not needed.
		
		uint fid;
		
		if ((fid = open(cachepathbinmap.ptr, O_RDONLY)) == -1) {
			fprintf(stderr, "failure to open %s\n", cachepathbinmap.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		if (read(fid, &execmap, sizeof(binmap)) != sizeof(binmap)) {
			fprintf(stderr, "failure to read %s\n", cachepathbinmap.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		if (close(fid) == -1) {
			fprintf(stderr, "failure to close %s\n", cachepathbinmap.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// Align execmap values to a pagesize.
		execmap.executableinstrsz = ROUNDUPTOPOWEROFTWO(execmap.executableinstrsz, 0x1000);
		execmap.constantstringssz = ROUNDUPTOPOWEROFTWO(execmap.constantstringssz, 0x1000);
		execmap.globalvarregionsz = ROUNDUPTOPOWEROFTWO(execmap.globalvarregionsz, 0x1000);
		
		#ifdef LYRICALUSEINOTIFY
		execsrc.ptr = fileread1(cachepathsrc.ptr).ptr;
		#endif
		
		if ((fid = open(cachepathbin.ptr, O_RDONLY)) == -1) {
			fprintf(stderr, "failure to open %s\n", cachepathbin.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		// The file will be left opened,
		// since mmap() use it below so that
		// only pages of it that are accessed
		// get loaded in memory, or so to share
		// pages of it that has already been
		// loaded in memory for another process.
		
		#if defined(__linux__)
		execpages = mmap(
			0,
			// execmap.globalvarregionsz is included
			// in the mapped region size so that
			// the space for the global variable region
			// get reserved, since it is not part
			// of the opened file "bin".
			execmap.executableinstrsz + execmap.constantstringssz + execmap.globalvarregionsz,
			PROT_READ,
			MAP_PRIVATE,
			fid, 0);
		
		if (execpages == (void*)-1) {
			fprintf(stderr, "mmap() failed\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// Allocate pages for global variables.
		if (execmap.globalvarregionsz && mmap(
			(void*)(execpages + execmap.executableinstrsz + execmap.constantstringssz),
			execmap.globalvarregionsz,
			PROT_READ|PROT_WRITE,
			MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED,
			0, 0) == (void*)-1) {
			fprintf(stderr, "failure to allocate pages for global variables\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		// Set page permissions.
		
		if (mprotect(
			execpages,
			execmap.executableinstrsz,
			PROT_EXEC) != 0) {
			fprintf(stderr, "failure to set --x memory protection\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		/*if (mprotect(
			execpages + execmap.executableinstrsz,
			execmap.constantstringssz,
			PROT_READ) != 0) {
			fprintf(stderr, "failure to set r-- memory protection\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		if (mprotect(
			execpages + execmap.executableinstrsz + execmap.constantstringssz,
			execmap.globalvarregionsz,
			PROT_READ|PROT_WRITE) != 0) {
			fprintf(stderr, "failure to set rw- memory protection\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}*/
		#else
		// Allocate the pages
		// to which a jump will be
		// done to start execution.
		execpages = mmap(
			0,
			execmap.executableinstrsz + execmap.constantstringssz + execmap.globalvarregionsz,
			PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED,
			0, 0);
		
		if (execpages == (void*)-1) {
			fprintf(stderr, "mmap() failed\n");
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		
		uint sztoread = execmap.executableinstrsz + execmap.constantstringssz;
		
		if (read(fid, execpages, sztoread) != sztoread) {
			fprintf(stderr, "failure to read %s\n", cachepathbin.ptr);
			exitstatus = -1; // The shell use non-null for failing.
			goto labelforinotifysetup;
		}
		#endif
	}
	
	mmfree(cachepathbinmap.ptr);
	mmfree(cachepathbin.ptr);
	mmfree(cachepathsrc.ptr);
	if (cachepathlog.ptr) mmfree(cachepathlog.ptr);
	// Note that cachepathdbg is not freed because
	// it is used at runtime by pagefaulthandler(). 
	
	#ifndef LYRICALUSEINOTIFY
	labelforinotifysetup:;
	#else
	
	mmsessionextract(memsession, execsrc.ptr);
	
	labelforinotifysetup:;
	
	void* inotifythreadstack;
	
	if ((inotifythreadstack = mmap(
		(void*)0,
		0x1000,
		PROT_READ|PROT_WRITE,
		MAP_PRIVATE|MAP_ANONYMOUS|MAP_UNINITIALIZED|MAP_STACK,
		0, 0)) == (void*)-1) {
		perror("failure to allocate inotify thread stack");
		exitstatus = -1; // The shell use non-null for failing.
		goto labelforskippingexec;
	}
	
	int inotifythread (void* arg) {
		
		uint inotifyfd = inotify_init();
		
		if (inotifyfd == -1) {
			perror("inotify_init() failed");
			return -1;
		}
		
		if (execsrc.ptr) {
			// If I get here, I parse for each
			// absolute path to source file.
			u8* str = execsrc.ptr;
			
			while (*str) {
				
				u8* strend = str;
				
				u8 endchar;
				
				// Find the end of the file path.
				while ((endchar = *strend) && endchar != '\n')
					++strend;
				
				// Set a null-terminating character
				// if the path was ended with
				// a newline character.
				if (endchar == '\n') *strend = 0;
				
				if (inotify_add_watch(inotifyfd, str, IN_MODIFY) == -1) {
					fprintf(stderr, "inotify_add_watch() failed on %s\n", str);
				}
				
				str = strend+1;
			}
			
		} else {
			
			if (inotify_add_watch(inotifyfd, savedarg[1], IN_MODIFY) == -1) {
				fprintf(stderr, "inotify_add_watch() failed on %s\n", savedarg[1]);
			}
		}
		
		struct inotify_event inotifybuffer;
		
		while (read(inotifyfd, (void*)&inotifybuffer, sizeof(struct inotify_event)) < 1) {
			perror("read(inotifyfd) failed");
		}
		
		if (execvp(savedarg[1], (char**)&savedarg[1]) == -1) {
			perror("failure to restart self");
			return -1;
		}
		
		return 0;
	}
	
	pid_t inotifythreadid = clone(
		inotifythread,
		inotifythreadstack+0x1000,
		CLONE_THREAD|CLONE_SIGHAND|CLONE_VM,
		0);
	
	if (inotifythreadid == -1) {
		perror("failure to create inotify thread");
		exitstatus = -1; // The shell use non-null for failing.
		goto labelforskippingexec;
	}
	
	#endif
	
	if (exitstatus != 0) goto labelforskippingexec;
	
	#ifdef MMDEBUG
	// Callback function used to catch all leaked memory block.
	void callback (void* addr, uint size, uint refcnt, uint changecount, u8* lastchangefilename, uint lastchangelinenumber, void** backtrace) {
		
		fprintf(stderr, "compiler memory leak\n--------------------\n");
		
		if (lastchangefilename) {
			
			fprintf(stderr,
				#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
				"%s:%d:%d:%d:%s:%d\n",
				#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
				"%s:%ld:%ld:%ld:%s:%ld\n",
				#endif
				(u8*)addr, size, refcnt, changecount, lastchangefilename, lastchangelinenumber);
			
			// Not all platforms support execinfo.h .
			#if defined(__linux__)
			
			uint n = 0;
			
			// Determine the number of non-null
			// addresses in the backtrace array.
			while (backtrace[n]) ++n;
			
			// Translate backtrace addresses into strings
			// that describe them symbolically.
			char** s = backtrace_symbols(backtrace, n);
			
			uint i = 0;
			
			do fprintf(stderr, "\t%s\n", s[i]);
			while (++i < n);
			
			free(s);
			
			#endif
			
		} else fprintf(stderr,
			#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
			"%s:%d:%d:%d\n",
			#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
			"%s:%ld:%ld:%ld\n",
			#endif
			(u8*)addr, size, refcnt, changecount);
		
		fprintf(stderr, "--------------------\n");
	}
	mmdebugsession(memsession, MMDOSUBSESSIONS, callback);
	#else
	uint memsessionusage = mmsessionusage(memsession, MMDOSUBSESSIONS);
	if (memsessionusage) fprintf(stderr,
		#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
		"leakage: %d Bytes\n",
		#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
		"leakage: %ld Bytes\n",
		#endif
		memsessionusage);
	#endif
	
	mmsessionfree(memsession, MMDOSUBSESSIONS);
	
	// Execute the binary after
	// setting the arguments, environments
	// and syscall-call-gate pointers.
	
	void** globalvarregion = (void**)(execpages + execmap.executableinstrsz + execmap.constantstringssz);
	
	globalvarregion[0] = (void*)&arg;
	globalvarregion[1] = (void*)&env;
	// globalvarregion[2] get set within
	// the file installing Lyrical syscalls.
	// globalvarregion[3] get set within
	// the file implementing a server.
	
	// Install the Lyrical syscalls.
	#include "syscalls.lyrical.c"
	
	#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
	// Install the pagefault handler.
	#include "x86.pagefaulthandler.lyrical.c"
	#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
	// Install the pagefault handler.
	#include "x64.pagefaulthandler.lyrical.c"
	#endif
	
	void runexecpages() {
		#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
		// Saving EBP because GCC do not save it,
		// and use it to access a stackframe.
		asm volatile ("push %ebp");
		((void(*)(void))execpages)();
		asm volatile ("pop %ebp");
		#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
		// Saving RBP because GCC do not save it,
		// and use it to access a stackframe.
		asm volatile ("push %rbp");
		((void(*)(void))execpages)();
		asm volatile ("pop %rbp");
		#endif
	}
	
	if (options.tcpipv4svr.en) {
		#include "tcpipv4svr.lyrical.c"
	} else runexecpages();
	
	// I jump here in the event of an error.
	labelforskippingexec:;
	
	#ifdef LYRICALUSEINOTIFY
	pause();
	#endif
	
	exit(exitstatus);
}
