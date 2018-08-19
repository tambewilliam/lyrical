
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


// File which implement printing out
// debug info when a pagefault occur.

// TODO: Print backtrace right after where
// file:line where the pagefault occured was printed.
// And do so only if the fault occurred
// among lyrical executable instructions.
// The executable map will be used to determine that.

void faulthandler (uint signo, siginfo_t* si, ucontext_t* uc) {
	
	fprintf(stderr,
		(signo == SIGSEGV) ?
			((si->si_code == SEGV_MAPERR) ? "\nPageFault(unmapped)@0x%08x\n" :
			 (si->si_code == SEGV_ACCERR) ? "\nPageFault(badperm)@0x%08x\n" :
			 "\nPageFault@0x%08x\n") :
		(signo == SIGFPE)  ? "\nArithmeticFault@0x%08x\n" :
		(signo == SIGILL)  ? "\nOpCodeFault@0x%08x\n" :
		(signo == SIGBUS)  ? "\nBusFault@0x%08x\n" :
		                     "\nFault@0x%08x\n",
		(uint)si->si_addr);
	
	// Compute the offset within
	// the executable binary
	// where the pagefault occur.
	#if defined(__linux__)
	uint binoffset = ((uint)uc->uc_mcontext.gregs[REG_EIP] - (uint)execpages);
	#elif defined(__CYGWIN__)
	uint binoffset = ((uint)uc->uc_mcontext.eip - (uint)execpages);
	#endif
	
	if (options.dbg) {
		
		lyricaldbg32getlineresult dbgline = lyricaldbg32getline(cachepathdbg.ptr, binoffset);
		
		if (dbgline.ltxt) {
			
			fprintf(stderr, "%s:%d\n%s\n", dbgline.path, dbgline.lnum, dbgline.ltxt);
			
			mmfree(dbgline.path);
			mmfree(dbgline.ltxt);
			
		} else fprintf(stderr, "?:?\n");
	}
	
	#if defined(__linux__)
	fprintf(stderr, "EIP 0x%08x\n", uc->uc_mcontext.gregs[REG_EIP]);
	fprintf(stderr, "ESP 0x%08x\n", uc->uc_mcontext.gregs[REG_ESP]);
	fprintf(stderr, "EAX 0x%08x\n", uc->uc_mcontext.gregs[REG_EAX]);
	fprintf(stderr, "EBX 0x%08x\n", uc->uc_mcontext.gregs[REG_EBX]);
	fprintf(stderr, "ECX 0x%08x\n", uc->uc_mcontext.gregs[REG_ECX]);
	fprintf(stderr, "EDX 0x%08x\n", uc->uc_mcontext.gregs[REG_EDX]);
	fprintf(stderr, "EBP 0x%08x\n", uc->uc_mcontext.gregs[REG_EBP]);
	fprintf(stderr, "EDI 0x%08x\n", uc->uc_mcontext.gregs[REG_EDI]);
	fprintf(stderr, "ESI 0x%08x\n", uc->uc_mcontext.gregs[REG_ESI]);
	#elif defined(__CYGWIN__)
	fprintf(stderr, "EIP 0x%08x\n", uc->uc_mcontext.eip);
	fprintf(stderr, "ESP 0x%08x\n", uc->uc_mcontext.esp);
	fprintf(stderr, "EAX 0x%08x\n", uc->uc_mcontext.eax);
	fprintf(stderr, "EBX 0x%08x\n", uc->uc_mcontext.ebx);
	fprintf(stderr, "ECX 0x%08x\n", uc->uc_mcontext.ecx);
	fprintf(stderr, "EDX 0x%08x\n", uc->uc_mcontext.edx);
	fprintf(stderr, "EBP 0x%08x\n", uc->uc_mcontext.ebp);
	fprintf(stderr, "EDI 0x%08x\n", uc->uc_mcontext.edi);
	fprintf(stderr, "ESI 0x%08x\n", uc->uc_mcontext.esi);
	#endif
	
	exit(-1); // The shell use non-null for failing.
}

// Variable declared as static
// so to be created in the global
// variable region, because
// if it get created in the stack
// and the signal handler use it,
// it will not have access to it
// because it has its own stack.
static struct sigaction sigsegvsa;
sigsegvsa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;
sigsegvsa.sa_sigaction = (void(*)(int, siginfo_t*, void*))faulthandler;
sigaction(SIGSEGV, &sigsegvsa, (struct sigaction*)0);

// Variable declared as static
// so to be created in the global
// variable region, because
// if they get created in the stack,
// and the signal handler use it,
// it will not have access to it
// because it has its own stack.
static struct sigaction sigfpesa;
sigfpesa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;
sigfpesa.sa_sigaction = (void(*)(int, siginfo_t*, void*))faulthandler;
sigaction(SIGFPE, &sigfpesa, (struct sigaction*)0);

// Variable declared as static
// so to be created in the global
// variable region, because
// if they get created in the stack,
// and the signal handler use it,
// it will not have access to it
// because it has its own stack.
static struct sigaction sigillsa;
sigillsa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;
sigillsa.sa_sigaction = (void(*)(int, siginfo_t*, void*))faulthandler;
sigaction(SIGILL, &sigillsa, (struct sigaction*)0);

// Variable declared as static
// so to be created in the global
// variable region, because
// if they get created in the stack,
// and the signal handler use it,
// it will not have access to it
// because it has its own stack.
static struct sigaction sigbussa;
sigbussa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;
sigbussa.sa_sigaction = (void(*)(int, siginfo_t*, void*))faulthandler;
sigaction(SIGBUS, &sigbussa, (struct sigaction*)0);


#if 0
// ### Code similar to what is below
// should be used to ignore SIGFPE thrown
// by Linux when a division by zero occur.
// Right now, this code do not work because
// obtaining the size on an instruction
// in order to compute the next instruction
// address is not trivial.

#include <signal.h>
#include <ucontext.h>

void main () {
	
	void sigfpehandler (uint signo, siginfo_t* si, ucontext_t* uc) {
		// TODO: Use function which will check
		// whether the fault occured
		// among lyrical executable instructions.
		// The executable map will be used to determine that.
		
		// TODO: Ignore SIGFPE only if fault occured among
		// lyrical executable instructions and if it was an integer division
		// by zero, which is the only instruction generated
		// by the lyricalbackend which can cause it.
		if (si->si_code == FPE_INTDIV_TRAP) { // TODO: Test also whether it was for a lyrical executable instruction.
			// TODO: The integer division generated
			// by the lyricalbackend is 2bytes,
			// so incrementing the instruction pointer
			// by 2 set it to the next instruction.
			uc->uc_mcontext.gregs[REG_EIP] += 2;
			
		} else pagefaulthandler(signo, si, uc);
	}
	
	// Variable declared as static
	// so to be created in the global
	// variable region, because
	// if they get created in the stack,
	// and the signal handler use it,
	// it will not have access to it
	// because it has its own stack.
	static struct sigaction sigfpesa;
	
	sigfpesa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;
	sigfpesa.sa_sigaction = sigfpehandler;
	sigaction(SIGFPE, &sigfpesa, (struct sigaction*)0);
	
	
	unsigned n, m = 0;
	n /= m; // This division by zero should now be ignored.
}
#endif
