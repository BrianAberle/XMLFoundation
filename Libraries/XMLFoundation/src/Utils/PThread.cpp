#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;


#ifdef WINCE
#define NEED_FTIME
#define NEED_PROCESS_AFFINITY_MASK
#define NEED_UNICODE_CONSTS
#include "WinCERuntimeC.h"
#endif

//
// This is the standard windows pthreads implementation created by John E. Bossom
// that the XMLFoundation has been using since 1999.  I found the 1999 build to be a 
// stable simple wrap over Win32 threading and therefore waited 7 years to pick up the latest
// implementation.  Various contributors have been optimizing and extending the 
// implementation and this is the code base released on Dec 22, 2006 (version 2.8.0.)
// That was the most recent release as of July 2007.  I noticed that some of the new 
// code is incomplete: spin_lock does not work for multi cpu machines, and its just a mutex.
// So check your return values, or even better look at the implementation of what you want to use.
// 
// Since a CriticalSection is faster than a Mutex in Windows, it would be beneficial to wrap one 
// with a POSIX mutex interface for certain situations.  Regardless of implementation, it is very 
// wise to code threads to the POSIX interface because then your code can go places it otherwise cannot.
//
// Several modifications have been made to this John E. Bossom code to make it more ideal 
// for a static  build  and to assimilate into the XMLFoundation.  The changes are as follows:

//	
// 1. Added to ptw32_callUserDestroyRoutines.c
// 
//#ifdef _WIN32
//#include <eh.h> // for terminate() when MSVC is compiling C++
//#endif
//
//
//
// 2. Added WSA_ERROR_FIX to pthread_getspecific.c 
//  This is done because not everyone who uses this library wants pthreads, and that code
//	forces a link to wsock32.lib for the reference to WSAGetLastError() and WSASetLastError()
//  If you are using pthreads, and sockets and run into a case where WSAGetLastError() does not work
//  then you'll need to build the library with WSA_ERROR_FIX defined.
//
//
// 3. Added the SelfInitPThreads class at the bottom of pthread.h, and g_Initialized to this file
//    because Unix apps don't need to init pthreads, and need to #ifdef out the code, and if
//	  windows apps forget - they crash violently.  This way you can't forget and don't need the #defines.
//
// 4 removed dll.c
//
// 5. This file is pthread.c with a .cpp extension and all includes expanded within it.
//	  It compiles as C++ rather than C, because of the SelfInitPThreads class added to it
//    Each function of pthreads was organized in to it's own .c source file, they have been
//    consolidated into this single source file for easier integration and utility style consistency.
//    Also so that this one file may be easily added or removed from the XMLFoundation that
//    does not depend on it.
//
//	  Search for the word CONSOLIDATION in this file to see how it was done.
//	  Each file is separated by this delimiter inside this file:
//
//===========================SOURCE FILE BREAK===============================
//
//
//  This entire library is free software, and it's open source, and the entire library
//  comes with very simple copyright agreements, except for this file.
//  Most of the utilities in this library were created by United Business Technologies
//  and certainly UBT would be capable of putting a POSIX standard interface over the
//  small subset of Win32 threading needed to keep all it's codebase cross platform.
//	John E. Bossom released this source code so that it was not necessary for everyone to do that.
//
//  As I understand all the laywer speak that follows, since the library as a whole is not 
//  a POSIX threads implementation, but infact a complete cross platform application development 
//  framework complete with parsers, protocols, design patterns and CORBA / COM middleware integration
//  that is neither based on standard POSIX threads nor uses this pthreads in itself - therefore, the 
//  library as a  whole is not forced to fall under the complex verbage that entangles this one file.
//	The licence seems to want to attach itself like a virus to other works.
//  This license is contained to this single source code file.  
//
//  If you cannot agree to this license then don't use this source file.
//  You must direct any inquiries about this license to John E. Bossom, or GNU not UBT.
// 
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * pthread.c
 *
 * Description:
 * This translation unit agregates pthreads-win32 translation units.
 * It is used for inline optimisation of the library,
 * maximising for speed at the expense of size.
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2005 Pthreads-win32 contributors
 * 
 *      Contact Email: rpj@callisto.canberra.edu.au
 * 
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// COPYING.LIB license file that applies to all source in this single file only

/*

		  GNU LESSER GENERAL PUBLIC LICENSE
		       Version 2.1, February 1999

 Copyright (C) 1991, 1999 Free Software Foundation, Inc.
     59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 Everyone is permitted to copy and distribute verbatim copies
 of this license document, but changing it is not allowed.

[This is the first released version of the Lesser GPL.  It also counts
 as the successor of the GNU Library Public License, version 2, hence
 the version number 2.1.]

			    Preamble

  The licenses for most software are designed to take away your
freedom to share and change it.  By contrast, the GNU General Public
Licenses are intended to guarantee your freedom to share and change
free software--to make sure the software is free for all its users.

  This license, the Lesser General Public License, applies to some
specially designated software packages--typically libraries--of the
Free Software Foundation and other authors who decide to use it.  You
can use it too, but we suggest you first think carefully about whether
this license or the ordinary General Public License is the better
strategy to use in any particular case, based on the explanations below.

  When we speak of free software, we are referring to freedom of use,
not price.  Our General Public Licenses are designed to make sure that
you have the freedom to distribute copies of free software (and charge
for this service if you wish); that you receive source code or can get
it if you want it; that you can change the software and use pieces of
it in new free programs; and that you are informed that you can do
these things.

  To protect your rights, we need to make restrictions that forbid
distributors to deny you these rights or to ask you to surrender these
rights.  These restrictions translate to certain responsibilities for
you if you distribute copies of the library or if you modify it.

  For example, if you distribute copies of the library, whether gratis
or for a fee, you must give the recipients all the rights that we gave
you.  You must make sure that they, too, receive or can get the source
code.  If you link other code with the library, you must provide
complete object files to the recipients, so that they can relink them
with the library after making changes to the library and recompiling
it.  And you must show them these terms so they know their rights.

  We protect your rights with a two-step method: (1) we copyright the
library, and (2) we offer you this license, which gives you legal
permission to copy, distribute and/or modify the library.

  To protect each distributor, we want to make it very clear that
there is no warranty for the free library.  Also, if the library is
modified by someone else and passed on, the recipients should know
that what they have is not the original version, so that the original
author's reputation will not be affected by problems that might be
introduced by others.

  Finally, software patents pose a constant threat to the existence of
any free program.  We wish to make sure that a company cannot
effectively restrict the users of a free program by obtaining a
restrictive license from a patent holder.  Therefore, we insist that
any patent license obtained for a version of the library must be
consistent with the full freedom of use specified in this license.

  Most GNU software, including some libraries, is covered by the
ordinary GNU General Public License.  This license, the GNU Lesser
General Public License, applies to certain designated libraries, and
is quite different from the ordinary General Public License.  We use
this license for certain libraries in order to permit linking those
libraries into non-free programs.

  When a program is linked with a library, whether statically or using
a shared library, the combination of the two is legally speaking a
combined work, a derivative of the original library.  The ordinary
General Public License therefore permits such linking only if the
entire combination fits its criteria of freedom.  The Lesser General
Public License permits more lax criteria for linking other code with
the library.

  We call this license the "Lesser" General Public License because it
does Less to protect the user's freedom than the ordinary General
Public License.  It also provides other free software developers Less
of an advantage over competing non-free programs.  These disadvantages
are the reason we use the ordinary General Public License for many
libraries.  However, the Lesser license provides advantages in certain
special circumstances.

  For example, on rare occasions, there may be a special need to
encourage the widest possible use of a certain library, so that it becomes
a de-facto standard.  To achieve this, non-free programs must be
allowed to use the library.  A more frequent case is that a free
library does the same job as widely used non-free libraries.  In this
case, there is little to gain by limiting the free library to free
software only, so we use the Lesser General Public License.

  In other cases, permission to use a particular library in non-free
programs enables a greater number of people to use a large body of
free software.  For example, permission to use the GNU C Library in
non-free programs enables many more people to use the whole GNU
operating system, as well as its variant, the GNU/Linux operating
system.

  Although the Lesser General Public License is Less protective of the
users' freedom, it does ensure that the user of a program that is
linked with the Library has the freedom and the wherewithal to run
that program using a modified version of the Library.

  The precise terms and conditions for copying, distribution and
modification follow.  Pay close attention to the difference between a
"work based on the library" and a "work that uses the library".  The
former contains code derived from the library, whereas the latter must
be combined with the library in order to run.

		  GNU LESSER GENERAL PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. This License Agreement applies to any software library or other
program which contains a notice placed by the copyright holder or
other authorized party saying it may be distributed under the terms of
this Lesser General Public License (also called "this License").
Each licensee is addressed as "you".

  A "library" means a collection of software functions and/or data
prepared so as to be conveniently linked with application programs
(which use some of those functions and data) to form executables.

  The "Library", below, refers to any such software library or work
which has been distributed under these terms.  A "work based on the
Library" means either the Library or any derivative work under
copyright law: that is to say, a work containing the Library or a
portion of it, either verbatim or with modifications and/or translated
straightforwardly into another language.  (Hereinafter, translation is
included without limitation in the term "modification".)

  "Source code" for a work means the preferred form of the work for
making modifications to it.  For a library, complete source code means
all the source code for all modules it contains, plus any associated
interface definition files, plus the scripts used to control compilation
and installation of the library.

  Activities other than copying, distribution and modification are not
covered by this License; they are outside its scope.  The act of
running a program using the Library is not restricted, and output from
such a program is covered only if its contents constitute a work based
on the Library (independent of the use of the Library in a tool for
writing it).  Whether that is true depends on what the Library does
and what the program that uses the Library does.
  
  1. You may copy and distribute verbatim copies of the Library's
complete source code as you receive it, in any medium, provided that
you conspicuously and appropriately publish on each copy an
appropriate copyright notice and disclaimer of warranty; keep intact
all the notices that refer to this License and to the absence of any
warranty; and distribute a copy of this License along with the
Library.

  You may charge a fee for the physical act of transferring a copy,
and you may at your option offer warranty protection in exchange for a
fee.

  2. You may modify your copy or copies of the Library or any portion
of it, thus forming a work based on the Library, and copy and
distribute such modifications or work under the terms of Section 1
above, provided that you also meet all of these conditions:

    a) The modified work must itself be a software library.

    b) You must cause the files modified to carry prominent notices
    stating that you changed the files and the date of any change.

    c) You must cause the whole of the work to be licensed at no
    charge to all third parties under the terms of this License.

    d) If a facility in the modified Library refers to a function or a
    table of data to be supplied by an application program that uses
    the facility, other than as an argument passed when the facility
    is invoked, then you must make a good faith effort to ensure that,
    in the event an application does not supply such function or
    table, the facility still operates, and performs whatever part of
    its purpose remains meaningful.

    (For example, a function in a library to compute square roots has
    a purpose that is entirely well-defined independent of the
    application.  Therefore, Subsection 2d requires that any
    application-supplied function or table used by this function must
    be optional: if the application does not supply it, the square
    root function must still compute square roots.)

These requirements apply to the modified work as a whole.  If
identifiable sections of that work are not derived from the Library,
and can be reasonably considered independent and separate works in
themselves, then this License, and its terms, do not apply to those
sections when you distribute them as separate works.  But when you
distribute the same sections as part of a whole which is a work based
on the Library, the distribution of the whole must be on the terms of
this License, whose permissions for other licensees extend to the
entire whole, and thus to each and every part regardless of who wrote
it.

Thus, it is not the intent of this section to claim rights or contest
your rights to work written entirely by you; rather, the intent is to
exercise the right to control the distribution of derivative or
collective works based on the Library.

In addition, mere aggregation of another work not based on the Library
with the Library (or with a work based on the Library) on a volume of
a storage or distribution medium does not bring the other work under
the scope of this License.

  3. You may opt to apply the terms of the ordinary GNU General Public
License instead of this License to a given copy of the Library.  To do
this, you must alter all the notices that refer to this License, so
that they refer to the ordinary GNU General Public License, version 2,
instead of to this License.  (If a newer version than version 2 of the
ordinary GNU General Public License has appeared, then you can specify
that version instead if you wish.)  Do not make any other change in
these notices.

  Once this change is made in a given copy, it is irreversible for
that copy, so the ordinary GNU General Public License applies to all
subsequent copies and derivative works made from that copy.

  This option is useful when you wish to copy part of the code of
the Library into a program that is not a library.

  4. You may copy and distribute the Library (or a portion or
derivative of it, under Section 2) in object code or executable form
under the terms of Sections 1 and 2 above provided that you accompany
it with the complete corresponding machine-readable source code, which
must be distributed under the terms of Sections 1 and 2 above on a
medium customarily used for software interchange.

  If distribution of object code is made by offering access to copy
from a designated place, then offering equivalent access to copy the
source code from the same place satisfies the requirement to
distribute the source code, even though third parties are not
compelled to copy the source along with the object code.

  5. A program that contains no derivative of any portion of the
Library, but is designed to work with the Library by being compiled or
linked with it, is called a "work that uses the Library".  Such a
work, in isolation, is not a derivative work of the Library, and
therefore falls outside the scope of this License.

  However, linking a "work that uses the Library" with the Library
creates an executable that is a derivative of the Library (because it
contains portions of the Library), rather than a "work that uses the
library".  The executable is therefore covered by this License.
Section 6 states terms for distribution of such executables.

  When a "work that uses the Library" uses material from a header file
that is part of the Library, the object code for the work may be a
derivative work of the Library even though the source code is not.
Whether this is true is especially significant if the work can be
linked without the Library, or if the work is itself a library.  The
threshold for this to be true is not precisely defined by law.

  If such an object file uses only numerical parameters, data
structure layouts and accessors, and small macros and small inline
functions (ten lines or less in length), then the use of the object
file is unrestricted, regardless of whether it is legally a derivative
work.  (Executables containing this object code plus portions of the
Library will still fall under Section 6.)

  Otherwise, if the work is a derivative of the Library, you may
distribute the object code for the work under the terms of Section 6.
Any executables containing that work also fall under Section 6,
whether or not they are linked directly with the Library itself.

  6. As an exception to the Sections above, you may also combine or
link a "work that uses the Library" with the Library to produce a
work containing portions of the Library, and distribute that work
under terms of your choice, provided that the terms permit
modification of the work for the customer's own use and reverse
engineering for debugging such modifications.

  You must give prominent notice with each copy of the work that the
Library is used in it and that the Library and its use are covered by
this License.  You must supply a copy of this License.  If the work
during execution displays copyright notices, you must include the
copyright notice for the Library among them, as well as a reference
directing the user to the copy of this License.  Also, you must do one
of these things:

    a) Accompany the work with the complete corresponding
    machine-readable source code for the Library including whatever
    changes were used in the work (which must be distributed under
    Sections 1 and 2 above); and, if the work is an executable linked
    with the Library, with the complete machine-readable "work that
    uses the Library", as object code and/or source code, so that the
    user can modify the Library and then relink to produce a modified
    executable containing the modified Library.  (It is understood
    that the user who changes the contents of definitions files in the
    Library will not necessarily be able to recompile the application
    to use the modified definitions.)

    b) Use a suitable shared library mechanism for linking with the
    Library.  A suitable mechanism is one that (1) uses at run time a
    copy of the library already present on the user's computer system,
    rather than copying library functions into the executable, and (2)
    will operate properly with a modified version of the library, if
    the user installs one, as long as the modified version is
    interface-compatible with the version that the work was made with.

    c) Accompany the work with a written offer, valid for at
    least three years, to give the same user the materials
    specified in Subsection 6a, above, for a charge no more
    than the cost of performing this distribution.

    d) If distribution of the work is made by offering access to copy
    from a designated place, offer equivalent access to copy the above
    specified materials from the same place.

    e) Verify that the user has already received a copy of these
    materials or that you have already sent this user a copy.

  For an executable, the required form of the "work that uses the
Library" must include any data and utility programs needed for
reproducing the executable from it.  However, as a special exception,
the materials to be distributed need not include anything that is
normally distributed (in either source or binary form) with the major
components (compiler, kernel, and so on) of the operating system on
which the executable runs, unless that component itself accompanies
the executable.

  It may happen that this requirement contradicts the license
restrictions of other proprietary libraries that do not normally
accompany the operating system.  Such a contradiction means you cannot
use both them and the Library together in an executable that you
distribute.

  7. You may place library facilities that are a work based on the
Library side-by-side in a single library together with other library
facilities not covered by this License, and distribute such a combined
library, provided that the separate distribution of the work based on
the Library and of the other library facilities is otherwise
permitted, and provided that you do these two things:

    a) Accompany the combined library with a copy of the same work
    based on the Library, uncombined with any other library
    facilities.  This must be distributed under the terms of the
    Sections above.

    b) Give prominent notice with the combined library of the fact
    that part of it is a work based on the Library, and explaining
    where to find the accompanying uncombined form of the same work.

  8. You may not copy, modify, sublicense, link with, or distribute
the Library except as expressly provided under this License.  Any
attempt otherwise to copy, modify, sublicense, link with, or
distribute the Library is void, and will automatically terminate your
rights under this License.  However, parties who have received copies,
or rights, from you under this License will not have their licenses
terminated so long as such parties remain in full compliance.

  9. You are not required to accept this License, since you have not
signed it.  However, nothing else grants you permission to modify or
distribute the Library or its derivative works.  These actions are
prohibited by law if you do not accept this License.  Therefore, by
modifying or distributing the Library (or any work based on the
Library), you indicate your acceptance of this License to do so, and
all its terms and conditions for copying, distributing or modifying
the Library or works based on it.

  10. Each time you redistribute the Library (or any work based on the
Library), the recipient automatically receives a license from the
original licensor to copy, distribute, link with or modify the Library
subject to these terms and conditions.  You may not impose any further
restrictions on the recipients' exercise of the rights granted herein.
You are not responsible for enforcing compliance by third parties with
this License.

  11. If, as a consequence of a court judgment or allegation of patent
infringement or for any other reason (not limited to patent issues),
conditions are imposed on you (whether by court order, agreement or
otherwise) that contradict the conditions of this License, they do not
excuse you from the conditions of this License.  If you cannot
distribute so as to satisfy simultaneously your obligations under this
License and any other pertinent obligations, then as a consequence you
may not distribute the Library at all.  For example, if a patent
license would not permit royalty-free redistribution of the Library by
all those who receive copies directly or indirectly through you, then
the only way you could satisfy both it and this License would be to
refrain entirely from distribution of the Library.

If any portion of this section is held invalid or unenforceable under any
particular circumstance, the balance of the section is intended to apply,
and the section as a whole is intended to apply in other circumstances.

It is not the purpose of this section to induce you to infringe any
patents or other property right claims or to contest validity of any
such claims; this section has the sole purpose of protecting the
integrity of the free software distribution system which is
implemented by public license practices.  Many people have made
generous contributions to the wide range of software distributed
through that system in reliance on consistent application of that
system; it is up to the author/donor to decide if he or she is willing
to distribute software through any other system and a licensee cannot
impose that choice.

This section is intended to make thoroughly clear what is believed to
be a consequence of the rest of this License.

  12. If the distribution and/or use of the Library is restricted in
certain countries either by patents or by copyrighted interfaces, the
original copyright holder who places the Library under this License may add
an explicit geographical distribution limitation excluding those countries,
so that distribution is permitted only in or among countries not thus
excluded.  In such case, this License incorporates the limitation as if
written in the body of this License.

  13. The Free Software Foundation may publish revised and/or new
versions of the Lesser General Public License from time to time.
Such new versions will be similar in spirit to the present version,
but may differ in detail to address new problems or concerns.

Each version is given a distinguishing version number.  If the Library
specifies a version number of this License which applies to it and
"any later version", you have the option of following the terms and
conditions either of that version or of any later version published by
the Free Software Foundation.  If the Library does not specify a
license version number, you may choose any version ever published by
the Free Software Foundation.

  14. If you wish to incorporate parts of the Library into other free
programs whose distribution conditions are incompatible with these,
write to the author to ask for permission.  For software which is
copyrighted by the Free Software Foundation, write to the Free
Software Foundation; we sometimes make exceptions for this.  Our
decision will be guided by the two goals of preserving the free status
of all derivatives of our free software and of promoting the sharing
and reuse of software generally.

			    NO WARRANTY

  15. BECAUSE THE LIBRARY IS LICENSED FREE OF CHARGE, THERE IS NO
WARRANTY FOR THE LIBRARY, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR
OTHER PARTIES PROVIDE THE LIBRARY "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
LIBRARY IS WITH YOU.  SHOULD THE LIBRARY PROVE DEFECTIVE, YOU ASSUME
THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

  16. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN
WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY
AND/OR REDISTRIBUTE THE LIBRARY AS PERMITTED ABOVE, BE LIABLE TO YOU
FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE
LIBRARY (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING
RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A
FAILURE OF THE LIBRARY TO OPERATE WITH ANY OTHER SOFTWARE), EVEN IF
SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGES.

		     END OF TERMS AND CONDITIONS

           How to Apply These Terms to Your New Libraries

  If you develop a new library, and you want it to be of the greatest
possible use to the public, we recommend making it free software that
everyone can redistribute and change.  You can do so by permitting
redistribution under these terms (or, alternatively, under the terms of the
ordinary General Public License).

  To apply these terms, attach the following notices to the library.  It is
safest to attach them to the start of each source file to most effectively
convey the exclusion of warranty; and each file should have at least the
"copyright" line and a pointer to where the full notice is found.

    <one line to give the library's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Also add information on how to contact you by electronic and paper mail.

You should also get your employer (if you work as a programmer) or your
school, if any, to sign a "copyright disclaimer" for the library, if
necessary.  Here is a sample; alter the names:

  Yoyodyne, Inc., hereby disclaims all copyright interest in the
  library `Frob' (a library for tweaking knobs) written by James Random Hacker.

  <signature of Ty Coon>, 1 April 1990
  Ty Coon, President of Vice

That's all there is to it!


*/





#include "Win32PThreads/pthread.h"
#include "Win32PThreads/implement.h"


//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "private.c"
//
//#include "ptw32_InterlockedCompareExchange.c"
//#include "ptw32_MCS_lock.c"
//#include "ptw32_is_attr.c"
//#include "ptw32_processInitialize.c"
//#include "ptw32_processTerminate.c"
//#include "ptw32_threadStart.c"
//#include "ptw32_threadDestroy.c"
//#include "ptw32_tkAssocCreate.c"
//#include "ptw32_tkAssocDestroy.c"
//#include "ptw32_callUserDestroyRoutines.c"
//#include "ptw32_semwait.c"
//#include "ptw32_timespec.c"
//#include "ptw32_relmillisecs.c"
//#include "ptw32_throw.c"
//#include "ptw32_getprocessors.c"
//////////////////////////////////////////////////////////////////////////////
/*
 * ptw32_InterlockedCompareExchange --
 *
 * Originally needed because W9x doesn't support InterlockedCompareExchange.
 * We now use this version wherever possible so we can inline it.
 */

PTW32_INTERLOCKED_LONG WINAPI
ptw32_InterlockedCompareExchange (PTW32_INTERLOCKED_LPLONG location,
				  PTW32_INTERLOCKED_LONG value,
				  PTW32_INTERLOCKED_LONG comparand)
{

#if defined(__WATCOMC__)
/* Don't report that result is not assigned a value before being referenced */
#pragma disable_message (200)
#endif

  PTW32_INTERLOCKED_LONG result;

  /*
   * Using the LOCK prefix on uni-processor machines is significantly slower
   * and it is not necessary. The overhead of the conditional below is
   * negligible in comparison. Since an optimised DLL will inline this
   * routine, this will be faster than calling the system supplied
   * Interlocked routine, which appears to avoid the LOCK prefix on
   * uniprocessor systems. So one DLL works for all systems.
   */
  if (ptw32_smp_system)

/* *INDENT-OFF* */

#if defined(_M_IX86) || defined(_X86_)

#if defined(_MSC_VER) || defined(__WATCOMC__) || (defined(__BORLANDC__) && defined(HAVE_TASM32))
#define HAVE_INLINABLE_INTERLOCKED_CMPXCHG
    {
      _asm {
	PUSH         ecx
	PUSH         edx
	MOV          ecx,dword ptr [location]
	MOV          edx,dword ptr [value]
	MOV          eax,dword ptr [comparand]
	LOCK CMPXCHG dword ptr [ecx],edx
	MOV          dword ptr [result], eax
	POP          edx
	POP          ecx
      }
    }
  else
    {
      _asm {
	PUSH         ecx
	PUSH         edx
	MOV          ecx,dword ptr [location]
	MOV          edx,dword ptr [value]
	MOV          eax,dword ptr [comparand]
	CMPXCHG      dword ptr [ecx],edx
	MOV          dword ptr [result], eax
	POP          edx
	POP          ecx
      }
    }

#elif defined(__GNUC__)
#define HAVE_INLINABLE_INTERLOCKED_CMPXCHG

    {
      __asm__ __volatile__
	(
	 "lock\n\t"
	 "cmpxchgl       %2,%1"      /* if (EAX == [location])  */
	                             /*   [location] = value    */
                                     /* else                    */
                                     /*   EAX = [location]      */
	 :"=a" (result)
	 :"m"  (*location), "r" (value), "a" (comparand));
    }
  else
    {
      __asm__ __volatile__
	(
	 "cmpxchgl       %2,%1"      /* if (EAX == [location])  */
	                             /*   [location] = value    */
                                     /* else                    */
                                     /*   EAX = [location]      */
	 :"=a" (result)
	 :"m"  (*location), "r" (value), "a" (comparand));
    }

#endif

#else

  /*
   * If execution gets to here then we're running on a currently
   * unsupported processor or compiler.
   */

  result = 0;

#endif

/* *INDENT-ON* */

  return result;

#if defined(__WATCOMC__)
#pragma enable_message (200)
#endif

}

/*
 * ptw32_InterlockedExchange --
 *
 * We now use this version wherever possible so we can inline it.
 */

LONG WINAPI
ptw32_InterlockedExchange (LPLONG location,
			   LONG value)
{

#if defined(__WATCOMC__)
/* Don't report that result is not assigned a value before being referenced */
#pragma disable_message (200)
#endif

  LONG result;

  /*
   * The XCHG instruction always locks the bus with or without the
   * LOCKED prefix. This makes it significantly slower than CMPXCHG on
   * uni-processor machines. The Windows InterlockedExchange function
   * is nearly 3 times faster than the XCHG instruction, so this routine
   * is not yet very useful for speeding up pthreads.
   */
  if (ptw32_smp_system)

/* *INDENT-OFF* */

#if defined(_M_IX86) || defined(_X86_)

#if defined(_MSC_VER) || defined(__WATCOMC__) || (defined(__BORLANDC__) && defined(HAVE_TASM32))
#define HAVE_INLINABLE_INTERLOCKED_XCHG

    {
      _asm {
	PUSH         ecx
	MOV          ecx,dword ptr [location]
	MOV          eax,dword ptr [value]
	XCHG         dword ptr [ecx],eax
	MOV          dword ptr [result], eax
        POP          ecx
      }
    }
  else
    {
      /*
       * Faster version of XCHG for uni-processor systems because
       * it doesn't lock the bus. If an interrupt or context switch
       * occurs between the MOV and the CMPXCHG then the value in
       * 'location' may have changed, in which case we will loop
       * back to do the MOV again.
       *
       * FIXME! Need memory barriers for the MOV+CMPXCHG combo?
       *
       * Tests show that this routine has almost identical timing
       * to Win32's InterlockedExchange(), which is much faster than
       * using the inlined 'xchg' instruction above, so it's probably
       * doing something similar to this (on UP systems).
       *
       * Can we do without the PUSH/POP instructions?
       */
      _asm {
	PUSH         ecx
	PUSH         edx
	MOV          ecx,dword ptr [location]
	MOV          edx,dword ptr [value]
L1:	MOV          eax,dword ptr [ecx]
	CMPXCHG      dword ptr [ecx],edx
	JNZ          L1
	MOV          dword ptr [result], eax
	POP          edx
        POP          ecx
      }
    }

#elif defined(__GNUC__)
#define HAVE_INLINABLE_INTERLOCKED_XCHG

    {
      __asm__ __volatile__
	(
	 "xchgl          %2,%1"
	 :"=r" (result)
	 :"m"  (*location), "0" (value));
    }
  else
    {
      /*
       * Faster version of XCHG for uni-processor systems because
       * it doesn't lock the bus. If an interrupt or context switch
       * occurs between the movl and the cmpxchgl then the value in
       * 'location' may have changed, in which case we will loop
       * back to do the movl again.
       *
       * FIXME! Need memory barriers for the MOV+CMPXCHG combo?
       *
       * Tests show that this routine has almost identical timing
       * to Win32's InterlockedExchange(), which is much faster than
       * using the an inlined 'xchg' instruction, so it's probably
       * doing something similar to this (on UP systems).
       */
      __asm__ __volatile__
	(
	 "0:\n\t"
	 "movl           %1,%%eax\n\t"
	 "cmpxchgl       %2,%1\n\t"
	 "jnz            0b"
	 :"=&a" (result)
	 :"m"  (*location), "r" (value));
    }

#endif

#else

  /*
   * If execution gets to here then we're running on a currently
   * unsupported processor or compiler.
   */

  result = 0;

#endif

/* *INDENT-ON* */

  return result;

#if defined(__WATCOMC__)
#pragma enable_message (200)
#endif

}


#if 1

#if defined(PTW32_BUILD_INLINED) && defined(HAVE_INLINABLE_INTERLOCKED_CMPXCHG)
#undef PTW32_INTERLOCKED_COMPARE_EXCHANGE
#define PTW32_INTERLOCKED_COMPARE_EXCHANGE ptw32_InterlockedCompareExchange
#endif

#if defined(PTW32_BUILD_INLINED) && defined(HAVE_INLINABLE_INTERLOCKED_XCHG)
#undef PTW32_INTERLOCKED_EXCHANGE
#define PTW32_INTERLOCKED_EXCHANGE ptw32_InterlockedExchange
#endif

#endif
// ===========================SOURCE FILE BREAK===============================

/*
 * ptw32_mcs_flag_set -- notify another thread about an event.
 * 
 * Set event if an event handle has been stored in the flag, and
 * set flag to -1 otherwise. Note that -1 cannot be a valid handle value.
 */
INLINE void 
ptw32_mcs_flag_set (LONG * flag)
{
  HANDLE e = (HANDLE)PTW32_INTERLOCKED_COMPARE_EXCHANGE(
						(PTW32_INTERLOCKED_LPLONG)flag,
						(PTW32_INTERLOCKED_LONG)-1,
						(PTW32_INTERLOCKED_LONG)0);
  if ((HANDLE)0 != e)
    {
      /* another thread has already stored an event handle in the flag */
      SetEvent(e);
    }
}

/*
 * ptw32_mcs_flag_set -- wait for notification from another.
 * 
 * Store an event handle in the flag and wait on it if the flag has not been
 * set, and proceed without creating an event otherwise.
 */
INLINE void 
ptw32_mcs_flag_wait (LONG * flag)
{
  if (0 == InterlockedExchangeAdd((LPLONG)flag, 0)) /* MBR fence */
    {
      /* the flag is not set. create event. */

      HANDLE e = CreateEvent(NULL, PTW32_FALSE, PTW32_FALSE, NULL);

      if (0 == PTW32_INTERLOCKED_COMPARE_EXCHANGE(
			                  (PTW32_INTERLOCKED_LPLONG)flag,
			                  (PTW32_INTERLOCKED_LONG)e,
			                  (PTW32_INTERLOCKED_LONG)0))
	{
	  /* stored handle in the flag. wait on it now. */
	  WaitForSingleObject(e, INFINITE);
	}

      CloseHandle(e);
    }
}

/*
 * ptw32_mcs_lock_acquire -- acquire an MCS lock.
 * 
 * See: 
 * J. M. Mellor-Crummey and M. L. Scott.
 * Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors.
 * ACM Transactions on Computer Systems, 9(1):21-65, Feb. 1991.
 */
INLINE void 
ptw32_mcs_lock_acquire (ptw32_mcs_lock_t * lock, ptw32_mcs_local_node_t * node)
{
  ptw32_mcs_local_node_t  *pred;
  
  node->lock = lock;
  node->nextFlag = 0;
  node->readyFlag = 0;
  node->next = 0; /* initially, no successor */
  
  /* queue for the lock */
  pred = (ptw32_mcs_local_node_t *)PTW32_INTERLOCKED_EXCHANGE((LPLONG)lock,
						              (LONG)node);

  if (0 != pred)
    {
      /* the lock was not free. link behind predecessor. */
      pred->next = node;
      ptw32_mcs_flag_set(&pred->nextFlag);
      ptw32_mcs_flag_wait(&node->readyFlag);
    }
}

/*
 * ptw32_mcs_lock_release -- release an MCS lock.
 * 
 * See: 
 * J. M. Mellor-Crummey and M. L. Scott.
 * Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors.
 * ACM Transactions on Computer Systems, 9(1):21-65, Feb. 1991.
 */
INLINE void 
ptw32_mcs_lock_release (ptw32_mcs_local_node_t * node)
{
  ptw32_mcs_lock_t *lock = node->lock;
  ptw32_mcs_local_node_t *next = (ptw32_mcs_local_node_t *)
    InterlockedExchangeAdd((LPLONG)&node->next, 0); /* MBR fence */

  if (0 == next)
    {
      /* no known successor */

      if (node == (ptw32_mcs_local_node_t *)
	  PTW32_INTERLOCKED_COMPARE_EXCHANGE((PTW32_INTERLOCKED_LPLONG)lock,
					     (PTW32_INTERLOCKED_LONG)0,
					     (PTW32_INTERLOCKED_LONG)node))
	{
	  /* no successor, lock is free now */
	  return;
	}
  
      /* wait for successor */
      ptw32_mcs_flag_wait(&node->nextFlag);
      next = (ptw32_mcs_local_node_t *)
	InterlockedExchangeAdd((LPLONG)&node->next, 0); /* MBR fence */
    }

  /* pass the lock */
  ptw32_mcs_flag_set(&next->readyFlag);
}
// ===========================SOURCE FILE BREAK===============================
int
ptw32_is_attr (const pthread_attr_t * attr)
{
  /* Return 0 if the attr object is valid, non-zero otherwise. */

  return (attr == NULL ||
	  *attr == NULL || (*attr)->valid != PTW32_ATTR_VALID);
}
// ===========================SOURCE FILE BREAK===============================
int
ptw32_processInitialize (void)
     /*
      * ------------------------------------------------------
      * DOCPRIVATE
      *      This function performs process wide initialization for
      *      the pthread library.
      *
      * PARAMETERS
      *      N/A
      *
      * DESCRIPTION
      *      This function performs process wide initialization for
      *      the pthread library.
      *      If successful, this routine sets the global variable
      *      ptw32_processInitialized to TRUE.
      *
      * RESULTS
      *              TRUE    if successful,
      *              FALSE   otherwise
      *
      * ------------------------------------------------------
      */
{
  if (ptw32_processInitialized)
    {
      /* 
       * Ignore if already initialized. this is useful for 
       * programs that uses a non-dll pthread
       * library. Such programs must call ptw32_processInitialize() explicitly,
       * since this initialization routine is automatically called only when
       * the dll is loaded.
       */
      return PTW32_TRUE;
    }

  ptw32_processInitialized = PTW32_TRUE;

  /*
   * Initialize Keys
   */
  if ((pthread_key_create (&ptw32_selfThreadKey, NULL) != 0) ||
      (pthread_key_create (&ptw32_cleanupKey, NULL) != 0))
    {

      ptw32_processTerminate ();
    }

  /* 
   * Set up the global locks.
   */
  InitializeCriticalSection (&ptw32_thread_reuse_lock);
  InitializeCriticalSection (&ptw32_mutex_test_init_lock);
  InitializeCriticalSection (&ptw32_cond_list_lock);
  InitializeCriticalSection (&ptw32_cond_test_init_lock);
  InitializeCriticalSection (&ptw32_rwlock_test_init_lock);
  InitializeCriticalSection (&ptw32_spinlock_test_init_lock);

  return (ptw32_processInitialized);

}				/* processInitialize */
// ===========================SOURCE FILE BREAK===============================
void
ptw32_processTerminate (void)
     /*
      * ------------------------------------------------------
      * DOCPRIVATE
      *      This function performs process wide termination for
      *      the pthread library.
      *
      * PARAMETERS
      *      N/A
      *
      * DESCRIPTION
      *      This function performs process wide termination for
      *      the pthread library.
      *      This routine sets the global variable
      *      ptw32_processInitialized to FALSE
      *
      * RESULTS
      *              N/A
      *
      * ------------------------------------------------------
      */
{
  if (ptw32_processInitialized)
    {
      ptw32_thread_t * tp, * tpNext;

      if (ptw32_selfThreadKey != NULL)
	{
	  /*
	   * Release ptw32_selfThreadKey
	   */
	  pthread_key_delete (ptw32_selfThreadKey);

	  ptw32_selfThreadKey = NULL;
	}

      if (ptw32_cleanupKey != NULL)
	{
	  /*
	   * Release ptw32_cleanupKey
	   */
	  pthread_key_delete (ptw32_cleanupKey);

	  ptw32_cleanupKey = NULL;
	}

      EnterCriticalSection (&ptw32_thread_reuse_lock);

      tp = ptw32_threadReuseTop;
      while (tp != PTW32_THREAD_REUSE_EMPTY)
	{
	  tpNext = tp->prevReuse;
	  free (tp);
	  tp = tpNext;
	}

      LeaveCriticalSection (&ptw32_thread_reuse_lock);

      /* 
       * Destroy the global locks and other objects.
       */
      DeleteCriticalSection (&ptw32_spinlock_test_init_lock);
      DeleteCriticalSection (&ptw32_rwlock_test_init_lock);
      DeleteCriticalSection (&ptw32_cond_test_init_lock);
      DeleteCriticalSection (&ptw32_cond_list_lock);
      DeleteCriticalSection (&ptw32_mutex_test_init_lock);
      DeleteCriticalSection (&ptw32_thread_reuse_lock);

      ptw32_processInitialized = PTW32_FALSE;
    }

}				/* processTerminate */
// ===========================SOURCE FILE BREAK===============================
#ifdef __CLEANUP_SEH

static DWORD
ExceptionFilter (EXCEPTION_POINTERS * ep, DWORD * ei)
{
  switch (ep->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_PTW32_SERVICES:
      {
	DWORD param;
	DWORD numParams = ep->ExceptionRecord->NumberParameters;

	numParams = (numParams > 3) ? 3 : numParams;

	for (param = 0; param < numParams; param++)
	  {
	    ei[param] = ep->ExceptionRecord->ExceptionInformation[param];
	  }

	return EXCEPTION_EXECUTE_HANDLER;
	break;
      }
    default:
      {
	/*
	 * A system unexpected exception has occurred running the user's
	 * routine. We need to cleanup before letting the exception
	 * out of thread scope.
	 */
	pthread_t self = pthread_self ();

	(void) pthread_mutex_destroy (&((ptw32_thread_t *)self.p)->cancelLock);
	ptw32_callUserDestroyRoutines (self);

	return EXCEPTION_CONTINUE_SEARCH;
	break;
      }
    }
}

#elif defined(__CLEANUP_CXX)

#if defined(_MSC_VER)
# include <eh.h>
#elif defined(__WATCOMC__)
# include <eh.h>
# include <exceptio.h>
typedef terminate_handler
  terminate_function;
#else
# if defined(__GNUC__) && __GNUC__ < 3
#   include <new.h>
# else
#   include <new>
using
  std::terminate_handler;
using
  std::terminate;
using
  std::set_terminate;
# endif
typedef terminate_handler
  terminate_function;
#endif

static terminate_function
  ptw32_oldTerminate;

void
ptw32_terminate ()
{
  set_terminate (ptw32_oldTerminate);
  (void) pthread_win32_thread_detach_np ();
  terminate ();
}

#endif

#if ! defined (__MINGW32__) || (defined (__MSVCRT__) && ! defined (__DMC__))
unsigned
  __stdcall
#else
void
#endif
ptw32_threadStart (void *vthreadParms)
{
  ThreadParms * threadParms = (ThreadParms *) vthreadParms;
  pthread_t self;
  ptw32_thread_t * sp;
  void *(*start) (void *);
  void * arg;

#ifdef __CLEANUP_SEH
  DWORD
  ei[] = { 0, 0, 0 };
#endif

#ifdef __CLEANUP_C
  int setjmp_rc;
#endif

  void * status = (void *) 0;

  self = threadParms->tid;
  sp = (ptw32_thread_t *) self.p;
  start = threadParms->start;
  arg = threadParms->arg;

  free (threadParms);

#if defined (__MINGW32__) && ! defined (__MSVCRT__)
  /*
   * beginthread does not return the thread id and is running
   * before it returns us the thread handle, and so we do it here.
   */
  sp->thread = GetCurrentThreadId ();
  /*
   * Here we're using cancelLock as a general-purpose lock
   * to make the new thread wait until the creating thread
   * has the new handle.
   */
  if (pthread_mutex_lock (&sp->cancelLock) == 0)
    {
      (void) pthread_mutex_unlock (&sp->cancelLock);
    }
#endif

  pthread_setspecific (ptw32_selfThreadKey, sp);

  sp->state = PThreadStateRunning;

#ifdef __CLEANUP_SEH

  __try
  {
    /*
     * Run the caller's routine;
     */
    status = sp->exitStatus = (*start) (arg);

#ifdef _UWIN
    if (--pthread_count <= 0)
      exit (0);
#endif

  }
  __except (ExceptionFilter (GetExceptionInformation (), ei))
  {
    switch (ei[0])
      {
      case PTW32_EPS_CANCEL:
	status = sp->exitStatus = PTHREAD_CANCELED;
#ifdef _UWIN
	if (--pthread_count <= 0)
	  exit (0);
#endif
	break;
      case PTW32_EPS_EXIT:
	status = sp->exitStatus;
	break;
      default:
	status = sp->exitStatus = PTHREAD_CANCELED;
	break;
      }
  }

#else /* __CLEANUP_SEH */

#ifdef __CLEANUP_C

  setjmp_rc = setjmp (sp->start_mark);

  if (0 == setjmp_rc)
    {

      /*
       * Run the caller's routine;
       */
      status = sp->exitStatus = (*start) (arg);
    }
  else
    {
      switch (setjmp_rc)
	{
	case PTW32_EPS_CANCEL:
	  status = sp->exitStatus = PTHREAD_CANCELED;
	  break;
	case PTW32_EPS_EXIT:
	  status = sp->exitStatus;
	  break;
	default:
	  status = sp->exitStatus = PTHREAD_CANCELED;
	  break;
	}
    }

#else /* __CLEANUP_C */

#ifdef __CLEANUP_CXX

  ptw32_oldTerminate = set_terminate (&ptw32_terminate);

  try
  {
    /*
     * Run the caller's routine in a nested try block so that we
     * can run the user's terminate function, which may call
     * pthread_exit() or be canceled.
     */
    try
    {
      status = sp->exitStatus = (*start) (arg);
    }
    catch (ptw32_exception &)
    {
      /*
       * Pass these through to the outer block.
       */
      throw;
    }
    catch (...)
    {
      /*
       * We want to run the user's terminate function if supplied.
       * That function may call pthread_exit() or be canceled, which will
       * be handled by the outer try block.
       *
       * ptw32_terminate() will be called if there is no user
       * supplied function.
       */

      terminate_function
	term_func = set_terminate (0);
      set_terminate (term_func);

      if (term_func != 0)
	{
	  term_func ();
	}

      throw;
    }
  }
  catch (ptw32_exception_cancel &)
  {
    /*
     * Thread was canceled.
     */
    status = sp->exitStatus = PTHREAD_CANCELED;
  }
  catch (ptw32_exception_exit &)
  {
    /*
     * Thread was exited via pthread_exit().
     */
    status = sp->exitStatus;
  }
  catch (...)
  {
    /*
     * A system unexpected exception has occurred running the user's
     * terminate routine. We get control back within this block - cleanup
     * and release the exception out of thread scope.
     */
    status = sp->exitStatus = PTHREAD_CANCELED;
    (void) pthread_mutex_lock (&sp->cancelLock);
    sp->state = PThreadStateException;
    (void) pthread_mutex_unlock (&sp->cancelLock);
    (void) pthread_win32_thread_detach_np ();
    (void) set_terminate (ptw32_oldTerminate);
    throw;

    /*
     * Never reached.
     */
  }

  (void) set_terminate (ptw32_oldTerminate);

#else

#error ERROR [__FILE__, line __LINE__]: Cleanup type undefined.

#endif /* __CLEANUP_CXX */
#endif /* __CLEANUP_C */
#endif /* __CLEANUP_SEH */

#if defined(PTW32_STATIC_LIB)
  /*
   * We need to cleanup the pthread now if we have
   * been statically linked, in which case the cleanup
   * in dllMain won't get done. Joinable threads will
   * only be partially cleaned up and must be fully cleaned
   * up by pthread_join() or pthread_detach().
   *
   * Note: if this library has been statically linked,
   * implicitly created pthreads (those created
   * for Win32 threads which have called pthreads routines)
   * must be cleaned up explicitly by the application
   * (by calling pthread_win32_thread_detach_np()).
   * For the dll, dllMain will do the cleanup automatically.
   */
  (void) pthread_win32_thread_detach_np ();
#endif

#ifdef WINCE
  #define _endthreadex ExitThread
#endif

#if ! defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__) 
  _endthreadex ((unsigned) status);
#else
  _endthread ();
#endif

  /*
   * Never reached.
   */

#if ! defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
  return (unsigned) status;
#endif

}				/* ptw32_threadStart */
// ===========================SOURCE FILE BREAK===============================
void
ptw32_threadDestroy (pthread_t thread)
{
  ptw32_thread_t * tp = (ptw32_thread_t *) thread.p;
  ptw32_thread_t threadCopy;

  if (tp != NULL)
    {
      /*
       * Copy thread state so that the thread can be atomically NULLed.
       */
      memcpy (&threadCopy, tp, sizeof (threadCopy));

      /*
       * Thread ID structs are never freed. They're NULLed and reused.
       * This also sets the thread to PThreadStateInitial (invalid).
       */
      ptw32_threadReusePush (thread);

      /* Now work on the copy. */
      if (threadCopy.cancelEvent != NULL)
	{
	  CloseHandle (threadCopy.cancelEvent);
	}

      (void) pthread_mutex_destroy(&threadCopy.cancelLock);
      (void) pthread_mutex_destroy(&threadCopy.threadLock);

#if ! defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
      /*
       * See documentation for endthread vs endthreadex.
       */
      if (threadCopy.threadH != 0)
	{
	  CloseHandle (threadCopy.threadH);
	}
#endif

    }
}				/* ptw32_threadDestroy */
// ===========================SOURCE FILE BREAK===============================
int
ptw32_tkAssocCreate (ptw32_thread_t * sp, pthread_key_t key)
     /*
      * -------------------------------------------------------------------
      * This routine creates an association that
      * is unique for the given (thread,key) combination.The association 
      * is referenced by both the thread and the key.
      * This association allows us to determine what keys the
      * current thread references and what threads a given key
      * references.
      * See the detailed description
      * at the beginning of this file for further details.
      *
      * Notes:
      *      1)      New associations are pushed to the beginning of the
      *              chain so that the internal ptw32_selfThreadKey association
      *              is always last, thus allowing selfThreadExit to
      *              be implicitly called last by pthread_exit.
      *      2)      
      *
      * Parameters:
      *              thread
      *                      current running thread.
      *              key
      *                      key on which to create an association.
      * Returns:
      *       0              - if successful,
      *       ENOMEM         - not enough memory to create assoc or other object
      *       EINVAL         - an internal error occurred
      *       ENOSYS         - an internal error occurred
      * -------------------------------------------------------------------
      */
{
  ThreadKeyAssoc *assoc;

  /*
   * Have to create an association and add it
   * to both the key and the thread.
   *
   * Both key->keyLock and thread->threadLock are locked on
   * entry to this routine.
   */
  assoc = (ThreadKeyAssoc *) calloc (1, sizeof (*assoc));

  if (assoc == NULL)
    {
      return ENOMEM;
    }

  assoc->thread = sp;
  assoc->key = key;

  /*
   * Register assoc with key
   */
  assoc->prevThread = NULL;
  assoc->nextThread = (ThreadKeyAssoc *) key->threads;
  if (assoc->nextThread != NULL)
    {
      assoc->nextThread->prevThread = assoc;
    }
  key->threads = (void *) assoc;

  /*
   * Register assoc with thread
   */
  assoc->prevKey = NULL;
  assoc->nextKey = (ThreadKeyAssoc *) sp->keys;
  if (assoc->nextKey != NULL)
    {
      assoc->nextKey->prevKey = assoc;
    }
  sp->keys = (void *) assoc;

  return (0);

}				/* ptw32_tkAssocCreate */
// ===========================SOURCE FILE BREAK===============================
void
ptw32_tkAssocDestroy (ThreadKeyAssoc * assoc)
     /*
      * -------------------------------------------------------------------
      * This routine releases all resources for the given ThreadKeyAssoc
      * once it is no longer being referenced
      * ie) either the key or thread has stopped referencing it.
      *
      * Parameters:
      *              assoc
      *                      an instance of ThreadKeyAssoc.
      * Returns:
      *      N/A
      * -------------------------------------------------------------------
      */
{

  /*
   * Both key->keyLock and thread->threadLock are locked on
   * entry to this routine.
   */
  if (assoc != NULL)
    {
      ThreadKeyAssoc * prev, * next;

      /* Remove assoc from thread's keys chain */
      prev = assoc->prevKey;
      next = assoc->nextKey;
      if (prev != NULL)
	{
	  prev->nextKey = next;
	}
      if (next != NULL)
	{
	  next->prevKey = prev;
	}

      if (assoc->thread->keys == assoc)
	{
	  /* We're at the head of the thread's keys chain */
	  assoc->thread->keys = next;
	}
      if (assoc->thread->nextAssoc == assoc)
	{
	  /*
	   * Thread is exiting and we're deleting the assoc to be processed next.
	   * Hand thread the assoc after this one.
	   */
	  assoc->thread->nextAssoc = next;
	}

      /* Remove assoc from key's threads chain */
      prev = assoc->prevThread;
      next = assoc->nextThread;
      if (prev != NULL)
	{
	  prev->nextThread = next;
	}
      if (next != NULL)
	{
	  next->prevThread = prev;
	}

      if (assoc->key->threads == assoc)
	{
	  /* We're at the head of the key's threads chain */
	  assoc->key->threads = next;
	}

      free (assoc);
    }

}				/* ptw32_tkAssocDestroy */
// ===========================SOURCE FILE BREAK===============================
#ifdef _WIN32
 #ifndef WINCE

  #ifdef BORLANDC
	#include <exception>
  #else
    #include <eh.h>
  #endif

 #endif
#endif

#ifdef __cplusplus
# if ! defined (_MSC_VER) && ! (defined(__GNUC__) && __GNUC__ < 3) && ! defined(__WATCOMC__)
using
  std::terminate;
# endif
#endif

void
ptw32_callUserDestroyRoutines (pthread_t thread)
     /*
      * -------------------------------------------------------------------
      * DOCPRIVATE
      *
      * This the routine runs through all thread keys and calls
      * the destroy routines on the user's data for the current thread.
      * It simulates the behaviour of POSIX Threads.
      *
      * PARAMETERS
      *              thread
      *                      an instance of pthread_t
      *
      * RETURNS
      *              N/A
      * -------------------------------------------------------------------
      */
{
  ThreadKeyAssoc * assoc;

  if (thread.p != NULL)
    {
      int assocsRemaining;
      int iterations = 0;
      ptw32_thread_t * sp = (ptw32_thread_t *) thread.p;

      /*
       * Run through all Thread<-->Key associations
       * for the current thread.
       *
       * Do this process at most PTHREAD_DESTRUCTOR_ITERATIONS times.
       */
      do
	{
	  assocsRemaining = 0;
	  iterations++;

	  (void) pthread_mutex_lock(&(sp->threadLock));
	  /*
	   * The pointer to the next assoc is stored in the thread struct so that
	   * the assoc destructor in pthread_key_delete can adjust it
	   * if it deletes this assoc. This can happen if we fail to acquire
	   * both locks below, and are forced to release all of our locks,
	   * leaving open the opportunity for pthread_key_delete to get in
	   * before us.
	   */
	  sp->nextAssoc = sp->keys;
	  (void) pthread_mutex_unlock(&(sp->threadLock));

	  for (;;)
	    {
	      void * value;
	      pthread_key_t k;
	      void (*destructor) (void *);

	      /*
	       * First we need to serialise with pthread_key_delete by locking
	       * both assoc guards, but in the reverse order to our convention,
	       * so we must be careful to avoid deadlock.
	       */
	      (void) pthread_mutex_lock(&(sp->threadLock));

	      if ((assoc = (ThreadKeyAssoc *)sp->nextAssoc) == NULL)
		{
		  /* Finished */
		  pthread_mutex_unlock(&(sp->threadLock));
		  break;
		}
	      else
		{
		  /*
		   * assoc->key must be valid because assoc can't change or be
		   * removed from our chain while we hold at least one lock. If
		   * the assoc was on our key chain then the key has not been
		   * deleted yet.
		   *
		   * Now try to acquire the second lock without deadlocking.
		   * If we fail, we need to relinquish the first lock and the
		   * processor and then try to acquire them all again.
		   */
		  if (pthread_mutex_trylock(&(assoc->key->keyLock)) == EBUSY)
		    {
		      pthread_mutex_unlock(&(sp->threadLock));
		      Sleep(1); // Ugly but necessary to avoid priority effects.
		      /*
		       * Go around again.
		       * If pthread_key_delete has removed this assoc in the meantime,
		       * sp->nextAssoc will point to a new assoc.
		       */
		      continue;
		    }
		}

	      /* We now hold both locks */

	      sp->nextAssoc = assoc->nextKey;

	      /*
	       * Key still active; pthread_key_delete
	       * will block on these same mutexes before
	       * it can release actual key; therefore,
	       * key is valid and we can call the destroy
	       * routine;
	       */
	      k = assoc->key;
	      destructor = k->destructor;
	      value = TlsGetValue(k->key);
	      TlsSetValue (k->key, NULL);

	      // Every assoc->key exists and has a destructor
	      if (value != NULL && iterations <= PTHREAD_DESTRUCTOR_ITERATIONS)
		{
		  /*
		   * Unlock both locks before the destructor runs.
		   * POSIX says pthread_key_delete can be run from destructors,
		   * and that probably includes with this key as target.
		   * pthread_setspecific can also be run from destructors and
		   * also needs to be able to access the assocs.
		   */
		  (void) pthread_mutex_unlock(&(sp->threadLock));
		  (void) pthread_mutex_unlock(&(k->keyLock));

		  assocsRemaining++;

#ifdef __cplusplus

		  try
		    {
		      /*
		       * Run the caller's cleanup routine.
		       */
		      destructor (value);
		    }
		  catch (...)
		    {
		      /*
		       * A system unexpected exception has occurred
		       * running the user's destructor.
		       * We get control back within this block in case
		       * the application has set up it's own terminate
		       * handler. Since we are leaving the thread we
		       * should not get any internal pthreads
		       * exceptions.
		       */
#ifndef WINCE
				terminate ();
#endif
		    }

#else /* __cplusplus */

		  /*
		   * Run the caller's cleanup routine.
		   */
		  destructor (value);

#endif /* __cplusplus */

		}
	      else
		{
		  /*
		   * Remove association from both the key and thread chains
		   * and reclaim it's memory resources.
		   */
		  ptw32_tkAssocDestroy (assoc);
		  (void) pthread_mutex_unlock(&(sp->threadLock));
		  (void) pthread_mutex_unlock(&(k->keyLock));
		}
	    }
	}
      while (assocsRemaining);
    }
}				/* ptw32_callUserDestroyRoutines */

// ===========================SOURCE FILE BREAK===============================
int
ptw32_semwait (sem_t * sem)
     /*
      * ------------------------------------------------------
      * DESCRIPTION
      *      This function waits on a POSIX semaphore. If the
      *      semaphore value is greater than zero, it decreases
      *      its value by one. If the semaphore value is zero, then
      *      the calling thread (or process) is blocked until it can
      *      successfully decrease the value.
      *
      *      Unlike sem_wait(), this routine is non-cancelable.
      *
      * RESULTS
      *              0               successfully decreased semaphore,
      *              -1              failed, error in errno.
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *              EINTR           the function was interrupted by a signal,
      *              EDEADLK         a deadlock condition was detected.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  sem_t s = *sem;

  if (s == NULL)
    {
      result = EINVAL;
    }
  else
    {
      if ((result = pthread_mutex_lock (&s->lock)) == 0)
        {
          int v = --s->value;

          (void) pthread_mutex_unlock (&s->lock);

          if (v < 0)
            {
              /* Must wait */
              if (WaitForSingleObject (s->sem, INFINITE) == WAIT_OBJECT_0)
		{
#ifdef NEED_SEM
		  if (pthread_mutex_lock (&s->lock) == 0)
		    {
		      if (s->leftToUnblock > 0)
			{
			  --s->leftToUnblock;
			  SetEvent(s->sem);
			}
		      (void) pthread_mutex_unlock (&s->lock);
		    }
#endif
		  return 0;
		}
            }
          else
	    {
	      return 0;
	    }
        }
    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  return 0;

}				/* ptw32_semwait */
// ===========================SOURCE FILE BREAK===============================
#ifdef NEED_FTIME

/*
 * time between jan 1, 1601 and jan 1, 1970 in units of 100 nanoseconds
 */
#define PTW32_TIMESPEC_TO_FILETIME_OFFSET \
	  ( ((LONGLONG) 27111902 << 32) + (LONGLONG) 3577643008 )

INLINE void
ptw32_timespec_to_filetime (const struct timespec *ts, FILETIME * ft)
     /*
      * -------------------------------------------------------------------
      * converts struct timespec
      * where the time is expressed in seconds and nanoseconds from Jan 1, 1970.
      * into FILETIME (as set by GetSystemTimeAsFileTime), where the time is
      * expressed in 100 nanoseconds from Jan 1, 1601,
      * -------------------------------------------------------------------
      */
{
  *(LONGLONG *) ft = ts->tv_sec * 10000000
    + (ts->tv_nsec + 50) / 100 + PTW32_TIMESPEC_TO_FILETIME_OFFSET;
}

INLINE void
ptw32_filetime_to_timespec (const FILETIME * ft, struct timespec *ts)
     /*
      * -------------------------------------------------------------------
      * converts FILETIME (as set by GetSystemTimeAsFileTime), where the time is
      * expressed in 100 nanoseconds from Jan 1, 1601,
      * into struct timespec
      * where the time is expressed in seconds and nanoseconds from Jan 1, 1970.
      * -------------------------------------------------------------------
      */
{
  ts->tv_sec =
    (int) ((*(LONGLONG *) ft - PTW32_TIMESPEC_TO_FILETIME_OFFSET) / 10000000);
  ts->tv_nsec =
    (int) ((*(LONGLONG *) ft - PTW32_TIMESPEC_TO_FILETIME_OFFSET -
	    ((LONGLONG) ts->tv_sec * (LONGLONG) 10000000)) * 100);
}

#endif /* NEED_FTIME */
// ===========================SOURCE FILE BREAK===============================
#ifndef NEED_FTIME
#ifndef WINCE
#include <sys/timeb.h>
#endif
#endif


INLINE DWORD
ptw32_relmillisecs (const struct timespec * abstime)
{
  const int64_t NANOSEC_PER_MILLISEC = 1000000;
  const int64_t MILLISEC_PER_SEC = 1000;
  DWORD milliseconds;
  int64_t tmpAbsMilliseconds;
  int64_t tmpCurrMilliseconds;
#ifdef NEED_FTIME
  struct timespec currSysTime;
  FILETIME ft;
  SYSTEMTIME st;
#else /* ! NEED_FTIME */
  struct _timeb currSysTime;
#endif /* NEED_FTIME */


  /* 
   * Calculate timeout as milliseconds from current system time. 
   */

  /*
   * subtract current system time from abstime in a way that checks
   * that abstime is never in the past, or is never equivalent to the
   * defined INFINITE value (0xFFFFFFFF).
   *
   * Assume all integers are unsigned, i.e. cannot test if less than 0.
   */
  tmpAbsMilliseconds =  (int64_t)abstime->tv_sec * MILLISEC_PER_SEC;
  tmpAbsMilliseconds += ((int64_t)abstime->tv_nsec + (NANOSEC_PER_MILLISEC/2)) / NANOSEC_PER_MILLISEC;

  /* get current system time */

#ifdef NEED_FTIME

  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &ft);
  /*
   * GetSystemTimeAsFileTime(&ft); would be faster,
   * but it does not exist on WinCE
   */

  ptw32_filetime_to_timespec(&ft, &currSysTime);

  tmpCurrMilliseconds = (int64_t)currSysTime.tv_sec * MILLISEC_PER_SEC;
  tmpCurrMilliseconds += ((int64_t)currSysTime.tv_nsec + (NANOSEC_PER_MILLISEC/2))
			   / NANOSEC_PER_MILLISEC;

#else /* ! NEED_FTIME */

  _ftime(&currSysTime);

  tmpCurrMilliseconds = (int64_t) currSysTime.time * MILLISEC_PER_SEC;
  tmpCurrMilliseconds += (int64_t) currSysTime.millitm;

#endif /* NEED_FTIME */

  if (tmpAbsMilliseconds > tmpCurrMilliseconds)
    {
      milliseconds = (DWORD) (tmpAbsMilliseconds - tmpCurrMilliseconds);
      if (milliseconds == INFINITE)
        {
          /* Timeouts must be finite */
          milliseconds--;
        }
    }
  else
    {
      /* The abstime given is in the past */
      milliseconds = 0;
    }

  return milliseconds;
}

// ===========================SOURCE FILE BREAK===============================

/*
 * ptw32_throw
 *
 * All canceled and explicitly exited POSIX threads go through
 * here. This routine knows how to exit both POSIX initiated threads and
 * 'implicit' POSIX threads for each of the possible language modes (C,
 * C++, and SEH).
 */
void
ptw32_throw (DWORD exception)
{
  /*
   * Don't use pthread_self() to avoid creating an implicit POSIX thread handle
   * unnecessarily.
   */
  ptw32_thread_t * sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

#ifdef __CLEANUP_SEH
  DWORD exceptionInformation[3];
#endif

  if (exception != PTW32_EPS_CANCEL && exception != PTW32_EPS_EXIT)
    {
      /* Should never enter here */
      exit (1);
    }

  if (NULL == sp || sp->implicit)
    {
      /*
       * We're inside a non-POSIX initialised Win32 thread
       * so there is no point to jump or throw back to. Just do an
       * explicit thread exit here after cleaning up POSIX
       * residue (i.e. cleanup handlers, POSIX thread handle etc).
       */
      unsigned exitCode = 0;

      switch (exception)
	{
	case PTW32_EPS_CANCEL:
	  exitCode = (unsigned) PTHREAD_CANCELED;
	  break;
	case PTW32_EPS_EXIT:
	  exitCode = (unsigned) sp->exitStatus;;
	  break;
	}

#if defined(PTW32_STATIC_LIB)

      pthread_win32_thread_detach_np ();

#endif

#if ! defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
      _endthreadex (exitCode);
#else
      _endthread ();
#endif

    }

#ifdef __CLEANUP_SEH


  exceptionInformation[0] = (DWORD) (exception);
  exceptionInformation[1] = (DWORD) (0);
  exceptionInformation[2] = (DWORD) (0);

  RaiseException (EXCEPTION_PTW32_SERVICES, 0, 3, exceptionInformation);

#else /* __CLEANUP_SEH */

#ifdef __CLEANUP_C

  ptw32_pop_cleanup_all (1);
  longjmp (sp->start_mark, exception);

#else /* __CLEANUP_C */

#ifdef __CLEANUP_CXX

  switch (exception)
    {
    case PTW32_EPS_CANCEL:
      throw ptw32_exception_cancel ();
      break;
    case PTW32_EPS_EXIT:
      throw ptw32_exception_exit ();
      break;
    }

#else

#error ERROR [__FILE__, line __LINE__]: Cleanup type undefined.

#endif /* __CLEANUP_CXX */

#endif /* __CLEANUP_C */

#endif /* __CLEANUP_SEH */

  /* Never reached */
}


void
ptw32_pop_cleanup_all (int execute)
{
  while (NULL != ptw32_pop_cleanup (execute))
    {
    }
}


DWORD
ptw32_get_exception_services_code (void)
{
#ifdef __CLEANUP_SEH

  return EXCEPTION_PTW32_SERVICES;

#else

  return (DWORD) NULL;

#endif
}
// ===========================SOURCE FILE BREAK===============================
/*
 * ptw32_getprocessors()
 *
 * Get the number of CPUs available to the process.
 *
 * If the available number of CPUs is 1 then pthread_spin_lock()
 * will block rather than spin if the lock is already owned.
 *
 * pthread_spin_init() calls this routine when initialising
 * a spinlock. If the number of available processors changes
 * (after a call to SetProcessAffinityMask()) then only
 * newly initialised spinlocks will notice.
 */
int
ptw32_getprocessors (int *count)
{
  int result = 0;
#if defined(NEED_PROCESS_AFFINITY_MASK)

  *count = 1;

#else
  DWORD_PTR vProcessCPUs;
  DWORD_PTR vSystemCPUs;

  if (GetProcessAffinityMask (GetCurrentProcess (),
			      &vProcessCPUs, &vSystemCPUs))
    {
      DWORD_PTR bit;
      int CPUs = 0;

      for (bit = 1; bit != 0; bit <<= 1)
	{
	  if (vProcessCPUs & bit)
	    {
	      CPUs++;
	    }
	}
      *count = CPUs;
    }
  else
    {
      result = EAGAIN;
    }

#endif

  return (result);
}
// ===========================SOURCE FILE BREAK===============================


//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "attr.c"
//
//#include "pthread_attr_init.c"
//#include "pthread_attr_destroy.c"
//#include "pthread_attr_getdetachstate.c"
//#include "pthread_attr_setdetachstate.c"
//#include "pthread_attr_getstackaddr.c"
//#include "pthread_attr_setstackaddr.c"
//#include "pthread_attr_getstacksize.c"
//#include "pthread_attr_setstacksize.c"
//#include "pthread_attr_getscope.c"
//#include "pthread_attr_setscope.c"


int
pthread_attr_init (pthread_attr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Initializes a thread attributes object with default
      *      attributes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *
      * DESCRIPTION
      *      Initializes a thread attributes object with default
      *      attributes.
      *
      *      NOTES:
      *              1)      Used to define thread attributes
      *
      * RESULTS
      *              0               successfully initialized attr,
      *              ENOMEM          insufficient memory for attr.
      *
      * ------------------------------------------------------
      */
{
  pthread_attr_t attr_result;

  if (attr == NULL)
    {
      /* This is disallowed. */
      return EINVAL;
    }

  attr_result = (pthread_attr_t) malloc (sizeof (*attr_result));

  if (attr_result == NULL)
    {
      return ENOMEM;
    }

#ifdef _POSIX_THREAD_ATTR_STACKSIZE
  /*
   * Default to zero size. Unless changed explicitly this
   * will allow Win32 to set the size to that of the
   * main thread.
   */
  attr_result->stacksize = 0;
#endif

#ifdef _POSIX_THREAD_ATTR_STACKADDR
  /* FIXME: Set this to something sensible when we support it. */
  attr_result->stackaddr = NULL;
#endif

  attr_result->detachstate = PTHREAD_CREATE_JOINABLE;

#if HAVE_SIGSET_T
  memset (&(attr_result->sigmask), 0, sizeof (sigset_t));
#endif /* HAVE_SIGSET_T */

  /*
   * Win32 sets new threads to THREAD_PRIORITY_NORMAL and
   * not to that of the parent thread. We choose to default to
   * this arrangement.
   */
  attr_result->param.sched_priority = THREAD_PRIORITY_NORMAL;
  attr_result->inheritsched = PTHREAD_EXPLICIT_SCHED;
  attr_result->contentionscope = PTHREAD_SCOPE_SYSTEM;

  attr_result->valid = PTW32_ATTR_VALID;

  *attr = attr_result;

  return 0;
}
// ===========================SOURCE FILE BREAK===============================
int
pthread_attr_destroy (pthread_attr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Destroys a thread attributes object.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *
      * DESCRIPTION
      *      Destroys a thread attributes object.
      *
      *      NOTES:
      *              1)      Does not affect threads created with 'attr'.
      *
      * RESULTS
      *              0               successfully destroyed attr,
      *              EINVAL          'attr' is invalid.
      *
      * ------------------------------------------------------
      */
{
  if (ptw32_is_attr (attr) != 0)
    {
      return EINVAL;
    }

  /*
   * Set the attribute object to a specific invalid value.
   */
  (*attr)->valid = 0;
  free (*attr);
  *attr = NULL;

  return 0;
}
// ===========================SOURCE FILE BREAK===============================
int
pthread_attr_getdetachstate (const pthread_attr_t * attr, int *detachstate)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function determines whether threads created with
      *      'attr' will run detached.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *      detachstate
      *              pointer to an integer into which is returned one
      *              of:
      *
      *              PTHREAD_CREATE_JOINABLE
      *                              Thread ID is valid, must be joined
      *
      *              PTHREAD_CREATE_DETACHED
      *                              Thread ID is invalid, cannot be joined,
      *                              canceled, or modified
      *
      *
      * DESCRIPTION
      *      This function determines whether threads created with
      *      'attr' will run detached.
      *
      *      NOTES:
      *              1)      You cannot join or cancel detached threads.
      *
      * RESULTS
      *              0               successfully retrieved detach state,
      *              EINVAL          'attr' is invalid
      *
      * ------------------------------------------------------
      */
{
  if (ptw32_is_attr (attr) != 0 || detachstate == NULL)
    {
      *detachstate = PTHREAD_CREATE_DETACHED;
      return EINVAL;
    }

  *detachstate = (*attr)->detachstate;
  return 0;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_attr_setdetachstate (pthread_attr_t * attr, int detachstate)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function specifies whether threads created with
      *      'attr' will run detached.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *      detachstate
      *              an integer containing one of:
      *
      *              PTHREAD_CREATE_JOINABLE
      *                              Thread ID is valid, must be joined
      *
      *              PTHREAD_CREATE_DETACHED
      *                              Thread ID is invalid, cannot be joined,
      *                              canceled, or modified
      *
      *
      * DESCRIPTION
      *      This function specifies whether threads created with
      *      'attr' will run detached.
      *
      *      NOTES:
      *              1)      You cannot join or cancel detached threads.
      *
      * RESULTS
      *              0               successfully set detach state,
      *              EINVAL          'attr' or 'detachstate' is invalid
      *
      * ------------------------------------------------------
      */
{
  if (ptw32_is_attr (attr) != 0)
    {
      return EINVAL;
    }

  if (detachstate != PTHREAD_CREATE_JOINABLE &&
      detachstate != PTHREAD_CREATE_DETACHED)
    {
      return EINVAL;
    }

  (*attr)->detachstate = detachstate;
  return 0;
}
// ===========================SOURCE FILE BREAK===============================
/* ignore warning "unreferenced formal parameter" */
#ifdef _MSC_VER
#pragma warning( disable : 4100 )
#endif

int
pthread_attr_getstackaddr (const pthread_attr_t * attr, void **stackaddr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function determines the address of the stack
      *      on which threads created with 'attr' will run.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *      stackaddr
      *              pointer into which is returned the stack address.
      *
      *
      * DESCRIPTION
      *      This function determines the address of the stack
      *      on which threads created with 'attr' will run.
      *
      *      NOTES:
      *              1)      Function supported only if this macro is
      *                      defined:
      *
      *                              _POSIX_THREAD_ATTR_STACKADDR
      *
      *              2)      Create only one thread for each stack
      *                      address..
      *
      * RESULTS
      *              0               successfully retreived stack address,
      *              EINVAL          'attr' is invalid
      *              ENOSYS          function not supported
      *
      * ------------------------------------------------------
      */
{
#if defined( _POSIX_THREAD_ATTR_STACKADDR )

  if (ptw32_is_attr (attr) != 0)
    {
      return EINVAL;
    }

  *stackaddr = (*attr)->stackaddr;
  return 0;

#else

  return ENOSYS;

#endif /* _POSIX_THREAD_ATTR_STACKADDR */
}
// ===========================SOURCE FILE BREAK===============================
int
pthread_attr_setstackaddr (pthread_attr_t * attr, void *stackaddr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Threads created with 'attr' will run on the stack
      *      starting at 'stackaddr'.
      *      Stack must be at least PTHREAD_STACK_MIN bytes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *      stacksize
      *              stack size, in bytes.
      *
      *
      * DESCRIPTION
      *      Threads created with 'attr' will run on the stack
      *      starting at 'stackaddr'.
      *      Stack must be at least PTHREAD_STACK_MIN bytes.
      *
      *      NOTES:
      *              1)      Function supported only if this macro is
      *                      defined:
      *
      *                              _POSIX_THREAD_ATTR_STACKADDR
      *
      *              2)      Create only one thread for each stack
      *                      address..
      *
      *              3)      Ensure that stackaddr is aligned.
      *
      * RESULTS
      *              0               successfully set stack address,
      *              EINVAL          'attr' is invalid
      *              ENOSYS          function not supported
      *
      * ------------------------------------------------------
      */
{
#if defined( _POSIX_THREAD_ATTR_STACKADDR )

  if (ptw32_is_attr (attr) != 0)
    {
      return EINVAL;
    }

  (*attr)->stackaddr = stackaddr;
  return 0;

#else

  return ENOSYS;

#endif /* _POSIX_THREAD_ATTR_STACKADDR */
}
// ===========================SOURCE FILE BREAK===============================

/* ignore warning "unreferenced formal parameter" */
#ifdef _MSC_VER
#pragma warning( disable : 4100 )
#endif

int
pthread_attr_getstacksize (const pthread_attr_t * attr, size_t * stacksize)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function determines the size of the stack on
      *      which threads created with 'attr' will run.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *      stacksize
      *              pointer to size_t into which is returned the
      *              stack size, in bytes.
      *
      *
      * DESCRIPTION
      *      This function determines the size of the stack on
      *      which threads created with 'attr' will run.
      *
      *      NOTES:
      *              1)      Function supported only if this macro is
      *                      defined:
      *
      *                              _POSIX_THREAD_ATTR_STACKSIZE
      *
      *              2)      Use on newly created attributes object to
      *                      find the default stack size.
      *
      * RESULTS
      *              0               successfully retrieved stack size,
      *              EINVAL          'attr' is invalid
      *              ENOSYS          function not supported
      *
      * ------------------------------------------------------
      */
{
#ifdef _POSIX_THREAD_ATTR_STACKSIZE

  if (ptw32_is_attr (attr) != 0)
    {
      return EINVAL;
    }

  /* Everything is okay. */
  *stacksize = (*attr)->stacksize;
  return 0;

#else

  return ENOSYS;

#endif /* _POSIX_THREAD_ATTR_STACKSIZE */

}
// ===========================SOURCE FILE BREAK===============================
int
pthread_attr_setstacksize (pthread_attr_t * attr, size_t stacksize)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function specifies the size of the stack on
      *      which threads created with 'attr' will run.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_attr_t
      *
      *      stacksize
      *              stack size, in bytes.
      *
      *
      * DESCRIPTION
      *      This function specifies the size of the stack on
      *      which threads created with 'attr' will run.
      *
      *      NOTES:
      *              1)      Function supported only if this macro is
      *                      defined:
      *
      *                              _POSIX_THREAD_ATTR_STACKSIZE
      *
      *              2)      Find the default first (using
      *                      pthread_attr_getstacksize), then increase
      *                      by multiplying.
      *
      *              3)      Only use if thread needs more than the
      *                      default.
      *
      * RESULTS
      *              0               successfully set stack size,
      *              EINVAL          'attr' is invalid or stacksize too
      *                              small or too big.
      *              ENOSYS          function not supported
      *
      * ------------------------------------------------------
      */
{
#ifdef _POSIX_THREAD_ATTR_STACKSIZE

#if PTHREAD_STACK_MIN > 0

  /*  Verify that the stack size is within range. */
  if (stacksize < PTHREAD_STACK_MIN)
    {
      return EINVAL;
    }

#endif

  if (ptw32_is_attr (attr) != 0)
    {
      return EINVAL;
    }

  /* Everything is okay. */
  (*attr)->stacksize = stacksize;
  return 0;

#else

  return ENOSYS;

#endif /* _POSIX_THREAD_ATTR_STACKSIZE */

}
// ===========================SOURCE FILE BREAK===============================
/* ignore warning "unreferenced formal parameter" */
#ifdef _MSC_VER
#pragma warning( disable : 4100 )
#endif

int
pthread_attr_getscope (const pthread_attr_t * attr, int *contentionscope)
{
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
  *contentionscope = (*attr)->contentionscope;
  return 0;
#else
  return ENOSYS;
#endif
}
// ===========================SOURCE FILE BREAK===============================

/* ignore warning "unreferenced formal parameter" */
#ifdef _MSC_VER
#pragma warning( disable : 4100 )
#endif

int
pthread_attr_setscope (pthread_attr_t * attr, int contentionscope)
{
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
  switch (contentionscope)
    {
    case PTHREAD_SCOPE_SYSTEM:
      (*attr)->contentionscope = contentionscope;
      return 0;
    case PTHREAD_SCOPE_PROCESS:
      return ENOTSUP;
    default:
      return EINVAL;
    }
#else
  return ENOSYS;
#endif
}
// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "barrier.c"
//
//#include "pthread_barrier_init.c"
//#include "pthread_barrier_destroy.c"
//#include "pthread_barrier_wait.c"
//#include "pthread_barrierattr_init.c"
//#include "pthread_barrierattr_destroy.c"
//#include "pthread_barrierattr_getpshared.c"
//#include "pthread_barrierattr_setpshared.c"

int
pthread_barrier_init (pthread_barrier_t * barrier,
		      const pthread_barrierattr_t * attr, unsigned int count)
{
  pthread_barrier_t b;

  if (barrier == NULL || count == 0)
    {
      return EINVAL;
    }

  if (NULL != (b = (pthread_barrier_t) calloc (1, sizeof (*b))))
    {
      b->pshared = (attr != NULL && *attr != NULL
		    ? (*attr)->pshared : PTHREAD_PROCESS_PRIVATE);

      b->nCurrentBarrierHeight = b->nInitialBarrierHeight = count;
      b->iStep = 0;

      /*
       * Two semaphores are used in the same way as two stepping
       * stones might be used in crossing a stream. Once all
       * threads are safely on one stone, the other stone can
       * be moved ahead, and the threads can start moving to it.
       * If some threads decide to eat their lunch before moving
       * then the other threads have to wait.
       */
      if (0 == sem_init (&(b->semBarrierBreeched[0]), b->pshared, 0))
	{
	  if (0 == sem_init (&(b->semBarrierBreeched[1]), b->pshared, 0))
	    {
	      *barrier = b;
	      return 0;
	    }
	  (void) sem_destroy (&(b->semBarrierBreeched[0]));
	}
      (void) free (b);
    }

  return ENOMEM;
}
// ===========================SOURCE FILE BREAK===============================
int
pthread_barrier_destroy (pthread_barrier_t * barrier)
{
  int result = 0;
  pthread_barrier_t b;

  if (barrier == NULL || *barrier == (pthread_barrier_t) PTW32_OBJECT_INVALID)
    {
      return EINVAL;
    }

  b = *barrier;
  *barrier = NULL;

  if (0 == (result = sem_destroy (&(b->semBarrierBreeched[0]))))
    {
      if (0 == (result = sem_destroy (&(b->semBarrierBreeched[1]))))
	{
	  (void) free (b);
	  return 0;
	}
      (void) sem_init (&(b->semBarrierBreeched[0]), b->pshared, 0);
    }

  *barrier = b;
  return (result);
}
// ===========================SOURCE FILE BREAK===============================
int
pthread_barrier_wait (pthread_barrier_t * barrier)
{
  int result;
  int step;
  pthread_barrier_t b;

  if (barrier == NULL || *barrier == (pthread_barrier_t) PTW32_OBJECT_INVALID)
    {
      return EINVAL;
    }

  b = *barrier;
  step = b->iStep;

  if (0 == InterlockedDecrement ((long *) &(b->nCurrentBarrierHeight)))
    {
      /* Must be done before posting the semaphore. */
      b->nCurrentBarrierHeight = b->nInitialBarrierHeight;

      /*
       * There is no race condition between the semaphore wait and post
       * because we are using two alternating semas and all threads have
       * entered barrier_wait and checked nCurrentBarrierHeight before this
       * barrier's sema can be posted. Any threads that have not quite
       * entered sem_wait below when the multiple_post has completed
       * will nevertheless continue through the semaphore (barrier)
       * and will not be left stranded.
       */
      result = (b->nInitialBarrierHeight > 1
		? sem_post_multiple (&(b->semBarrierBreeched[step]),
				     b->nInitialBarrierHeight - 1) : 0);
    }
  else
    {
      /*
       * Use the non-cancelable version of sem_wait().
       */
      result = ptw32_semwait (&(b->semBarrierBreeched[step]));
    }

  /*
   * The first thread across will be the PTHREAD_BARRIER_SERIAL_THREAD.
   * This also sets up the alternate semaphore as the next barrier.
   */
  if (0 == result)
    {
      result = ((PTW32_INTERLOCKED_LONG) step ==
		PTW32_INTERLOCKED_COMPARE_EXCHANGE ((PTW32_INTERLOCKED_LPLONG)
						    & (b->iStep),
						    (PTW32_INTERLOCKED_LONG)
						    (1L - step),
						    (PTW32_INTERLOCKED_LONG)
						    step) ?
		PTHREAD_BARRIER_SERIAL_THREAD : 0);
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================
int
pthread_barrierattr_init (pthread_barrierattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Initializes a barrier attributes object with default
      *      attributes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_barrierattr_t
      *
      *
      * DESCRIPTION
      *      Initializes a barrier attributes object with default
      *      attributes.
      *
      *      NOTES:
      *              1)      Used to define barrier types
      *
      * RESULTS
      *              0               successfully initialized attr,
      *              ENOMEM          insufficient memory for attr.
      *
      * ------------------------------------------------------
      */
{
  pthread_barrierattr_t ba;
  int result = 0;

  ba = (pthread_barrierattr_t) calloc (1, sizeof (*ba));

  if (ba == NULL)
    {
      result = ENOMEM;
    }
  else
    {
      ba->pshared = PTHREAD_PROCESS_PRIVATE;
    }

  *attr = ba;

  return (result);
}				/* pthread_barrierattr_init */

// ===========================SOURCE FILE BREAK===============================
int
pthread_barrierattr_destroy (pthread_barrierattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Destroys a barrier attributes object. The object can
      *      no longer be used.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_barrierattr_t
      *
      *
      * DESCRIPTION
      *      Destroys a barrier attributes object. The object can
      *      no longer be used.
      *
      *      NOTES:
      *              1)      Does not affect barrieres created using 'attr'
      *
      * RESULTS
      *              0               successfully released attr,
      *              EINVAL          'attr' is invalid.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (attr == NULL || *attr == NULL)
    {
      result = EINVAL;
    }
  else
    {
      pthread_barrierattr_t ba = *attr;

      *attr = NULL;
      free (ba);
    }

  return (result);
}				/* pthread_barrierattr_destroy */

// ===========================SOURCE FILE BREAK===============================

int
pthread_barrierattr_getpshared (const pthread_barrierattr_t * attr,
				int *pshared)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Determine whether barriers created with 'attr' can be
      *      shared between processes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_barrierattr_t
      *
      *      pshared
      *              will be set to one of:
      *
      *                      PTHREAD_PROCESS_SHARED
      *                              May be shared if in shared memory
      *
      *                      PTHREAD_PROCESS_PRIVATE
      *                              Cannot be shared.
      *
      *
      * DESCRIPTION
      *      Mutexes creatd with 'attr' can be shared between
      *      processes if pthread_barrier_t variable is allocated
      *      in memory shared by these processes.
      *      NOTES:
      *              1)      pshared barriers MUST be allocated in shared
      *                      memory.
      *              2)      The following macro is defined if shared barriers
      *                      are supported:
      *                              _POSIX_THREAD_PROCESS_SHARED
      *
      * RESULTS
      *              0               successfully retrieved attribute,
      *              EINVAL          'attr' is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result;

  if ((attr != NULL && *attr != NULL) && (pshared != NULL))
    {
      *pshared = (*attr)->pshared;
      result = 0;
    }
  else
    {
      result = EINVAL;
    }

  return (result);
}				/* pthread_barrierattr_getpshared */

// ===========================SOURCE FILE BREAK===============================

int
pthread_barrierattr_setpshared (pthread_barrierattr_t * attr, int pshared)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Barriers created with 'attr' can be shared between
      *      processes if pthread_barrier_t variable is allocated
      *      in memory shared by these processes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_barrierattr_t
      *
      *      pshared
      *              must be one of:
      *
      *                      PTHREAD_PROCESS_SHARED
      *                              May be shared if in shared memory
      *
      *                      PTHREAD_PROCESS_PRIVATE
      *                              Cannot be shared.
      *
      * DESCRIPTION
      *      Mutexes creatd with 'attr' can be shared between
      *      processes if pthread_barrier_t variable is allocated
      *      in memory shared by these processes.
      *
      *      NOTES:
      *              1)      pshared barriers MUST be allocated in shared
      *                      memory.
      *
      *              2)      The following macro is defined if shared barriers
      *                      are supported:
      *                              _POSIX_THREAD_PROCESS_SHARED
      *
      * RESULTS
      *              0               successfully set attribute,
      *              EINVAL          'attr' or pshared is invalid,
      *              ENOSYS          PTHREAD_PROCESS_SHARED not supported,
      *
      * ------------------------------------------------------
      */
{
  int result;

  if ((attr != NULL && *attr != NULL) &&
      ((pshared == PTHREAD_PROCESS_SHARED) ||
       (pshared == PTHREAD_PROCESS_PRIVATE)))
    {
      if (pshared == PTHREAD_PROCESS_SHARED)
	{

#if !defined( _POSIX_THREAD_PROCESS_SHARED )

	  result = ENOSYS;
	  pshared = PTHREAD_PROCESS_PRIVATE;

#else

	  result = 0;

#endif /* _POSIX_THREAD_PROCESS_SHARED */

	}
      else
	{
	  result = 0;
	}

      (*attr)->pshared = pshared;
    }
  else
    {
      result = EINVAL;
    }

  return (result);

}				/* pthread_barrierattr_setpshared */

// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "cancel.c"
//
//#include "pthread_setcancelstate.c"
//#include "pthread_setcanceltype.c"
//#include "pthread_testcancel.c"
//#include "pthread_cancel.c"

int
pthread_setcancelstate (int state, int *oldstate)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function atomically sets the calling thread's
      *      cancelability state to 'state' and returns the previous
      *      cancelability state at the location referenced by
      *      'oldstate'
      *
      * PARAMETERS
      *      state,
      *      oldstate
      *              PTHREAD_CANCEL_ENABLE
      *                      cancellation is enabled,
      *
      *              PTHREAD_CANCEL_DISABLE
      *                      cancellation is disabled
      *
      *
      * DESCRIPTION
      *      This function atomically sets the calling thread's
      *      cancelability state to 'state' and returns the previous
      *      cancelability state at the location referenced by
      *      'oldstate'.
      *
      *      NOTES:
      *      1)      Use to disable cancellation around 'atomic' code that
      *              includes cancellation points
      *
      * COMPATIBILITY ADDITIONS
      *      If 'oldstate' is NULL then the previous state is not returned
      *      but the function still succeeds. (Solaris)
      *
      * RESULTS
      *              0               successfully set cancelability type,
      *              EINVAL          'state' is invalid
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  pthread_t self = pthread_self ();
  ptw32_thread_t * sp = (ptw32_thread_t *) self.p;

  if (sp == NULL
      || (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE))
    {
      return EINVAL;
    }

  /*
   * Lock for async-cancel safety.
   */
  (void) pthread_mutex_lock (&sp->cancelLock);

  if (oldstate != NULL)
    {
      *oldstate = sp->cancelState;
    }

  sp->cancelState = state;

  /*
   * Check if there is a pending asynchronous cancel
   */
  if (state == PTHREAD_CANCEL_ENABLE
      && sp->cancelType == PTHREAD_CANCEL_ASYNCHRONOUS
      && WaitForSingleObject (sp->cancelEvent, 0) == WAIT_OBJECT_0)
    {
      sp->state = PThreadStateCanceling;
      sp->cancelState = PTHREAD_CANCEL_DISABLE;
      ResetEvent (sp->cancelEvent);
      (void) pthread_mutex_unlock (&sp->cancelLock);
      ptw32_throw (PTW32_EPS_CANCEL);

      /* Never reached */
    }

  (void) pthread_mutex_unlock (&sp->cancelLock);

  return (result);

}				/* pthread_setcancelstate */

// ===========================SOURCE FILE BREAK===============================

int
pthread_setcanceltype (int type, int *oldtype)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function atomically sets the calling thread's
      *      cancelability type to 'type' and returns the previous
      *      cancelability type at the location referenced by
      *      'oldtype'
      *
      * PARAMETERS
      *      type,
      *      oldtype
      *              PTHREAD_CANCEL_DEFERRED
      *                      only deferred cancelation is allowed,
      *
      *              PTHREAD_CANCEL_ASYNCHRONOUS
      *                      Asynchronous cancellation is allowed
      *
      *
      * DESCRIPTION
      *      This function atomically sets the calling thread's
      *      cancelability type to 'type' and returns the previous
      *      cancelability type at the location referenced by
      *      'oldtype'
      *
      *      NOTES:
      *      1)      Use with caution; most code is not safe for use
      *              with asynchronous cancelability.
      *
      * COMPATIBILITY ADDITIONS
      *      If 'oldtype' is NULL then the previous type is not returned
      *      but the function still succeeds. (Solaris)
      *
      * RESULTS
      *              0               successfully set cancelability type,
      *              EINVAL          'type' is invalid
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  pthread_t self = pthread_self ();
  ptw32_thread_t * sp = (ptw32_thread_t *) self.p;

  if (sp == NULL
      || (type != PTHREAD_CANCEL_DEFERRED
	  && type != PTHREAD_CANCEL_ASYNCHRONOUS))
    {
      return EINVAL;
    }

  /*
   * Lock for async-cancel safety.
   */
  (void) pthread_mutex_lock (&sp->cancelLock);

  if (oldtype != NULL)
    {
      *oldtype = sp->cancelType;
    }

  sp->cancelType = type;

  /*
   * Check if there is a pending asynchronous cancel
   */
  if (sp->cancelState == PTHREAD_CANCEL_ENABLE
      && type == PTHREAD_CANCEL_ASYNCHRONOUS
      && WaitForSingleObject (sp->cancelEvent, 0) == WAIT_OBJECT_0)
    {
      sp->state = PThreadStateCanceling;
      sp->cancelState = PTHREAD_CANCEL_DISABLE;
      ResetEvent (sp->cancelEvent);
      (void) pthread_mutex_unlock (&sp->cancelLock);
      ptw32_throw (PTW32_EPS_CANCEL);

      /* Never reached */
    }

  (void) pthread_mutex_unlock (&sp->cancelLock);

  return (result);

}				/* pthread_setcanceltype */

// ===========================SOURCE FILE BREAK===============================

void
pthread_testcancel (void)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function creates a deferred cancellation point
      *      in the calling thread. The call has no effect if the
      *      current cancelability state is
      *              PTHREAD_CANCEL_DISABLE
      *
      * PARAMETERS
      *      N/A
      *
      *
      * DESCRIPTION
      *      This function creates a deferred cancellation point
      *      in the calling thread. The call has no effect if the
      *      current cancelability state is
      *              PTHREAD_CANCEL_DISABLE
      *
      *      NOTES:
      *      1)      Cancellation is asynchronous. Use pthread_join
      *              to wait for termination of thread if necessary
      *
      * RESULTS
      *              N/A
      *
      * ------------------------------------------------------
      */
{
  pthread_t self = pthread_self ();
  ptw32_thread_t * sp = (ptw32_thread_t *) self.p;

  if (sp == NULL)
    {
      return;
    }

  /*
   * Pthread_cancel() will have set sp->state to PThreadStateCancelPending
   * and set an event, so no need to enter kernel space if
   * sp->state != PThreadStateCancelPending - that only slows us down.
   */
  if (sp->state != PThreadStateCancelPending)
    {
      return;
    }

  (void) pthread_mutex_lock (&sp->cancelLock);

  if (sp->cancelState != PTHREAD_CANCEL_DISABLE)
    {
      ResetEvent(sp->cancelEvent);
      sp->state = PThreadStateCanceling;
      (void) pthread_mutex_unlock (&sp->cancelLock);
      sp->cancelState = PTHREAD_CANCEL_DISABLE;
      (void) pthread_mutex_unlock (&sp->cancelLock);
      ptw32_throw (PTW32_EPS_CANCEL);
    }

  (void) pthread_mutex_unlock (&sp->cancelLock);
}				/* pthread_testcancel */

// ===========================SOURCE FILE BREAK===============================

// WINCE note - this is a guess:  
//    ULONG Sp;
//    ULONG Lr;
//    ULONG Pc;
//    ULONG Psr;
#if defined(ARM)
#define PTW32_PROGCTR(Context)  ((Context).Sp)
#endif


#if defined(_M_IX86) || defined(_X86_)
#define PTW32_PROGCTR(Context)  ((Context).Eip)
#endif

#if defined (_M_IA64)
#define PTW32_PROGCTR(Context)  ((Context).StIIP)
#endif

#if defined(_MIPS_)
#define PTW32_PROGCTR(Context)  ((Context).Fir)
#endif

#if defined(_ALPHA_)
#define PTW32_PROGCTR(Context)  ((Context).Fir)
#endif

#if defined(_PPC_)
#define PTW32_PROGCTR(Context)  ((Context).Iar)
#endif

#if defined(_AMD64_)
#define PTW32_PROGCTR(Context)  ((Context).Rip)
#endif


#if !defined(PTW32_PROGCTR)
#error Module contains CPU-specific code; modify and recompile.
#endif

static void
ptw32_cancel_self (void)
{
  ptw32_throw (PTW32_EPS_CANCEL);

  /* Never reached */
}

static void CALLBACK
ptw32_cancel_callback (DWORD unused)
{
  ptw32_throw (PTW32_EPS_CANCEL);

  /* Never reached */
}

/*
 * ptw32_RegisterCancelation() -
 * Must have args of same type as QueueUserAPCEx because this function
 * is a substitute for QueueUserAPCEx if it's not available.
 */
DWORD
ptw32_RegisterCancelation (PAPCFUNC unused1, HANDLE threadH, DWORD unused2)
{
  CONTEXT context;

  context.ContextFlags = CONTEXT_CONTROL;
  GetThreadContext (threadH, &context);
  PTW32_PROGCTR (context) = (DWORD_PTR) ptw32_cancel_self;
  SetThreadContext (threadH, &context);
  return 0;
}

int
pthread_cancel (pthread_t thread)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function requests cancellation of 'thread'.
      *
      * PARAMETERS
      *      thread
      *              reference to an instance of pthread_t
      *
      *
      * DESCRIPTION
      *      This function requests cancellation of 'thread'.
      *      NOTE: cancellation is asynchronous; use pthread_join to
      *                wait for termination of 'thread' if necessary.
      *
      * RESULTS
      *              0               successfully requested cancellation,
      *              ESRCH           no thread found corresponding to 'thread',
      *              ENOMEM          implicit self thread create failed.
      * ------------------------------------------------------
      */
{
  int result;
  int cancel_self;
  pthread_t self;
  ptw32_thread_t * tp;

  result = pthread_kill (thread, 0);

  if (0 != result)
    {
      return result;
    }

  if ((self = pthread_self ()).p == NULL)
    {
      return ENOMEM;
    };

  /*
   * FIXME!!
   *
   * Can a thread cancel itself?
   *
   * The standard doesn't
   * specify an error to be returned if the target
   * thread is itself.
   *
   * If it may, then we need to ensure that a thread can't
   * deadlock itself trying to cancel itself asyncronously
   * (pthread_cancel is required to be an async-cancel
   * safe function).
   */
  cancel_self = pthread_equal (thread, self);

  tp = (ptw32_thread_t *) thread.p;

  /*
   * Lock for async-cancel safety.
   */
  (void) pthread_mutex_lock (&tp->cancelLock);

  if (tp->cancelType == PTHREAD_CANCEL_ASYNCHRONOUS
      && tp->cancelState == PTHREAD_CANCEL_ENABLE
      && tp->state < PThreadStateCanceling)
    {
      if (cancel_self)
	{
	  tp->state = PThreadStateCanceling;
	  tp->cancelState = PTHREAD_CANCEL_DISABLE;

	  (void) pthread_mutex_unlock (&tp->cancelLock);
	  ptw32_throw (PTW32_EPS_CANCEL);

	  /* Never reached */
	}
      else
	{
	  HANDLE threadH = tp->threadH;

	  SuspendThread (threadH);

	  if (WaitForSingleObject (threadH, 0) == WAIT_TIMEOUT)
	    {
	      tp->state = PThreadStateCanceling;
	      tp->cancelState = PTHREAD_CANCEL_DISABLE;
	      /*
	       * If alertdrv and QueueUserAPCEx is available then the following
	       * will result in a call to QueueUserAPCEx with the args given, otherwise
	       * this will result in a call to ptw32_RegisterCancelation and only
	       * the threadH arg will be used.
	       */
	     
		  // the callback is empty, never gets called, and this fails to build in 64 bit so it was removed.
		  // ptw32_register_cancelation (ptw32_cancel_callback, threadH, 0);

	      (void) pthread_mutex_unlock (&tp->cancelLock);
	      ResumeThread (threadH);
	    }
	}
    }
  else
    {
      /*
       * Set for deferred cancellation.
       */
      if (tp->state < PThreadStateCancelPending)
	{
	  tp->state = PThreadStateCancelPending;
	  if (!SetEvent (tp->cancelEvent))
	    {
	      result = ESRCH;
	    }
	}
      else if (tp->state >= PThreadStateCanceling)
	{
	  result = ESRCH;
	}

      (void) pthread_mutex_unlock (&tp->cancelLock);
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "cleanup.c"
//
/*
 * The functions ptw32_pop_cleanup and ptw32_push_cleanup
 * are implemented here for applications written in C with no
 * SEH or C++ destructor support. 
 */

ptw32_cleanup_t *
ptw32_pop_cleanup (int execute)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function pops the most recently pushed cleanup
      *      handler. If execute is nonzero, then the cleanup handler
      *      is executed if non-null.
      *
      * PARAMETERS
      *      execute
      *              if nonzero, execute the cleanup handler
      *
      *
      * DESCRIPTION
      *      This function pops the most recently pushed cleanup
      *      handler. If execute is nonzero, then the cleanup handler
      *      is executed if non-null.
      *      NOTE: specify 'execute' as nonzero to avoid duplication
      *                of common cleanup code.
      *
      * RESULTS
      *              N/A
      *
      * ------------------------------------------------------
      */
{
  ptw32_cleanup_t *cleanup;

  cleanup = (ptw32_cleanup_t *) pthread_getspecific (ptw32_cleanupKey);

  if (cleanup != NULL)
    {
      if (execute && (cleanup->routine != NULL))
	{

	  (*cleanup->routine) (cleanup->arg);

	}

      pthread_setspecific (ptw32_cleanupKey, (void *) cleanup->prev);

    }

  return (cleanup);

}				/* ptw32_pop_cleanup */


void
ptw32_push_cleanup (ptw32_cleanup_t * cleanup,
		    ptw32_cleanup_callback_t routine, void *arg)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function pushes a new cleanup handler onto the thread's stack
      *      of cleanup handlers. Each cleanup handler pushed onto the stack is
      *      popped and invoked with the argument 'arg' when
      *              a) the thread exits by calling 'pthread_exit',
      *              b) when the thread acts on a cancellation request,
      *              c) or when the thread calls pthread_cleanup_pop with a nonzero
      *                 'execute' argument
      *
      * PARAMETERS
      *      cleanup
      *              a pointer to an instance of pthread_cleanup_t,
      *
      *      routine
      *              pointer to a cleanup handler,
      *
      *      arg
      *              parameter to be passed to the cleanup handler
      *
      *
      * DESCRIPTION
      *      This function pushes a new cleanup handler onto the thread's stack
      *      of cleanup handlers. Each cleanup handler pushed onto the stack is
      *      popped and invoked with the argument 'arg' when
      *              a) the thread exits by calling 'pthread_exit',
      *              b) when the thread acts on a cancellation request,
      *              c) or when the thrad calls pthread_cleanup_pop with a nonzero
      *                 'execute' argument
      *      NOTE: pthread_push_cleanup, ptw32_pop_cleanup must be paired
      *                in the same lexical scope.
      *
      * RESULTS
      *              pthread_cleanup_t *
      *                              pointer to the previous cleanup
      *
      * ------------------------------------------------------
      */
{
  cleanup->routine = routine;
  cleanup->arg = arg;

  cleanup->prev = (ptw32_cleanup_t *) pthread_getspecific (ptw32_cleanupKey);

  pthread_setspecific (ptw32_cleanupKey, (void *) cleanup);

}				/* ptw32_push_cleanup */

// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "condvar.c"
//
//#include "ptw32_cond_check_need_init.c"
//#include "pthread_condattr_init.c"
//#include "pthread_condattr_destroy.c"
//#include "pthread_condattr_getpshared.c"
//#include "pthread_condattr_setpshared.c"
//#include "pthread_cond_init.c"
//#include "pthread_cond_destroy.c"
//#include "pthread_cond_wait.c"
//#include "pthread_cond_signal.c"


INLINE int
ptw32_cond_check_need_init (pthread_cond_t * cond)
{
  int result = 0;

  /*
   * The following guarded test is specifically for statically
   * initialised condition variables (via PTHREAD_OBJECT_INITIALIZER).
   *
   * Note that by not providing this synchronisation we risk
   * introducing race conditions into applications which are
   * correctly written.
   *
   * Approach
   * --------
   * We know that static condition variables will not be PROCESS_SHARED
   * so we can serialise access to internal state using
   * Win32 Critical Sections rather than Win32 Mutexes.
   *
   * If using a single global lock slows applications down too much,
   * multiple global locks could be created and hashed on some random
   * value associated with each mutex, the pointer perhaps. At a guess,
   * a good value for the optimal number of global locks might be
   * the number of processors + 1.
   *
   */
  EnterCriticalSection (&ptw32_cond_test_init_lock);

  /*
   * We got here possibly under race
   * conditions. Check again inside the critical section.
   * If a static cv has been destroyed, the application can
   * re-initialise it only by calling pthread_cond_init()
   * explicitly.
   */
  if (*cond == PTHREAD_COND_INITIALIZER)
    {
      result = pthread_cond_init (cond, NULL);
    }
  else if (*cond == NULL)
    {
      /*
       * The cv has been destroyed while we were waiting to
       * initialise it, so the operation that caused the
       * auto-initialisation should fail.
       */
      result = EINVAL;
    }

  LeaveCriticalSection (&ptw32_cond_test_init_lock);

  return result;
}

// ===========================SOURCE FILE BREAK===============================

int
pthread_condattr_init (pthread_condattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Initializes a condition variable attributes object
      *      with default attributes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_condattr_t
      *
      *
      * DESCRIPTION
      *      Initializes a condition variable attributes object
      *      with default attributes.
      *
      *      NOTES:
      *              1)      Use to define condition variable types
      *              2)      It is up to the application to ensure
      *                      that it doesn't re-init an attribute
      *                      without destroying it first. Otherwise
      *                      a memory leak is created.
      *
      * RESULTS
      *              0               successfully initialized attr,
      *              ENOMEM          insufficient memory for attr.
      *
      * ------------------------------------------------------
      */
{
  pthread_condattr_t attr_result;
  int result = 0;

  attr_result = (pthread_condattr_t) calloc (1, sizeof (*attr_result));

  if (attr_result == NULL)
    {
      result = ENOMEM;
    }

  *attr = attr_result;

  return result;

}				/* pthread_condattr_init */

// ===========================SOURCE FILE BREAK===============================

int
pthread_condattr_destroy (pthread_condattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Destroys a condition variable attributes object.
      *      The object can no longer be used.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_condattr_t
      *
      *
      * DESCRIPTION
      *      Destroys a condition variable attributes object.
      *      The object can no longer be used.
      *
      *      NOTES:
      *      1)      Does not affect condition variables created
      *              using 'attr'
      *
      * RESULTS
      *              0               successfully released attr,
      *              EINVAL          'attr' is invalid.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (attr == NULL || *attr == NULL)
    {
      result = EINVAL;
    }
  else
    {
      (void) free (*attr);

      *attr = NULL;
      result = 0;
    }

  return result;

}				/* pthread_condattr_destroy */

// ===========================SOURCE FILE BREAK===============================

int
pthread_condattr_getpshared (const pthread_condattr_t * attr, int *pshared)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Determine whether condition variables created with 'attr'
      *      can be shared between processes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_condattr_t
      *
      *      pshared
      *              will be set to one of:
      *
      *                      PTHREAD_PROCESS_SHARED
      *                              May be shared if in shared memory
      *
      *                      PTHREAD_PROCESS_PRIVATE
      *                              Cannot be shared.
      *
      *
      * DESCRIPTION
      *      Condition Variables created with 'attr' can be shared
      *      between processes if pthread_cond_t variable is allocated
      *      in memory shared by these processes.
      *      NOTES:
      *      1)      pshared condition variables MUST be allocated in
      *              shared memory.
      *
      *      2)      The following macro is defined if shared mutexes
      *              are supported:
      *                      _POSIX_THREAD_PROCESS_SHARED
      *
      * RESULTS
      *              0               successfully retrieved attribute,
      *              EINVAL          'attr' or 'pshared' is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result;

  if ((attr != NULL && *attr != NULL) && (pshared != NULL))
    {
      *pshared = (*attr)->pshared;
      result = 0;
    }
  else
    {
      result = EINVAL;
    }

  return result;

}				/* pthread_condattr_getpshared */

// ===========================SOURCE FILE BREAK===============================

int
pthread_condattr_setpshared (pthread_condattr_t * attr, int pshared)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Mutexes created with 'attr' can be shared between
      *      processes if pthread_mutex_t variable is allocated
      *      in memory shared by these processes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *      pshared
      *              must be one of:
      *
      *                      PTHREAD_PROCESS_SHARED
      *                              May be shared if in shared memory
      *
      *                      PTHREAD_PROCESS_PRIVATE
      *                              Cannot be shared.
      *
      * DESCRIPTION
      *      Mutexes creatd with 'attr' can be shared between
      *      processes if pthread_mutex_t variable is allocated
      *      in memory shared by these processes.
      *
      *      NOTES:
      *              1)      pshared mutexes MUST be allocated in shared
      *                      memory.
      *
      *              2)      The following macro is defined if shared mutexes
      *                      are supported:
      *                              _POSIX_THREAD_PROCESS_SHARED
      *
      * RESULTS
      *              0               successfully set attribute,
      *              EINVAL          'attr' or pshared is invalid,
      *              ENOSYS          PTHREAD_PROCESS_SHARED not supported,
      *
      * ------------------------------------------------------
      */
{
  int result;

  if ((attr != NULL && *attr != NULL)
      && ((pshared == PTHREAD_PROCESS_SHARED)
	  || (pshared == PTHREAD_PROCESS_PRIVATE)))
    {
      if (pshared == PTHREAD_PROCESS_SHARED)
	{

#if !defined( _POSIX_THREAD_PROCESS_SHARED )
	  result = ENOSYS;
	  pshared = PTHREAD_PROCESS_PRIVATE;
#else
	  result = 0;

#endif /* _POSIX_THREAD_PROCESS_SHARED */

	}
      else
	{
	  result = 0;
	}

      (*attr)->pshared = pshared;
    }
  else
    {
      result = EINVAL;
    }

  return result;

}				/* pthread_condattr_setpshared */

// ===========================SOURCE FILE BREAK===============================
int
pthread_cond_init (pthread_cond_t * cond, const pthread_condattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function initializes a condition variable.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of pthread_cond_t
      *
      *      attr
      *              specifies optional creation attributes.
      *
      *
      * DESCRIPTION
      *      This function initializes a condition variable.
      *
      * RESULTS
      *              0               successfully created condition variable,
      *              EINVAL          'attr' is invalid,
      *              EAGAIN          insufficient resources (other than
      *                              memory,
      *              ENOMEM          insufficient memory,
      *              EBUSY           'cond' is already initialized,
      *
      * ------------------------------------------------------
      */
{
	int result;
  pthread_cond_t cv = NULL;

  if (cond == NULL)
    {
      return EINVAL;
    }

  if ((attr != NULL && *attr != NULL) &&
      ((*attr)->pshared == PTHREAD_PROCESS_SHARED))
    {
      /*
       * Creating condition variable that can be shared between
       * processes.
       */
      result = ENOSYS;
      goto DONE;
    }

  cv = (pthread_cond_t) calloc (1, sizeof (*cv));

  if (cv == NULL)
    {
      result = ENOMEM;
      goto DONE;
    }

  cv->nWaitersBlocked = 0;
  cv->nWaitersToUnblock = 0;
  cv->nWaitersGone = 0;

  if (sem_init (&(cv->semBlockLock), 0, 1) != 0)
    {
      result = errno;
      goto FAIL0;
    }

  if (sem_init (&(cv->semBlockQueue), 0, 0) != 0)
    {
      result = errno;
      goto FAIL1;
    }

  if ((result = pthread_mutex_init (&(cv->mtxUnblockLock), 0)) != 0)
    {
      goto FAIL2;
    }

  result = 0;

  goto DONE;

  /*
   * -------------
   * Failed...
   * -------------
   */
FAIL2:
  (void) sem_destroy (&(cv->semBlockQueue));

FAIL1:
  (void) sem_destroy (&(cv->semBlockLock));

FAIL0:
  (void) free (cv);
  cv = NULL;

DONE:
  if (0 == result)
    {
      EnterCriticalSection (&ptw32_cond_list_lock);

      cv->next = NULL;
      cv->prev = ptw32_cond_list_tail;

      if (ptw32_cond_list_tail != NULL)
	{
	  ptw32_cond_list_tail->next = cv;
	}

      ptw32_cond_list_tail = cv;

      if (ptw32_cond_list_head == NULL)
	{
	  ptw32_cond_list_head = cv;
	}

      LeaveCriticalSection (&ptw32_cond_list_lock);
    }

  *cond = cv;

  return result;

}				/* pthread_cond_init */
// ===========================SOURCE FILE BREAK===============================

int
pthread_cond_destroy (pthread_cond_t * cond)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function destroys a condition variable
      *
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of pthread_cond_t
      *
      *
      * DESCRIPTION
      *      This function destroys a condition variable.
      *
      *      NOTES:
      *              1)      A condition variable can be destroyed
      *                      immediately after all the threads that
      *                      are blocked on it are awakened. e.g.
      *
      *                      struct list {
      *                        pthread_mutex_t lm;
      *                        ...
      *                      }
      *
      *                      struct elt {
      *                        key k;
      *                        int busy;
      *                        pthread_cond_t notbusy;
      *                        ...
      *                      }
      *
      *                      
      *                      struct elt *
      *                      list_find(struct list *lp, key k)
      *                      {
      *                        struct elt *ep;
      *
      *                        pthread_mutex_lock(&lp->lm);
      *                        while ((ep = find_elt(l,k) != NULL) && ep->busy)
      *                          pthread_cond_wait(&ep->notbusy, &lp->lm);
      *                        if (ep != NULL)
      *                          ep->busy = 1;
      *                        pthread_mutex_unlock(&lp->lm);
      *                        return(ep);
      *                      }
      *
      *                      delete_elt(struct list *lp, struct elt *ep)
      *                      {
      *                        pthread_mutex_lock(&lp->lm);
      *                        assert(ep->busy);
      *                        ... remove ep from list ...
      *                        ep->busy = 0;
      *                    (A) pthread_cond_broadcast(&ep->notbusy);
      *                        pthread_mutex_unlock(&lp->lm);
      *                    (B) pthread_cond_destroy(&rp->notbusy);
      *                        free(ep);
      *                      }
      *
      *                      In this example, the condition variable
      *                      and its list element may be freed (line B)
      *                      immediately after all threads waiting for
      *                      it are awakened (line A), since the mutex
      *                      and the code ensure that no other thread
      *                      can touch the element to be deleted.
      *
      * RESULTS
      *              0               successfully released condition variable,
      *              EINVAL          'cond' is invalid,
      *              EBUSY           'cond' is in use,
      *
      * ------------------------------------------------------
      */
{
  pthread_cond_t cv;
  int result = 0, result1 = 0, result2 = 0;

  /*
   * Assuming any race condition here is harmless.
   */
  if (cond == NULL || *cond == NULL)
    {
      return EINVAL;
    }

  if (*cond != PTHREAD_COND_INITIALIZER)
    {
#ifndef WINCE
		if (ptw32_cond_list_lock.OwningThread == 0 && ptw32_cond_list_lock.DebugInfo == 0)	// UBT added this to handle application shutdown race conditions
		  return 0;
#endif

      EnterCriticalSection (&ptw32_cond_list_lock);

      cv = *cond;

      /*
       * Close the gate; this will synchronize this thread with
       * all already signaled waiters to let them retract their
       * waiter status - SEE NOTE 1 ABOVE!!!
       */
      if (sem_wait (&(cv->semBlockLock)) != 0)
	{
	  return errno;
	}

      /*
       * !TRY! lock mtxUnblockLock; try will detect busy condition
       * and will not cause a deadlock with respect to concurrent
       * signal/broadcast.
       */
      if ((result = pthread_mutex_trylock (&(cv->mtxUnblockLock))) != 0)
	{
	  (void) sem_post (&(cv->semBlockLock));
	  return result;
	}

      /*
       * Check whether cv is still busy (still has waiters)
       */
      if (cv->nWaitersBlocked > cv->nWaitersGone)
	{
	  if (sem_post (&(cv->semBlockLock)) != 0)
	    {
	      result = errno;
	    }
	  result1 = pthread_mutex_unlock (&(cv->mtxUnblockLock));
	  result2 = EBUSY;
	}
      else
	{
	  /*
	   * Now it is safe to destroy
	   */
	  *cond = NULL;

	  if (sem_destroy (&(cv->semBlockLock)) != 0)
	    {
	      result = errno;
	    }
	  if (sem_destroy (&(cv->semBlockQueue)) != 0)
	    {
	      result1 = errno;
	    }
	  if ((result2 = pthread_mutex_unlock (&(cv->mtxUnblockLock))) == 0)
	    {
	      result2 = pthread_mutex_destroy (&(cv->mtxUnblockLock));
	    }

	  /* Unlink the CV from the list */

	  if (ptw32_cond_list_head == cv)
	    {
	      ptw32_cond_list_head = cv->next;
	    }
	  else
	    {
	      cv->prev->next = cv->next;
	    }

	  if (ptw32_cond_list_tail == cv)
	    {
	      ptw32_cond_list_tail = cv->prev;
	    }
	  else
	    {
	      cv->next->prev = cv->prev;
	    }

	  (void) free (cv);
	}

      LeaveCriticalSection (&ptw32_cond_list_lock);
    }
  else
    {
      /*
       * See notes in ptw32_cond_check_need_init() above also.
       */
      EnterCriticalSection (&ptw32_cond_test_init_lock);

      /*
       * Check again.
       */
      if (*cond == PTHREAD_COND_INITIALIZER)
	{
	  /*
	   * This is all we need to do to destroy a statically
	   * initialised cond that has not yet been used (initialised).
	   * If we get to here, another thread waiting to initialise
	   * this cond will get an EINVAL. That's OK.
	   */
	  *cond = NULL;
	}
      else
	{
	  /*
	   * The cv has been initialised while we were waiting
	   * so assume it's in use.
	   */
	  result = EBUSY;
	}

      LeaveCriticalSection (&ptw32_cond_test_init_lock);
    }

  return ((result != 0) ? result : ((result1 != 0) ? result1 : result2));
}

// ===========================SOURCE FILE BREAK===============================
/*
 * Arguments for cond_wait_cleanup, since we can only pass a
 * single void * to it.
 */
typedef struct
{
  pthread_mutex_t *mutexPtr;
  pthread_cond_t cv;
  int *resultPtr;
} ptw32_cond_wait_cleanup_args_t;

static void PTW32_CDECL
ptw32_cond_wait_cleanup (void *args)
{
  ptw32_cond_wait_cleanup_args_t *cleanup_args =
    (ptw32_cond_wait_cleanup_args_t *) args;
  pthread_cond_t cv = cleanup_args->cv;
  int *resultPtr = cleanup_args->resultPtr;
  int nSignalsWasLeft;
  int result;

  /*
   * Whether we got here as a result of signal/broadcast or because of
   * timeout on wait or thread cancellation we indicate that we are no
   * longer waiting. The waiter is responsible for adjusting waiters
   * (to)unblock(ed) counts (protected by unblock lock).
   */
  if ((result = pthread_mutex_lock (&(cv->mtxUnblockLock))) != 0)
    {
      *resultPtr = result;
      return;
    }

  if (0 != (nSignalsWasLeft = cv->nWaitersToUnblock))
    {
      --(cv->nWaitersToUnblock);
    }
  else if (INT_MAX / 2 == ++(cv->nWaitersGone))
    {
      /* Use the non-cancellable version of sem_wait() */
      if (ptw32_semwait (&(cv->semBlockLock)) != 0)
	{
	  *resultPtr = errno;
	  /*
	   * This is a fatal error for this CV,
	   * so we deliberately don't unlock
	   * cv->mtxUnblockLock before returning.
	   */
	  return;
	}
      cv->nWaitersBlocked -= cv->nWaitersGone;
      if (sem_post (&(cv->semBlockLock)) != 0)
	{
	  *resultPtr = errno;
	  /*
	   * This is a fatal error for this CV,
	   * so we deliberately don't unlock
	   * cv->mtxUnblockLock before returning.
	   */
	  return;
	}
      cv->nWaitersGone = 0;
    }

  if ((result = pthread_mutex_unlock (&(cv->mtxUnblockLock))) != 0)
    {
      *resultPtr = result;
      return;
    }

  if (1 == nSignalsWasLeft)
    {
      if (sem_post (&(cv->semBlockLock)) != 0)
	{
	  *resultPtr = errno;
	  return;
	}
    }

  /*
   * XSH: Upon successful return, the mutex has been locked and is owned
   * by the calling thread.
   */
  if ((result = pthread_mutex_lock (cleanup_args->mutexPtr)) != 0)
    {
      *resultPtr = result;
    }
}				/* ptw32_cond_wait_cleanup */

static INLINE int
ptw32_cond_timedwait (pthread_cond_t * cond,
		      pthread_mutex_t * mutex, const struct timespec *abstime)
{
  int result = 0;
  pthread_cond_t cv;
  ptw32_cond_wait_cleanup_args_t cleanup_args;

  if (cond == NULL || *cond == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static condition variable. We check
   * again inside the guarded section of ptw32_cond_check_need_init()
   * to avoid race conditions.
   */
  if (*cond == PTHREAD_COND_INITIALIZER)
    {
      result = ptw32_cond_check_need_init (cond);
    }

  if (result != 0 && result != EBUSY)
    {
      return result;
    }

  cv = *cond;

  /* Thread can be cancelled in sem_wait() but this is OK */
  if (sem_wait (&(cv->semBlockLock)) != 0)
    {
      return errno;
    }

  ++(cv->nWaitersBlocked);

  if (sem_post (&(cv->semBlockLock)) != 0)
    {
      return errno;
    }

  /*
   * Setup this waiter cleanup handler
   */
  cleanup_args.mutexPtr = mutex;
  cleanup_args.cv = cv;
  cleanup_args.resultPtr = &result;

#ifdef _MSC_VER
#pragma inline_depth(0)
#endif
  pthread_cleanup_push (ptw32_cond_wait_cleanup, (void *) &cleanup_args);

  /*
   * Now we can release 'mutex' and...
   */
  if ((result = pthread_mutex_unlock (mutex)) == 0)
    {

      /*
       * ...wait to be awakened by
       *              pthread_cond_signal, or
       *              pthread_cond_broadcast, or
       *              timeout, or
       *              thread cancellation
       *
       * Note:
       *
       *      sem_timedwait is a cancellation point,
       *      hence providing the mechanism for making
       *      pthread_cond_wait a cancellation point.
       *      We use the cleanup mechanism to ensure we
       *      re-lock the mutex and adjust (to)unblock(ed) waiters
       *      counts if we are cancelled, timed out or signalled.
       */
      if (sem_timedwait (&(cv->semBlockQueue), abstime) != 0)
	{
	  result = errno;
	}
    }

  /*
   * Always cleanup
   */
  pthread_cleanup_pop (1);
#ifdef _MSC_VER
#pragma inline_depth()
#endif

  /*
   * "result" can be modified by the cleanup handler.
   */
  return result;

}				/* ptw32_cond_timedwait */


int
pthread_cond_wait (pthread_cond_t * cond, pthread_mutex_t * mutex)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function waits on a condition variable until
      *      awakened by a signal or broadcast.
      *
      *      Caller MUST be holding the mutex lock; the
      *      lock is released and the caller is blocked waiting
      *      on 'cond'. When 'cond' is signaled, the mutex
      *      is re-acquired before returning to the caller.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of pthread_cond_t
      *
      *      mutex
      *              pointer to an instance of pthread_mutex_t
      *
      *
      * DESCRIPTION
      *      This function waits on a condition variable until
      *      awakened by a signal or broadcast.
      *
      *      NOTES:
      *
      *      1)      The function must be called with 'mutex' LOCKED
      *              by the calling thread, or undefined behaviour
      *              will result.
      *
      *      2)      This routine atomically releases 'mutex' and causes
      *              the calling thread to block on the condition variable.
      *              The blocked thread may be awakened by 
      *                      pthread_cond_signal or 
      *                      pthread_cond_broadcast.
      *
      * Upon successful completion, the 'mutex' has been locked and 
      * is owned by the calling thread.
      *
      *
      * RESULTS
      *              0               caught condition; mutex released,
      *              EINVAL          'cond' or 'mutex' is invalid,
      *              EINVAL          different mutexes for concurrent waits,
      *              EINVAL          mutex is not held by the calling thread,
      *
      * ------------------------------------------------------
      */
{
  /*
   * The NULL abstime arg means INFINITE waiting.
   */
  return (ptw32_cond_timedwait (cond, mutex, NULL));

}				/* pthread_cond_wait */


int
pthread_cond_timedwait (pthread_cond_t * cond,
			pthread_mutex_t * mutex,
			const struct timespec *abstime)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function waits on a condition variable either until
      *      awakened by a signal or broadcast; or until the time
      *      specified by abstime passes.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of pthread_cond_t
      *
      *      mutex
      *              pointer to an instance of pthread_mutex_t
      *
      *      abstime
      *              pointer to an instance of (const struct timespec)
      *
      *
      * DESCRIPTION
      *      This function waits on a condition variable either until
      *      awakened by a signal or broadcast; or until the time
      *      specified by abstime passes.
      *
      *      NOTES:
      *      1)      The function must be called with 'mutex' LOCKED
      *              by the calling thread, or undefined behaviour
      *              will result.
      *
      *      2)      This routine atomically releases 'mutex' and causes
      *              the calling thread to block on the condition variable.
      *              The blocked thread may be awakened by 
      *                      pthread_cond_signal or 
      *                      pthread_cond_broadcast.
      *
      *
      * RESULTS
      *              0               caught condition; mutex released,
      *              EINVAL          'cond', 'mutex', or abstime is invalid,
      *              EINVAL          different mutexes for concurrent waits,
      *              EINVAL          mutex is not held by the calling thread,
      *              ETIMEDOUT       abstime ellapsed before cond was signaled.
      *
      * ------------------------------------------------------
      */
{
  if (abstime == NULL)
    {
      return EINVAL;
    }

  return (ptw32_cond_timedwait (cond, mutex, abstime));

}				/* pthread_cond_timedwait */

// ===========================SOURCE FILE BREAK===============================
static INLINE int
ptw32_cond_unblock (pthread_cond_t * cond, int unblockAll)
     /*
      * Notes.
      *
      * Does not use the external mutex for synchronisation,
      * therefore semBlockLock is needed.
      * mtxUnblockLock is for LEVEL-2 synch. LEVEL-2 is the
      * state where the external mutex is not necessarily locked by
      * any thread, ie. between cond_wait unlocking and re-acquiring
      * the lock after having been signaled or a timeout or
      * cancellation.
      *
      * Uses the following CV elements:
      *   nWaitersBlocked
      *   nWaitersToUnblock
      *   nWaitersGone
      *   mtxUnblockLock
      *   semBlockLock
      *   semBlockQueue
      */
{
  int result;
  pthread_cond_t cv;
  int nSignalsToIssue;

  if (cond == NULL || *cond == NULL)
    {
      return EINVAL;
    }

  cv = *cond;

  /*
   * No-op if the CV is static and hasn't been initialised yet.
   * Assuming that any race condition is harmless.
   */
  if (cv == PTHREAD_COND_INITIALIZER)
    {
      return 0;
    }

  if ((result = pthread_mutex_lock (&(cv->mtxUnblockLock))) != 0)
    {
      return result;
    }

  if (0 != cv->nWaitersToUnblock)
    {
      if (0 == cv->nWaitersBlocked)
	{
	  return pthread_mutex_unlock (&(cv->mtxUnblockLock));
	}
      if (unblockAll)
	{
	  cv->nWaitersToUnblock += (nSignalsToIssue = cv->nWaitersBlocked);
	  cv->nWaitersBlocked = 0;
	}
      else
	{
	  nSignalsToIssue = 1;
	  cv->nWaitersToUnblock++;
	  cv->nWaitersBlocked--;
	}
    }
  else if (cv->nWaitersBlocked > cv->nWaitersGone)
    {
      /* Use the non-cancellable version of sem_wait() */
      if (ptw32_semwait (&(cv->semBlockLock)) != 0)
	{
	  result = errno;
	  (void) pthread_mutex_unlock (&(cv->mtxUnblockLock));
	  return result;
	}
      if (0 != cv->nWaitersGone)
	{
	  cv->nWaitersBlocked -= cv->nWaitersGone;
	  cv->nWaitersGone = 0;
	}
      if (unblockAll)
	{
	  nSignalsToIssue = cv->nWaitersToUnblock = cv->nWaitersBlocked;
	  cv->nWaitersBlocked = 0;
	}
      else
	{
	  nSignalsToIssue = cv->nWaitersToUnblock = 1;
	  cv->nWaitersBlocked--;
	}
    }
  else
    {
      return pthread_mutex_unlock (&(cv->mtxUnblockLock));
    }

  if ((result = pthread_mutex_unlock (&(cv->mtxUnblockLock))) == 0)
    {
      if (sem_post_multiple (&(cv->semBlockQueue), nSignalsToIssue) != 0)
	{
	  result = errno;
	}
    }

  return result;

}				/* ptw32_cond_unblock */

int
pthread_cond_signal (pthread_cond_t * cond)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function signals a condition variable, waking
      *      one waiting thread.
      *      If SCHED_FIFO or SCHED_RR policy threads are waiting
      *      the highest priority waiter is awakened; otherwise,
      *      an unspecified waiter is awakened.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of pthread_cond_t
      *
      *
      * DESCRIPTION
      *      This function signals a condition variable, waking
      *      one waiting thread.
      *      If SCHED_FIFO or SCHED_RR policy threads are waiting
      *      the highest priority waiter is awakened; otherwise,
      *      an unspecified waiter is awakened.
      *
      *      NOTES:
      *
      *      1)      Use when any waiter can respond and only one need
      *              respond (all waiters being equal).
      *
      * RESULTS
      *              0               successfully signaled condition,
      *              EINVAL          'cond' is invalid,
      *
      * ------------------------------------------------------
      */
{
  /*
   * The '0'(FALSE) unblockAll arg means unblock ONE waiter.
   */
  return (ptw32_cond_unblock (cond, 0));

}				/* pthread_cond_signal */

int
pthread_cond_broadcast (pthread_cond_t * cond)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function broadcasts the condition variable,
      *      waking all current waiters.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of pthread_cond_t
      *
      *
      * DESCRIPTION
      *      This function signals a condition variable, waking
      *      all waiting threads.
      *
      *      NOTES:
      *
      *      1)      Use when more than one waiter may respond to
      *              predicate change or if any waiting thread may
      *              not be able to respond
      *
      * RESULTS
      *              0               successfully signalled condition to all
      *                              waiting threads,
      *              EINVAL          'cond' is invalid
      *              ENOSPC          a required resource has been exhausted,
      *
      * ------------------------------------------------------
      */
{
  /*
   * The TRUE unblockAll arg means unblock ALL waiters.
   */
  return (ptw32_cond_unblock (cond, PTW32_TRUE));

}				/* pthread_cond_broadcast */

// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "create.c"
//
#ifndef _UWIN
 #ifdef WINCE
 #else
  #include <process.h>
 #endif
#endif

int
pthread_create (pthread_t * tid,
		const pthread_attr_t * attr,
		void *(*start) (void *), void *arg)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function creates a thread running the start function,
      *      passing it the parameter value, 'arg'. The 'attr'
      *      argument specifies optional creation attributes.
      *      The identity of the new thread is returned
      *      via 'tid', which should not be NULL.
      *
      * PARAMETERS
      *      tid
      *              pointer to an instance of pthread_t
      *
      *      attr
      *              optional pointer to an instance of pthread_attr_t
      *
      *      start
      *              pointer to the starting routine for the new thread
      *
      *      arg
      *              optional parameter passed to 'start'
      *
      *
      * DESCRIPTION
      *      This function creates a thread running the start function,
      *      passing it the parameter value, 'arg'. The 'attr'
      *      argument specifies optional creation attributes.
      *      The identity of the new thread is returned
      *      via 'tid', which should not be the NULL pointer.
      *
      * RESULTS
      *              0               successfully created thread,
      *              EINVAL          attr invalid,
      *              EAGAIN          insufficient resources.
      *
      * ------------------------------------------------------
      */
{
  pthread_t thread;
  ptw32_thread_t * tp;
  register pthread_attr_t a;
  HANDLE threadH = 0;
  int result = EAGAIN;
  int run = PTW32_TRUE;
  ThreadParms *parms = NULL;
  long stackSize;
  int priority;
  pthread_t self;

  /*
   * Before doing anything, check that tid can be stored through
   * without invoking a memory protection error (segfault).
   * Make sure that the assignment below can't be optimised out by the compiler.
   * This is assured by conditionally assigning *tid again at the end.
   */
  tid->x = 0;

  if (attr != NULL)
    {
      a = *attr;
    }
  else
    {
      a = NULL;
    }

  if ((thread = ptw32_new ()).p == NULL)
    {
      goto FAIL0;
    }

  tp = (ptw32_thread_t *) thread.p;

  priority = tp->sched_priority;

  if ((parms = (ThreadParms *) malloc (sizeof (*parms))) == NULL)
    {
      goto FAIL0;
    }

  parms->tid = thread;
  parms->start = start;
  parms->arg = arg;

#if defined(HAVE_SIGSET_T)

  /*
   * Threads inherit their initial sigmask from their creator thread.
   */
  self = pthread_self();
  tp->sigmask = ((ptw32_thread_t *)self.p)->sigmask;

#endif /* HAVE_SIGSET_T */


  if (a != NULL)
    {
      stackSize = a->stacksize;
      tp->detachState = a->detachstate;
      priority = a->param.sched_priority;

#if (THREAD_PRIORITY_LOWEST > THREAD_PRIORITY_NORMAL)
      /* WinCE */
#else
      /* Everything else */

      /*
       * Thread priority must be set to a valid system level
       * without altering the value set by pthread_attr_setschedparam().
       */

      /*
       * PTHREAD_EXPLICIT_SCHED is the default because Win32 threads
       * don't inherit their creator's priority. They are started with
       * THREAD_PRIORITY_NORMAL (win32 value). The result of not supplying
       * an 'attr' arg to pthread_create() is equivalent to defaulting to
       * PTHREAD_EXPLICIT_SCHED and priority THREAD_PRIORITY_NORMAL.
       */
      if (PTHREAD_INHERIT_SCHED == a->inheritsched)
	{
	  /*
	   * If the thread that called pthread_create() is a Win32 thread
	   * then the inherited priority could be the result of a temporary
	   * system adjustment. This is not the case for POSIX threads.
	   */
#if ! defined(HAVE_SIGSET_T)
	  self = pthread_self ();
#endif
	  priority = ((ptw32_thread_t *) self.p)->sched_priority;
	}

#endif

    }
  else
    {
      /*
       * Default stackSize
       */
      stackSize = PTHREAD_STACK_MIN;
    }

  tp->state = run ? PThreadStateInitial : PThreadStateSuspended;

  tp->keys = NULL;

  /*
   * Threads must be started in suspended mode and resumed if necessary
   * after _beginthreadex returns us the handle. Otherwise we set up a
   * race condition between the creating and the created threads.
   * Note that we also retain a local copy of the handle for use
   * by us in case thread.p->threadH gets NULLed later but before we've
   * finished with it here.
   */

#if ! defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__) 

  tp->threadH =
    threadH =
    (HANDLE) _beginthreadex ((void *) NULL,	/* No security info             */
			     (unsigned) stackSize,	/* default stack size   */
			     ptw32_threadStart,
			     parms,
			     (unsigned)
			     CREATE_SUSPENDED,
			     (unsigned *) &(tp->thread));

  if (threadH != 0)
    {
      if (a != NULL)
	{
	  (void) ptw32_setthreadpriority (thread, SCHED_OTHER, priority);
	}

      if (run)
	{
	  ResumeThread (threadH);
	}
    }

#else /* __MINGW32__ && ! __MSVCRT__ */

  /*
   * This lock will force pthread_threadStart() to wait until we have
   * the thread handle and have set the priority.
   */
  (void) pthread_mutex_lock (&tp->cancelLock);

  tp->threadH =
    threadH =
    (HANDLE) _beginthread (ptw32_threadStart, (unsigned) stackSize,	/* default stack size   */
			   parms);

  /*
   * Make the return code match _beginthreadex's.
   */
  if (threadH == (HANDLE) - 1L)
    {
      tp->threadH = threadH = 0;
    }
  else
    {
      if (!run)
	{
	  /* 
	   * beginthread does not allow for create flags, so we do it now.
	   * Note that beginthread itself creates the thread in SUSPENDED
	   * mode, and then calls ResumeThread to start it.
	   */
	  SuspendThread (threadH);
	}

      if (a != NULL)
	{
	  (void) ptw32_setthreadpriority (thread, SCHED_OTHER, priority);
	}
    }

  (void) pthread_mutex_unlock (&tp->cancelLock);

#endif /* __MINGW32__ && ! __MSVCRT__ */

  result = (threadH != 0) ? 0 : EAGAIN;

  /*
   * Fall Through Intentionally
   */

  /*
   * ------------
   * Failure Code
   * ------------
   */

FAIL0:
  if (result != 0)
    {

      ptw32_threadDestroy (thread);
      tp = NULL;

      if (parms != NULL)
	{
	  free (parms);
	}
    }
  else
    {
      *tid = thread;
    }

#ifdef _UWIN
  if (result == 0)
    pthread_count++;
#endif
  return (result);

}				/* pthread_create */

// ===========================SOURCE FILE BREAK===============================


//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
//#include "errno.c"

/*
 * Re-entrant errno.
 *
 * Each thread has it's own errno variable in pthread_t.
 *
 * The benefit of using the pthread_t structure
 * instead of another TSD key is TSD keys are limited
 * on Win32 to 64 per process. Secondly, to implement
 * it properly without using pthread_t you'd need
 * to dynamically allocate an int on starting the thread
 * and store it manually into TLS and then ensure that you free
 * it on thread termination. We get all that for free
 * by simply storing the errno on the pthread_t structure.
 *
 * MSVC and Mingw32 already have their own thread-safe errno.
 *
 * #if defined( _REENTRANT ) || defined( _MT )
 * #define errno *_errno()
 *
 * int *_errno( void );
 * #else
 * extern int errno;
 * #endif
 *
 */
#if defined(NEED_ERRNO)

 static int reallyBad = ENOMEM;

int *
_errno (void)
{
  pthread_t self;
  int *result;
  self = pthread_self ();

#ifdef WINCE
  if (self.p == NULL)
#else
  if (self == NULL)
#endif
  {
      /*
       * Yikes! unable to allocate a thread!
       * Throw an exception? return an error?
       */
      result = &reallyBad;
    }
  else
    {
#ifdef WINCE
	  result = (int *)self.p;
#else
      result = &(self->ptErrno);
#endif
    }

  return (result);

}				/* _errno */

#endif /* (NEED_ERRNO) */


// ===========================SOURCE FILE BREAK===============================




//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "exit.c"
#ifndef _UWIN
#ifndef WINCE
 #include <process.h>
#endif
#endif
void
pthread_exit (void *value_ptr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function terminates the calling thread, returning
      *      the value 'value_ptr' to any joining thread.
      *
      * PARAMETERS
      *      value_ptr
      *              a generic data value (i.e. not the address of a value)
      *
      *
      * DESCRIPTION
      *      This function terminates the calling thread, returning
      *      the value 'value_ptr' to any joining thread.
      *      NOTE: thread should be joinable.
      *
      * RESULTS
      *              N/A
      *
      * ------------------------------------------------------
      */
{
  ptw32_thread_t * sp;

  /*
   * Don't use pthread_self() to avoid creating an implicit POSIX thread handle
   * unnecessarily.
   */
  sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

#ifdef _UWIN
  if (--pthread_count <= 0)
    exit ((int) value_ptr);
#endif

  if (NULL == sp)
    {
      /*
       * A POSIX thread handle was never created. I.e. this is a
       * Win32 thread that has never called a pthreads-win32 routine that
       * required a POSIX handle.
       *
       * Implicit POSIX handles are cleaned up in ptw32_throw() now.
       */

#if ! defined (__MINGW32__) || defined (__MSVCRT__)  || defined (__DMC__)
      _endthreadex ((unsigned) value_ptr);
#else
      _endthread ();
#endif

      /* Never reached */
    }

  sp->exitStatus = value_ptr;

  ptw32_throw (PTW32_EPS_EXIT);

  /* Never reached. */

}
// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "fork.c"		// Nothing.
//

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "global.c"
//
int ptw32_processInitialized = PTW32_FALSE;
ptw32_thread_t * ptw32_threadReuseTop = PTW32_THREAD_REUSE_EMPTY;
ptw32_thread_t * ptw32_threadReuseBottom = PTW32_THREAD_REUSE_EMPTY;
pthread_key_t ptw32_selfThreadKey = NULL;
pthread_key_t ptw32_cleanupKey = NULL;
pthread_cond_t ptw32_cond_list_head = NULL;
pthread_cond_t ptw32_cond_list_tail = NULL;

int ptw32_concurrency = 0;

/* What features have been auto-detaected */
int ptw32_features = 0;

BOOL ptw32_smp_system = PTW32_TRUE;  /* Safer if assumed true initially. */

/* 
 * Function pointer to InterlockedCompareExchange if it exists, otherwise
 * it will be set at runtime to a substitute local version with the same
 * functionality but may be architecture specific.
 */
PTW32_INTERLOCKED_LONG
  (WINAPI * ptw32_interlocked_compare_exchange) (PTW32_INTERLOCKED_LPLONG,
						 PTW32_INTERLOCKED_LONG,
						 PTW32_INTERLOCKED_LONG) =
  NULL;

/* 
 * Function pointer to QueueUserAPCEx if it exists, otherwise
 * it will be set at runtime to a substitute routine which cannot unblock
 * blocked threads.
 */
DWORD (*ptw32_register_cancelation) (PAPCFUNC, HANDLE, DWORD) = NULL;

/*
 * Global lock for managing pthread_t struct reuse.
 */
CRITICAL_SECTION ptw32_thread_reuse_lock;

/*
 * Global lock for testing internal state of statically declared mutexes.
 */
CRITICAL_SECTION ptw32_mutex_test_init_lock;

/*
 * Global lock for testing internal state of PTHREAD_COND_INITIALIZER
 * created condition variables.
 */
CRITICAL_SECTION ptw32_cond_test_init_lock;

/*
 * Global lock for testing internal state of PTHREAD_RWLOCK_INITIALIZER
 * created read/write locks.
 */
CRITICAL_SECTION ptw32_rwlock_test_init_lock;

/*
 * Global lock for testing internal state of PTHREAD_SPINLOCK_INITIALIZER
 * created spin locks.
 */
CRITICAL_SECTION ptw32_spinlock_test_init_lock;

/*
 * Global lock for condition variable linked list. The list exists
 * to wake up CVs when a WM_TIMECHANGE message arrives. See
 * w32_TimeChangeHandler.c.
 */
CRITICAL_SECTION ptw32_cond_list_lock;

#ifdef _UWIN
/*
 * Keep a count of the number of threads.
 */
int pthread_count = 0;
#endif
// ===========================SOURCE FILE BREAK===============================
//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "misc.c"
//
//#include "pthread_kill.c"
//#include "pthread_once.c"
//#include "pthread_self.c"
//#include "pthread_equal.c"
//#include "pthread_setconcurrency.c"
//#include "pthread_getconcurrency.c"
//#include "ptw32_new.c"
//#include "ptw32_calloc.c"
//#include "ptw32_reuse.c"
//#include "w32_CancelableWait.c"

#ifndef WINCE
#include <signal.h>
#endif

int pthread_kill (pthread_t thread, int sig)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function requests that a signal be delivered to the
      *      specified thread. If sig is zero, error checking is
      *      performed but no signal is actually sent such that this
      *      function can be used to check for a valid thread ID.
      *
      * PARAMETERS
      *      thread  reference to an instances of pthread_t
      *      sig     signal. Currently only a value of 0 is supported.
      *
      *
      * DESCRIPTION
      *      This function requests that a signal be delivered to the
      *      specified thread. If sig is zero, error checking is
      *      performed but no signal is actually sent such that this
      *      function can be used to check for a valid thread ID.
      *
      * RESULTS
      *              ESRCH           the thread is not a valid thread ID,
      *              EINVAL          the value of the signal is invalid
      *                              or unsupported.
      *              0               the signal was successfully sent.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  ptw32_thread_t * tp;

  EnterCriticalSection (&ptw32_thread_reuse_lock);

  tp = (ptw32_thread_t *) thread.p;

  if (NULL == tp
      || thread.x != tp->ptHandle.x
      || NULL == tp->threadH)
    {
      result = ESRCH;
    }

  LeaveCriticalSection (&ptw32_thread_reuse_lock);

  if (0 == result && 0 != sig)
    {
      /*
       * Currently does not support any signals.
       */
      result = EINVAL;
    }

  return result;

}				/* pthread_kill */

// ===========================SOURCE FILE BREAK===============================



static void PTW32_CDECL
ptw32_once_on_init_cancel (void * arg)
{
  /* when the initting thread is cancelled we have to release the lock */
  ptw32_mcs_local_node_t *node = (ptw32_mcs_local_node_t *)arg;
  ptw32_mcs_lock_release(node);
}

int
pthread_once (pthread_once_t * once_control, void (*init_routine) (void))
{
  if (once_control == NULL || init_routine == NULL)
    {
      return EINVAL;
    }
  
  if (!InterlockedExchangeAdd((LPLONG)&once_control->done, 0)) /* MBR fence */
    {
      ptw32_mcs_local_node_t node;

      ptw32_mcs_lock_acquire((ptw32_mcs_lock_t *)&once_control->lock, &node);

      if (!once_control->done)
	{

#ifdef _MSC_VER
#pragma inline_depth(0)
#endif

	  pthread_cleanup_push(ptw32_once_on_init_cancel, (void *)&node);
	  (*init_routine)();
	  pthread_cleanup_pop(0);

#ifdef _MSC_VER
#pragma inline_depth()
#endif

	  once_control->done = PTW32_TRUE;
	}

	ptw32_mcs_lock_release(&node);
    }

  return 0;

}				/* pthread_once */

// ===========================SOURCE FILE BREAK===============================

pthread_t pthread_self (void)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function returns a reference to the current running
      *      thread.
      *
      * PARAMETERS
      *      N/A
      *
      *
      * DESCRIPTION
      *      This function returns a reference to the current running
      *      thread.
      *
      * RESULTS
      *              pthread_t       reference to the current thread
      *
      * ------------------------------------------------------
      */
{
  pthread_t self;
  pthread_t nil = {NULL, 0};
  ptw32_thread_t * sp;

#ifdef _UWIN
  if (!ptw32_selfThreadKey)
    return nil;
#endif

  sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

  if (sp != NULL)
    {
      self = sp->ptHandle;
    }
  else
    {
      /*
       * Need to create an implicit 'self' for the currently
       * executing thread.
       */
      self = ptw32_new ();
      sp = (ptw32_thread_t *) self.p;

      if (sp != NULL)
	{
	  /*
	   * This is a non-POSIX thread which has chosen to call
	   * a POSIX threads function for some reason. We assume that
	   * it isn't joinable, but we do assume that it's
	   * (deferred) cancelable.
	   */
	  sp->implicit = 1;
	  sp->detachState = PTHREAD_CREATE_DETACHED;
	  sp->thread = GetCurrentThreadId ();

#ifdef NEED_DUPLICATEHANDLE
	  /*
	   * DuplicateHandle does not exist on WinCE.
	   *
	   * NOTE:
	   * GetCurrentThread only returns a pseudo-handle
	   * which is only valid in the current thread context.
	   * Therefore, you should not pass the handle to
	   * other threads for whatever purpose.
	   */
	  sp->threadH = GetCurrentThread ();
#else
	  if (!DuplicateHandle (GetCurrentProcess (),
				GetCurrentThread (),
				GetCurrentProcess (),
				&sp->threadH,
				0, FALSE, DUPLICATE_SAME_ACCESS))
	    {
	      /*
	       * Should not do this, but we have no alternative if
	       * we can't get a Win32 thread handle.
	       * Thread structs are never freed.
	       */
	      ptw32_threadReusePush (self);
	      return nil;
	    }
#endif

	  /*
	   * No need to explicitly serialise access to sched_priority
	   * because the new handle is not yet public.
	   */
	  sp->sched_priority = GetThreadPriority (sp->threadH);

          pthread_setspecific (ptw32_selfThreadKey, (void *) sp);
	}
    }

  return (self);

}				/* pthread_self */

// ===========================SOURCE FILE BREAK===============================


int
pthread_equal (pthread_t t1, pthread_t t2)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function returns nonzero if t1 and t2 are equal, else
      *      returns nonzero
      *
      * PARAMETERS
      *      t1,
      *      t2
      *              thread IDs
      *
      *
      * DESCRIPTION
      *      This function returns nonzero if t1 and t2 are equal, else
      *      returns zero.
      *
      * RESULTS
      *              non-zero        if t1 and t2 refer to the same thread,
      *              0               t1 and t2 do not refer to the same thread
      *
      * ------------------------------------------------------
      */
{
  int result;

  /*
   * We also accept NULL == NULL - treating NULL as a thread
   * for this special case, because there is no error that we can return.
   */
  result = ( t1.p == t2.p && t1.x == t2.x );

  return (result);

}				/* pthread_equal */

// ===========================SOURCE FILE BREAK===============================


int
pthread_setconcurrency (int level)
{
  if (level < 0)
    {
      return EINVAL;
    }
  else
    {
      ptw32_concurrency = level;
      return 0;
    }
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_getconcurrency (void)
{
  return ptw32_concurrency;
}
// ===========================SOURCE FILE BREAK===============================

pthread_t
ptw32_new (void)
{
  pthread_t t;
  pthread_t nil = {NULL, 0};
  ptw32_thread_t * tp;

  /*
   * If there's a reusable pthread_t then use it.
   */
  t = ptw32_threadReusePop ();

  if (NULL != t.p)
    {
      tp = (ptw32_thread_t *) t.p;
    }
  else
    {
      /* No reuse threads available */
      tp = (ptw32_thread_t *) calloc (1, sizeof(ptw32_thread_t));

      if (tp == NULL)
	{
	  return nil;
	}

      /* ptHandle.p needs to point to it's parent ptw32_thread_t. */
      t.p = tp->ptHandle.p = tp;
      t.x = tp->ptHandle.x = 0;
    }

  /* Set default state. */
  tp->sched_priority = THREAD_PRIORITY_NORMAL;
  tp->detachState = PTHREAD_CREATE_JOINABLE;
  tp->cancelState = PTHREAD_CANCEL_ENABLE;
  tp->cancelType = PTHREAD_CANCEL_DEFERRED;
  tp->cancelLock = PTHREAD_MUTEX_INITIALIZER;
  tp->threadLock = PTHREAD_MUTEX_INITIALIZER;
  tp->cancelEvent = CreateEvent (0, (int) PTW32_TRUE,	/* manualReset  */
				 (int) PTW32_FALSE,	/* setSignaled  */
				 NULL);

  if (tp->cancelEvent == NULL)
    {
      ptw32_threadReusePush (tp->ptHandle);
      return nil;
    }

  return t;

}
// ===========================SOURCE FILE BREAK===============================

#ifdef NEED_CALLOC
void *
ptw32_calloc (size_t n, size_t s)
{
  unsigned int m = n * s;
  void *p;

  p = malloc (m);
  if (p == NULL)
    return NULL;

  memset (p, 0, m);

  return p;
}
#endif
// ===========================SOURCE FILE BREAK===============================

/*
 * How it works:
 * A pthread_t is a struct (2x32 bit scalar types on IA-32, 2x64 bit on IA-64)
 * which is normally passed/returned by value to/from pthreads routines.
 * Applications are therefore storing a copy of the struct as it is at that
 * time.
 *
 * The original pthread_t struct plus all copies of it contain the address of
 * the thread state struct ptw32_thread_t_ (p), plus a reuse counter (x). Each
 * ptw32_thread_t contains the original copy of it's pthread_t.
 * Once malloced, a ptw32_thread_t_ struct is not freed until the process exits.
 * 
 * The thread reuse stack is a simple LILO stack managed through a singly
 * linked list element in the ptw32_thread_t.
 *
 * Each time a thread is destroyed, the ptw32_thread_t address is pushed onto the
 * reuse stack after it's ptHandle's reuse counter has been incremented.
 * 
 * The following can now be said from this:
 * - two pthread_t's are identical if their ptw32_thread_t reference pointers
 * are equal and their reuse counters are equal. That is,
 *
 *   equal = (a.p == b.p && a.x == b.x)
 *
 * - a pthread_t copy refers to a destroyed thread if the reuse counter in
 * the copy is not equal to the reuse counter in the original.
 *
 *   threadDestroyed = (copy.x != ((ptw32_thread_t *)copy.p)->ptHandle.x)
 *
 */

/*
 * Pop a clean pthread_t struct off the reuse stack.
 */
pthread_t
ptw32_threadReusePop (void)
{
  pthread_t t = {NULL, 0};

  EnterCriticalSection (&ptw32_thread_reuse_lock);

  if (PTW32_THREAD_REUSE_EMPTY != ptw32_threadReuseTop)
    {
      ptw32_thread_t * tp;

      tp = ptw32_threadReuseTop;

      ptw32_threadReuseTop = tp->prevReuse;

      if (PTW32_THREAD_REUSE_EMPTY == ptw32_threadReuseTop)
        {
          ptw32_threadReuseBottom = PTW32_THREAD_REUSE_EMPTY;
        }

      tp->prevReuse = NULL;

      t = tp->ptHandle;
    }

  LeaveCriticalSection (&ptw32_thread_reuse_lock);

  return t;

}

/*
 * Push a clean pthread_t struct onto the reuse stack.
 * Must be re-initialised when reused.
 * All object elements (mutexes, events etc) must have been either
 * detroyed before this, or never initialised.
 */
void
ptw32_threadReusePush (pthread_t thread)
{
  ptw32_thread_t * tp = (ptw32_thread_t *) thread.p;
  pthread_t t;

  EnterCriticalSection (&ptw32_thread_reuse_lock);

  t = tp->ptHandle;
  memset(tp, 0, sizeof(ptw32_thread_t));

  /* Must restore the original POSIX handle that we just wiped. */
  tp->ptHandle = t;

  /* Bump the reuse counter now */
#ifdef PTW32_THREAD_ID_REUSE_INCREMENT
  tp->ptHandle.x += PTW32_THREAD_ID_REUSE_INCREMENT;
#else
  tp->ptHandle.x++;
#endif

  tp->prevReuse = PTW32_THREAD_REUSE_EMPTY;

  if (PTW32_THREAD_REUSE_EMPTY != ptw32_threadReuseBottom)
    {
      ptw32_threadReuseBottom->prevReuse = tp;
    }
  else
    {
      ptw32_threadReuseTop = tp;
    }

  ptw32_threadReuseBottom = tp;

  LeaveCriticalSection (&ptw32_thread_reuse_lock);
}
// ===========================SOURCE FILE BREAK===============================

static INLINE int
ptw32_cancelable_wait (HANDLE waitHandle, DWORD timeout)
     /*
      * -------------------------------------------------------------------
      * This provides an extra hook into the pthread_cancel
      * mechanism that will allow you to wait on a Windows handle and make it a
      * cancellation point. This function blocks until the given WIN32 handle is
      * signaled or pthread_cancel has been called. It is implemented using
      * WaitForMultipleObjects on 'waitHandle' and a manually reset WIN32
      * event used to implement pthread_cancel.
      * 
      * Given this hook it would be possible to implement more of the cancellation
      * points.
      * -------------------------------------------------------------------
      */
{
  int result;
  pthread_t self;
  ptw32_thread_t * sp;
  HANDLE handles[2];
  DWORD nHandles = 1;
  DWORD status;

  handles[0] = waitHandle;

  self = pthread_self();
  sp = (ptw32_thread_t *) self.p;

  if (sp != NULL)
    {
      /*
       * Get cancelEvent handle
       */
      if (sp->cancelState == PTHREAD_CANCEL_ENABLE)
	{

	  if ((handles[1] = sp->cancelEvent) != NULL)
	    {
	      nHandles++;
	    }
	}
    }
  else
    {
      handles[1] = NULL;
    }

  status = WaitForMultipleObjects (nHandles, handles, PTW32_FALSE, timeout);

  switch (status - WAIT_OBJECT_0)
    {
    case 0:
      /*
       * Got the handle.
       * In the event that both handles are signalled, the smallest index
       * value (us) is returned. As it has been arranged, this ensures that
       * we don't drop a signal that we should act on (i.e. semaphore,
       * mutex, or condition variable etc).
       */
      result = 0;
      break;

    case 1:
      /*
       * Got cancel request.
       * In the event that both handles are signaled, the cancel will
       * be ignored (see case 0 comment).
       */
      ResetEvent (handles[1]);

      if (sp != NULL)
	{
	  /*
	   * Should handle POSIX and implicit POSIX threads..
	   * Make sure we haven't been async-canceled in the meantime.
	   */
	  (void) pthread_mutex_lock (&sp->cancelLock);
	  if (sp->state < PThreadStateCanceling)
	    {
	      sp->state = PThreadStateCanceling;
	      sp->cancelState = PTHREAD_CANCEL_DISABLE;
	      (void) pthread_mutex_unlock (&sp->cancelLock);
	      ptw32_throw (PTW32_EPS_CANCEL);

	      /* Never reached */
	    }
	  (void) pthread_mutex_unlock (&sp->cancelLock);
	}

      /* Should never get to here. */
      result = EINVAL;
      break;

    default:
      if (status == WAIT_TIMEOUT)
	{
	  result = ETIMEDOUT;
	}
      else
	{
	  result = EINVAL;
	}
      break;
    }

  return (result);

}				/* CancelableWait */

int
pthreadCancelableWait (HANDLE waitHandle)
{
  return (ptw32_cancelable_wait (waitHandle, INFINITE));
}

int
pthreadCancelableTimedWait (HANDLE waitHandle, DWORD timeout)
{
  return (ptw32_cancelable_wait (waitHandle, timeout));
}
// ===========================SOURCE FILE BREAK===============================


//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "mutex.c"
//
#ifndef _UWIN
#ifndef WINCE
 #include <process.h>
#endif
#endif
#ifndef NEED_FTIME
#include <sys/timeb.h>
#endif

//#include "ptw32_mutex_check_need_init.c"
//#include "pthread_mutex_init.c"
//#include "pthread_mutex_destroy.c"
//#include "pthread_mutexattr_init.c"
//#include "pthread_mutexattr_destroy.c"
//#include "pthread_mutexattr_getpshared.c"
//#include "pthread_mutexattr_setpshared.c"
//#include "pthread_mutexattr_settype.c"
//#include "pthread_mutexattr_gettype.c"
//#include "pthread_mutex_lock.c"
//#include "pthread_mutex_timedlock.c"
//#include "pthread_mutex_unlock.c"
//#include "pthread_mutex_trylock.c"

static struct pthread_mutexattr_t_ ptw32_recursive_mutexattr_s =
  {PTHREAD_PROCESS_PRIVATE, PTHREAD_MUTEX_RECURSIVE};
static struct pthread_mutexattr_t_ ptw32_errorcheck_mutexattr_s =
  {PTHREAD_PROCESS_PRIVATE, PTHREAD_MUTEX_ERRORCHECK};
static pthread_mutexattr_t ptw32_recursive_mutexattr = &ptw32_recursive_mutexattr_s;
static pthread_mutexattr_t ptw32_errorcheck_mutexattr = &ptw32_errorcheck_mutexattr_s;


INLINE int
ptw32_mutex_check_need_init (pthread_mutex_t * mutex)
{
  register int result = 0;
  register pthread_mutex_t mtx;

  /*
   * The following guarded test is specifically for statically
   * initialised mutexes (via PTHREAD_MUTEX_INITIALIZER).
   *
   * Note that by not providing this synchronisation we risk
   * introducing race conditions into applications which are
   * correctly written.
   *
   * Approach
   * --------
   * We know that static mutexes will not be PROCESS_SHARED
   * so we can serialise access to internal state using
   * Win32 Critical Sections rather than Win32 Mutexes.
   *
   * If using a single global lock slows applications down too much,
   * multiple global locks could be created and hashed on some random
   * value associated with each mutex, the pointer perhaps. At a guess,
   * a good value for the optimal number of global locks might be
   * the number of processors + 1.
   *
   */
  EnterCriticalSection (&ptw32_mutex_test_init_lock);

  /*
   * We got here possibly under race
   * conditions. Check again inside the critical section
   * and only initialise if the mutex is valid (not been destroyed).
   * If a static mutex has been destroyed, the application can
   * re-initialise it only by calling pthread_mutex_init()
   * explicitly.
   */
  mtx = *mutex;

  if (mtx == PTHREAD_MUTEX_INITIALIZER)
    {
      result = pthread_mutex_init (mutex, NULL);
    }
  else if (mtx == PTHREAD_RECURSIVE_MUTEX_INITIALIZER)
    {
      result = pthread_mutex_init (mutex, &ptw32_recursive_mutexattr);
    }
  else if (mtx == PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
    {
      result = pthread_mutex_init (mutex, &ptw32_errorcheck_mutexattr);
    }
  else if (mtx == NULL)
    {
      /*
       * The mutex has been destroyed while we were waiting to
       * initialise it, so the operation that caused the
       * auto-initialisation should fail.
       */
      result = EINVAL;
    }

  LeaveCriticalSection (&ptw32_mutex_test_init_lock);

  return (result);
}

// ===========================SOURCE FILE BREAK===============================

int
pthread_mutex_init (pthread_mutex_t * mutex, const pthread_mutexattr_t * attr)
{
  int result = 0;
  pthread_mutex_t mx;

  if (mutex == NULL)
    {
      return EINVAL;
    }

  if (attr != NULL
      && *attr != NULL && (*attr)->pshared == PTHREAD_PROCESS_SHARED)
    {
      /*
       * Creating mutex that can be shared between
       * processes.
       */
#if _POSIX_THREAD_PROCESS_SHARED >= 0

      /*
       * Not implemented yet.
       */

#error ERROR [__FILE__, line __LINE__]: Process shared mutexes are not supported yet.

#else

      return ENOSYS;

#endif /* _POSIX_THREAD_PROCESS_SHARED */

    }

  mx = (pthread_mutex_t) calloc (1, sizeof (*mx));

  if (mx == NULL)
    {
      result = ENOMEM;
    }
  else
    {
      mx->lock_idx = 0;
      mx->recursive_count = 0;
      mx->kind = (attr == NULL || *attr == NULL
		  ? PTHREAD_MUTEX_DEFAULT : (*attr)->kind);
      mx->ownerThread.p = NULL;

      mx->event = CreateEvent (NULL, PTW32_FALSE,    /* manual reset = No */
                              PTW32_FALSE,           /* initial state = not signaled */
                              NULL);                 /* event name */

      if (0 == mx->event)
        {
          result = ENOSPC;
          free (mx);
          mx = NULL;
        }
    }

  *mutex = mx;

  return (result);
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_mutex_destroy (pthread_mutex_t * mutex)
{
  int result = 0;
  pthread_mutex_t mx;

  /*
   * Let the system deal with invalid pointers.
   */

  /*
   * Check to see if we have something to delete.
   */
  if (*mutex < PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
    {
      mx = *mutex;

      result = pthread_mutex_trylock (&mx);

      /*
       * If trylock succeeded and the mutex is not recursively locked it
       * can be destroyed.
       */
      if (result == 0)
	{
	  if (mx->kind != PTHREAD_MUTEX_RECURSIVE || 1 == mx->recursive_count)
	    {
	      /*
	       * FIXME!!!
	       * The mutex isn't held by another thread but we could still
	       * be too late invalidating the mutex below since another thread
	       * may already have entered mutex_lock and the check for a valid
	       * *mutex != NULL.
	       *
	       * Note that this would be an unusual situation because it is not
	       * common that mutexes are destroyed while they are still in
	       * use by other threads.
	       */
	      *mutex = NULL;

	      result = pthread_mutex_unlock (&mx);

	      if (result == 0)
		{
		  if (!CloseHandle (mx->event))
		    {
		      *mutex = mx;
		      result = EINVAL;
		    }
		  else
		    {
		      free (mx);
		    }
		}
	      else
		{
		  /*
		   * Restore the mutex before we return the error.
		   */
		  *mutex = mx;
		}
	    }
	  else			/* mx->recursive_count > 1 */
	    {
	      /*
	       * The mutex must be recursive and already locked by us (this thread).
	       */
	      mx->recursive_count--;	/* Undo effect of pthread_mutex_trylock() above */
	      result = EBUSY;
	    }
	}
    }
  else
    {
      /*
       * See notes in ptw32_mutex_check_need_init() above also.
       */
      EnterCriticalSection (&ptw32_mutex_test_init_lock);

      /*
       * Check again.
       */
      if (*mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
	{
	  /*
	   * This is all we need to do to destroy a statically
	   * initialised mutex that has not yet been used (initialised).
	   * If we get to here, another thread
	   * waiting to initialise this mutex will get an EINVAL.
	   */
	  *mutex = NULL;
	}
      else
	{
	  /*
	   * The mutex has been initialised while we were waiting
	   * so assume it's in use.
	   */
	  result = EBUSY;
	}

      LeaveCriticalSection (&ptw32_mutex_test_init_lock);
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_mutexattr_init (pthread_mutexattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Initializes a mutex attributes object with default
      *      attributes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *
      * DESCRIPTION
      *      Initializes a mutex attributes object with default
      *      attributes.
      *
      *      NOTES:
      *              1)      Used to define mutex types
      *
      * RESULTS
      *              0               successfully initialized attr,
      *              ENOMEM          insufficient memory for attr.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  pthread_mutexattr_t ma;

  ma = (pthread_mutexattr_t) calloc (1, sizeof (*ma));

  if (ma == NULL)
    {
      result = ENOMEM;
    }
  else
    {
      ma->pshared = PTHREAD_PROCESS_PRIVATE;
      ma->kind = PTHREAD_MUTEX_DEFAULT;
    }

  *attr = ma;

  return (result);
}				/* pthread_mutexattr_init */

// ===========================SOURCE FILE BREAK===============================


int
pthread_mutexattr_destroy (pthread_mutexattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Destroys a mutex attributes object. The object can
      *      no longer be used.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *
      * DESCRIPTION
      *      Destroys a mutex attributes object. The object can
      *      no longer be used.
      *
      *      NOTES:
      *              1)      Does not affect mutexes created using 'attr'
      *
      * RESULTS
      *              0               successfully released attr,
      *              EINVAL          'attr' is invalid.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (attr == NULL || *attr == NULL)
    {
      result = EINVAL;
    }
  else
    {
      pthread_mutexattr_t ma = *attr;

      *attr = NULL;
      free (ma);
    }

  return (result);
}				/* pthread_mutexattr_destroy */
// ===========================SOURCE FILE BREAK===============================

int
pthread_mutexattr_getpshared (const pthread_mutexattr_t * attr, int *pshared)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Determine whether mutexes created with 'attr' can be
      *      shared between processes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *      pshared
      *              will be set to one of:
      *
      *                      PTHREAD_PROCESS_SHARED
      *                              May be shared if in shared memory
      *
      *                      PTHREAD_PROCESS_PRIVATE
      *                              Cannot be shared.
      *
      *
      * DESCRIPTION
      *      Mutexes creatd with 'attr' can be shared between
      *      processes if pthread_mutex_t variable is allocated
      *      in memory shared by these processes.
      *      NOTES:
      *              1)      pshared mutexes MUST be allocated in shared
      *                      memory.
      *              2)      The following macro is defined if shared mutexes
      *                      are supported:
      *                              _POSIX_THREAD_PROCESS_SHARED
      *
      * RESULTS
      *              0               successfully retrieved attribute,
      *              EINVAL          'attr' is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result;

  if ((attr != NULL && *attr != NULL) && (pshared != NULL))
    {
      *pshared = (*attr)->pshared;
      result = 0;
    }
  else
    {
      result = EINVAL;
    }

  return (result);

}				/* pthread_mutexattr_getpshared */

// ===========================SOURCE FILE BREAK===============================

int
pthread_mutexattr_setpshared (pthread_mutexattr_t * attr, int pshared)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Mutexes created with 'attr' can be shared between
      *      processes if pthread_mutex_t variable is allocated
      *      in memory shared by these processes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *      pshared
      *              must be one of:
      *
      *                      PTHREAD_PROCESS_SHARED
      *                              May be shared if in shared memory
      *
      *                      PTHREAD_PROCESS_PRIVATE
      *                              Cannot be shared.
      *
      * DESCRIPTION
      *      Mutexes creatd with 'attr' can be shared between
      *      processes if pthread_mutex_t variable is allocated
      *      in memory shared by these processes.
      *
      *      NOTES:
      *              1)      pshared mutexes MUST be allocated in shared
      *                      memory.
      *
      *              2)      The following macro is defined if shared mutexes
      *                      are supported:
      *                              _POSIX_THREAD_PROCESS_SHARED
      *
      * RESULTS
      *              0               successfully set attribute,
      *              EINVAL          'attr' or pshared is invalid,
      *              ENOSYS          PTHREAD_PROCESS_SHARED not supported,
      *
      * ------------------------------------------------------
      */
{
  int result;

  if ((attr != NULL && *attr != NULL) &&
      ((pshared == PTHREAD_PROCESS_SHARED) ||
       (pshared == PTHREAD_PROCESS_PRIVATE)))
    {
      if (pshared == PTHREAD_PROCESS_SHARED)
	{

#if !defined( _POSIX_THREAD_PROCESS_SHARED )

	  result = ENOSYS;
	  pshared = PTHREAD_PROCESS_PRIVATE;

#else

	  result = 0;

#endif /* _POSIX_THREAD_PROCESS_SHARED */

	}
      else
	{
	  result = 0;
	}

      (*attr)->pshared = pshared;
    }
  else
    {
      result = EINVAL;
    }

  return (result);

}				/* pthread_mutexattr_setpshared */
// ===========================SOURCE FILE BREAK===============================

int
pthread_mutexattr_settype (pthread_mutexattr_t * attr, int kind)
     /*
      * ------------------------------------------------------
      *
      * DOCPUBLIC
      * The pthread_mutexattr_settype() and
      * pthread_mutexattr_gettype() functions  respectively set and
      * get the mutex type  attribute. This attribute is set in  the
      * type parameter to these functions.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_mutexattr_t
      *
      *      type
      *              must be one of:
      *
      *                      PTHREAD_MUTEX_DEFAULT
      *
      *                      PTHREAD_MUTEX_NORMAL
      *
      *                      PTHREAD_MUTEX_ERRORCHECK
      *
      *                      PTHREAD_MUTEX_RECURSIVE
      *
      * DESCRIPTION
      * The pthread_mutexattr_settype() and
      * pthread_mutexattr_gettype() functions  respectively set and
      * get the mutex type  attribute. This attribute is set in  the
      * type  parameter to these functions. The default value of the
      * type  attribute is  PTHREAD_MUTEX_DEFAULT.
      * 
      * The type of mutex is contained in the type  attribute of the
      * mutex attributes. Valid mutex types include:
      *
      * PTHREAD_MUTEX_NORMAL
      *          This type of mutex does  not  detect  deadlock.  A
      *          thread  attempting  to  relock  this mutex without
      *          first unlocking it will  deadlock.  Attempting  to
      *          unlock  a  mutex  locked  by  a  different  thread
      *          results  in  undefined  behavior.  Attempting   to
      *          unlock  an  unlocked  mutex  results  in undefined
      *          behavior.
      * 
      * PTHREAD_MUTEX_ERRORCHECK
      *          This type of  mutex  provides  error  checking.  A
      *          thread  attempting  to  relock  this mutex without
      *          first unlocking it will return with  an  error.  A
      *          thread  attempting to unlock a mutex which another
      *          thread has locked will return  with  an  error.  A
      *          thread attempting to unlock an unlocked mutex will
      *          return with an error.
      *
      * PTHREAD_MUTEX_DEFAULT
      *          Same as PTHREAD_MUTEX_NORMAL.
      * 
      * PTHREAD_MUTEX_RECURSIVE
      *          A thread attempting to relock this  mutex  without
      *          first  unlocking  it  will  succeed in locking the
      *          mutex. The relocking deadlock which can occur with
      *          mutexes of type  PTHREAD_MUTEX_NORMAL cannot occur
      *          with this type of mutex. Multiple  locks  of  this
      *          mutex  require  the  same  number  of  unlocks  to
      *          release  the  mutex  before  another  thread   can
      *          acquire the mutex. A thread attempting to unlock a
      *          mutex which another thread has locked will  return
      *          with  an  error. A thread attempting to  unlock an
      *          unlocked mutex will return  with  an  error.  This
      *          type  of mutex is only supported for mutexes whose
      *          process        shared         attribute         is
      *          PTHREAD_PROCESS_PRIVATE.
      *
      * RESULTS
      *              0               successfully set attribute,
      *              EINVAL          'attr' or 'type' is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if ((attr != NULL && *attr != NULL))
    {
      switch (kind)
	{
	case PTHREAD_MUTEX_FAST_NP:
	case PTHREAD_MUTEX_RECURSIVE_NP:
	case PTHREAD_MUTEX_ERRORCHECK_NP:
	  (*attr)->kind = kind;
	  break;
	default:
	  result = EINVAL;
	  break;
	}
    }
  else
    {
      result = EINVAL;
    }

  return (result);
}				/* pthread_mutexattr_settype */
// ===========================SOURCE FILE BREAK===============================



int
pthread_mutexattr_gettype (pthread_mutexattr_t * attr, int *kind)
{
  int result = 0;

  if (attr != NULL && *attr != NULL && kind != NULL)
    {
      *kind = (*attr)->kind;
    }
  else
    {
      result = EINVAL;
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================


int
pthread_mutex_lock (pthread_mutex_t * mutex)
{
  int result = 0;
  pthread_mutex_t mx;

  /*
   * Let the system deal with invalid pointers.
   */
  if (*mutex == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static mutex. We check
   * again inside the guarded section of ptw32_mutex_check_need_init()
   * to avoid race conditions.
   */
  if (*mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
    {
      if ((result = ptw32_mutex_check_need_init (mutex)) != 0)
	{
	  return (result);
	}
    }

  mx = *mutex;

  if (mx->kind == PTHREAD_MUTEX_NORMAL)
    {
      if ((LONG) PTW32_INTERLOCKED_EXCHANGE(
		   (LPLONG) &mx->lock_idx,
		   (LONG) 1) != 0)
	{
	  while ((LONG) PTW32_INTERLOCKED_EXCHANGE(
                          (LPLONG) &mx->lock_idx,
			  (LONG) -1) != 0)
	    {
	      if (WAIT_OBJECT_0 != WaitForSingleObject (mx->event, INFINITE))
	        {
	          result = EINVAL;
		  break;
	        }
	    }
	}
    }
  else
    {
      pthread_t self = pthread_self();

      if ((PTW32_INTERLOCKED_LONG) PTW32_INTERLOCKED_COMPARE_EXCHANGE(
                   (PTW32_INTERLOCKED_LPLONG) &mx->lock_idx,
		   (PTW32_INTERLOCKED_LONG) 1,
		   (PTW32_INTERLOCKED_LONG) 0) == 0)
	{
	  mx->recursive_count = 1;
	  mx->ownerThread = self;
	}
      else
	{
	  if (pthread_equal (mx->ownerThread, self))
	    {
	      if (mx->kind == PTHREAD_MUTEX_RECURSIVE)
		{
		  mx->recursive_count++;
		}
	      else
		{
		  result = EDEADLK;
		}
	    }
	  else
	    {
	      while ((LONG) PTW32_INTERLOCKED_EXCHANGE(
                              (LPLONG) &mx->lock_idx,
			      (LONG) -1) != 0)
		{
	          if (WAIT_OBJECT_0 != WaitForSingleObject (mx->event, INFINITE))
		    {
	              result = EINVAL;
		      break;
		    }
		}

	      if (0 == result)
		{
		  mx->recursive_count = 1;
		  mx->ownerThread = self;
		}
	    }
	}
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================


static INLINE int
ptw32_timed_eventwait (HANDLE event, const struct timespec *abstime)
     /*
      * ------------------------------------------------------
      * DESCRIPTION
      *      This function waits on an event until signaled or until
      *      abstime passes.
      *      If abstime has passed when this routine is called then
      *      it returns a result to indicate this.
      *
      *      If 'abstime' is a NULL pointer then this function will
      *      block until it can successfully decrease the value or
      *      until interrupted by a signal.
      *
      *      This routine is not a cancelation point.
      *
      * RESULTS
      *              0               successfully signaled,
      *              ETIMEDOUT       abstime passed
      *              EINVAL          'event' is not a valid event,
      *
      * ------------------------------------------------------
      */
{

  DWORD milliseconds;
  DWORD status;

  if (event == NULL)
    {
      return EINVAL;
    }
  else
    {
      if (abstime == NULL)
	{
	  milliseconds = INFINITE;
	}
      else
	{
	  /* 
	   * Calculate timeout as milliseconds from current system time. 
	   */
	  milliseconds = ptw32_relmillisecs (abstime);
	}

      status = WaitForSingleObject (event, milliseconds);

      if (status == WAIT_OBJECT_0)
	{
	  return 0;
	}
      else if (status == WAIT_TIMEOUT)
	{
	  return ETIMEDOUT;
	}
      else
	{
	  return EINVAL;
	}
    }

  return 0;

}				/* ptw32_timed_semwait */


int
pthread_mutex_timedlock (pthread_mutex_t * mutex,
			 const struct timespec *abstime)
{
  int result;
  pthread_mutex_t mx;

  /*
   * Let the system deal with invalid pointers.
   */

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static mutex. We check
   * again inside the guarded section of ptw32_mutex_check_need_init()
   * to avoid race conditions.
   */
  if (*mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
    {
      if ((result = ptw32_mutex_check_need_init (mutex)) != 0)
	{
	  return (result);
	}
    }

  mx = *mutex;

  if (mx->kind == PTHREAD_MUTEX_NORMAL)
    {
      if ((LONG) PTW32_INTERLOCKED_EXCHANGE(
		   (LPLONG) &mx->lock_idx,
		   (LONG) 1) != 0)
	{
          while ((LONG) PTW32_INTERLOCKED_EXCHANGE(
                          (LPLONG) &mx->lock_idx,
			  (LONG) -1) != 0)
            {
	      if (0 != (result = ptw32_timed_eventwait (mx->event, abstime)))
		{
		  return result;
		}
	    }
	}
    }
  else
    {
      pthread_t self = pthread_self();

      if ((PTW32_INTERLOCKED_LONG) PTW32_INTERLOCKED_COMPARE_EXCHANGE(
                   (PTW32_INTERLOCKED_LPLONG) &mx->lock_idx,
		   (PTW32_INTERLOCKED_LONG) 1,
		   (PTW32_INTERLOCKED_LONG) 0) == 0)
	{
	  mx->recursive_count = 1;
	  mx->ownerThread = self;
	}
      else
	{
	  if (pthread_equal (mx->ownerThread, self))
	    {
	      if (mx->kind == PTHREAD_MUTEX_RECURSIVE)
		{
		  mx->recursive_count++;
		}
	      else
		{
		  return EDEADLK;
		}
	    }
	  else
	    {
              while ((LONG) PTW32_INTERLOCKED_EXCHANGE(
                              (LPLONG) &mx->lock_idx,
			      (LONG) -1) != 0)
                {
		  if (0 != (result = ptw32_timed_eventwait (mx->event, abstime)))
		    {
		      return result;
		    }
		}

	      mx->recursive_count = 1;
	      mx->ownerThread = self;
	    }
	}
    }

  return 0;
}
// ===========================SOURCE FILE BREAK===============================


int
pthread_mutex_unlock (pthread_mutex_t * mutex)
{
  int result = 0;
  pthread_mutex_t mx;

  /*
   * Let the system deal with invalid pointers.
   */

  mx = *mutex;

  /*
   * If the thread calling us holds the mutex then there is no
   * race condition. If another thread holds the
   * lock then we shouldn't be in here.
   */
  if (mx < PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
    {
      if (mx->kind == PTHREAD_MUTEX_NORMAL)
	{
	  LONG idx;

	  idx = (LONG) PTW32_INTERLOCKED_EXCHANGE ((LPLONG) &mx->lock_idx,
						   (LONG) 0);
	  if (idx != 0)
	    {
	      if (idx < 0)
		{
		  /*
		   * Someone may be waiting on that mutex.
		   */
		  if (SetEvent (mx->event) == 0)
		    {
		      result = EINVAL;
		    }
		}
	    }
	  else
	    {
	      /*
	       * Was not locked (so can't be owned by us).
	       */
	      result = EPERM;
	    }
	}
      else
	{
	  if (pthread_equal (mx->ownerThread, pthread_self ()))
	    {
	      if (mx->kind != PTHREAD_MUTEX_RECURSIVE
		  || 0 == --mx->recursive_count)
		{
		  mx->ownerThread.p = NULL;

		  if ((LONG) PTW32_INTERLOCKED_EXCHANGE ((LPLONG) &mx->lock_idx,
							 (LONG) 0) < 0)
		    {
		      /* Someone may be waiting on that mutex */
		      if (SetEvent (mx->event) == 0)
			{
			  result = EINVAL;
			}
		    }
		}
	    }
	  else
	    {
	      result = EPERM;
	    }
	}
    }
  else
    {
      result = EINVAL;
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================



int
pthread_mutex_trylock (pthread_mutex_t * mutex)
{
  int result = 0;
  pthread_mutex_t mx;

  /*
   * Let the system deal with invalid pointers.
   */

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static mutex. We check
   * again inside the guarded section of ptw32_mutex_check_need_init()
   * to avoid race conditions.
   */
  if (*mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER)
    {
      if ((result = ptw32_mutex_check_need_init (mutex)) != 0)
	{
	  return (result);
	}
    }

  mx = *mutex;

  if (0 == (LONG) PTW32_INTERLOCKED_COMPARE_EXCHANGE (
		     (PTW32_INTERLOCKED_LPLONG) &mx->lock_idx,
		     (PTW32_INTERLOCKED_LONG) 1,
		     (PTW32_INTERLOCKED_LONG) 0))
    {
      if (mx->kind != PTHREAD_MUTEX_NORMAL)
	{
	  mx->recursive_count = 1;
	  mx->ownerThread = pthread_self ();
	}
    }
  else
    {
      if (mx->kind == PTHREAD_MUTEX_RECURSIVE &&
	  pthread_equal (mx->ownerThread, pthread_self ()))
	{
	  mx->recursive_count++;
	}
      else
	{
	  result = EBUSY;
	}
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================
//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "nonportable.c"
//
//#include "pthread_mutexattr_setkind_np.c"
//#include "pthread_mutexattr_getkind_np.c"
//#include "pthread_getw32threadhandle_np.c"
//#include "pthread_delay_np.c"
//#include "pthread_num_processors_np.c"
//#include "pthread_win32_attach_detach_np.c"
//#include "pthread_timechange_handler_np.c"

int
pthread_mutexattr_setkind_np (pthread_mutexattr_t * attr, int kind)
{
  return pthread_mutexattr_settype (attr, kind);
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_mutexattr_getkind_np (pthread_mutexattr_t * attr, int *kind)
{
  return pthread_mutexattr_gettype (attr, kind);
}
// ===========================SOURCE FILE BREAK===============================


/*
 * pthread_getw32threadhandle_np()
 *
 * Returns the win32 thread handle that the POSIX
 * thread "thread" is running as.
 *
 * Applications can use the win32 handle to set
 * win32 specific attributes of the thread.
 */
HANDLE
pthread_getw32threadhandle_np (pthread_t thread)
{
  return ((ptw32_thread_t *)thread.p)->threadH;
}

// ===========================SOURCE FILE BREAK===============================

/*
 * pthread_delay_np
 *
 * DESCRIPTION
 *
 *       This routine causes a thread to delay execution for a specific period of time.
 *       This period ends at the current time plus the specified interval. The routine
 *       will not return before the end of the period is reached, but may return an
 *       arbitrary amount of time after the period has gone by. This can be due to
 *       system load, thread priorities, and system timer granularity. 
 *
 *       Specifying an interval of zero (0) seconds and zero (0) nanoseconds is
 *       allowed and can be used to force the thread to give up the processor or to
 *       deliver a pending cancelation request. 
 *
 *       The timespec structure contains the following two fields: 
 *
 *            tv_sec is an integer number of seconds. 
 *            tv_nsec is an integer number of nanoseconds. 
 *
 *  Return Values
 *
 *  If an error condition occurs, this routine returns an integer value indicating
 *  the type of error. Possible return values are as follows: 
 *
 *  0 
 *           Successful completion.
 *  [EINVAL] 
 *           The value specified by interval is invalid. 
 *
 * Example
 *
 * The following code segment would wait for 5 and 1/2 seconds
 *
 *  struct timespec tsWait;
 *  int      intRC;
 *
 *  tsWait.tv_sec  = 5;
 *  tsWait.tv_nsec = 500000000L;
 *  intRC = pthread_delay_np(&tsWait);
 */
int
pthread_delay_np (struct timespec *interval)
{
  DWORD wait_time;
  DWORD secs_in_millisecs;
  DWORD millisecs;
  DWORD status;
  pthread_t self;
  ptw32_thread_t * sp;

  if (interval == NULL)
    {
      return EINVAL;
    }

  if (interval->tv_sec == 0L && interval->tv_nsec == 0L)
    {
      pthread_testcancel ();
      Sleep (0);
      pthread_testcancel ();
      return (0);
    }

  /* convert secs to millisecs */
  secs_in_millisecs = interval->tv_sec * 1000L;

  /* convert nanosecs to millisecs (rounding up) */
  millisecs = (interval->tv_nsec + 999999L) / 1000000L;

#if defined(__WATCOMC__)
#pragma disable_message (124)
#endif

  /*
   * Most compilers will issue a warning 'comparison always 0'
   * because the variable type is unsigned, but we need to keep this
   * for some reason I can't recall now.
   */
  if (0 > (wait_time = secs_in_millisecs + millisecs))
    {
      return EINVAL;
    }

#if defined(__WATCOMC__)
#pragma enable_message (124)
#endif

  if (NULL == (self = pthread_self ()).p)
    {
      return ENOMEM;
    }

  sp = (ptw32_thread_t *) self.p;

  if (sp->cancelState == PTHREAD_CANCEL_ENABLE)
    {
      /*
       * Async cancelation won't catch us until wait_time is up.
       * Deferred cancelation will cancel us immediately.
       */
      if (WAIT_OBJECT_0 ==
	  (status = WaitForSingleObject (sp->cancelEvent, wait_time)))
	{
	  /*
	   * Canceling!
	   */
	  (void) pthread_mutex_lock (&sp->cancelLock);
	  if (sp->state < PThreadStateCanceling)
	    {
	      sp->state = PThreadStateCanceling;
	      sp->cancelState = PTHREAD_CANCEL_DISABLE;
	      (void) pthread_mutex_unlock (&sp->cancelLock);

	      ptw32_throw (PTW32_EPS_CANCEL);
	    }

	  (void) pthread_mutex_unlock (&sp->cancelLock);
	  return ESRCH;
	}
      else if (status != WAIT_TIMEOUT)
	{
	  return EINVAL;
	}
    }
  else
    {
      Sleep (wait_time);
    }

  return (0);
}
// ===========================SOURCE FILE BREAK===============================

/*
 * pthread_num_processors_np()
 *
 * Get the number of CPUs available to the process.
 */
int
pthread_num_processors_np (void)
{
  int count;

  if (ptw32_getprocessors (&count) != 0)
    {
      count = 1;
    }

  return (count);
}
// ===========================SOURCE FILE BREAK===============================


/*
 * Handle to kernel32.dll 
 */
static HINSTANCE ptw32_h_kernel32;

/*
 * Handle to quserex.dll 
 */
static HINSTANCE ptw32_h_quserex;

BOOL
pthread_win32_process_attach_np ()
{
  BOOL result = TRUE;
  DWORD_PTR vProcessCPUs;
  DWORD_PTR vSystemCPUs;

  result = ptw32_processInitialize ();

#ifdef _UWIN
  pthread_count++;
#endif

  ptw32_features = 0;


#if defined(NEED_PROCESS_AFFINITY_MASK)

  ptw32_smp_system = PTW32_FALSE;

#else

  if (GetProcessAffinityMask (GetCurrentProcess (),
			      &vProcessCPUs, &vSystemCPUs))
    {
      int CPUs = 0;
      DWORD_PTR bit;

      for (bit = 1; bit != 0; bit <<= 1)
	{
	  if (vSystemCPUs & bit)
	    {
	      CPUs++;
	    }
	}
      ptw32_smp_system = (CPUs > 1);
    }
  else
    {
      ptw32_smp_system = PTW32_FALSE;
    }

#endif

#ifdef WINCE

  /*
   * Load COREDLL and try to get address of InterlockedCompareExchange
   */
  ptw32_h_kernel32 = LoadLibrary (TEXT ("COREDLL.DLL"));

#else

  /*
   * Load KERNEL32 and try to get address of InterlockedCompareExchange
   */
  ptw32_h_kernel32 = LoadLibrary (TEXT ("KERNEL32.DLL"));

#endif

  ptw32_interlocked_compare_exchange =
    (PTW32_INTERLOCKED_LONG (WINAPI *)
     (PTW32_INTERLOCKED_LPLONG, PTW32_INTERLOCKED_LONG,
      PTW32_INTERLOCKED_LONG))
#if defined(NEED_UNICODE_CONSTS)
    GetProcAddress (ptw32_h_kernel32, /*(const TCHAR *)*/ TEXT ("InterlockedCompareExchange"));
#else
    GetProcAddress (ptw32_h_kernel32, (LPCSTR) "InterlockedCompareExchange");
#endif

  if (ptw32_interlocked_compare_exchange == NULL)
    {
      ptw32_interlocked_compare_exchange = ptw32_InterlockedCompareExchange;

      /*
       * If InterlockedCompareExchange is not being used, then free
       * the kernel32.dll handle now, rather than leaving it until
       * DLL_PROCESS_DETACH.
       *
       * Note: this is not a pedantic exercise in freeing unused
       * resources!  It is a work-around for a bug in Windows 95
       * (see microsoft knowledge base article, Q187684) which
       * does Bad Things when FreeLibrary is called within
       * the DLL_PROCESS_DETACH code, in certain situations.
       * Since w95 just happens to be a platform which does not
       * provide InterlockedCompareExchange, the bug will be
       * effortlessly avoided.
       */
      (void) FreeLibrary (ptw32_h_kernel32);
      ptw32_h_kernel32 = 0;
    }
  else
    {
      ptw32_features |= PTW32_SYSTEM_INTERLOCKED_COMPARE_EXCHANGE;
    }

  /*
   * Load QUSEREX.DLL and try to get address of QueueUserAPCEx
   */
  ptw32_h_quserex = LoadLibrary (TEXT ("QUSEREX.DLL"));

  if (ptw32_h_quserex != NULL)
    {
      ptw32_register_cancelation = (DWORD (*)(PAPCFUNC, HANDLE, DWORD))
#if defined(NEED_UNICODE_CONSTS)
	GetProcAddress (ptw32_h_quserex, (const TCHAR *) TEXT ("QueueUserAPCEx"));
#else
	GetProcAddress (ptw32_h_quserex, (LPCSTR) "QueueUserAPCEx");
#endif
    }

  if (NULL == ptw32_register_cancelation)
    {
      ptw32_register_cancelation = ptw32_RegisterCancelation;

      if (ptw32_h_quserex != NULL)
	{
	  (void) FreeLibrary (ptw32_h_quserex);
	}
      ptw32_h_quserex = 0;
    }
  else
    {
      /* Initialise QueueUserAPCEx */
      BOOL (*queue_user_apc_ex_init) (VOID);

      queue_user_apc_ex_init = (BOOL (*)(VOID))
#if defined(NEED_UNICODE_CONSTS)
	GetProcAddress (ptw32_h_quserex,(const TCHAR *) TEXT ("QueueUserAPCEx_Init"));
#else
	GetProcAddress (ptw32_h_quserex, (LPCSTR) "QueueUserAPCEx_Init");
#endif

      if (queue_user_apc_ex_init == NULL || !queue_user_apc_ex_init ())
	{
	  ptw32_register_cancelation = ptw32_RegisterCancelation;

	  (void) FreeLibrary (ptw32_h_quserex);
	  ptw32_h_quserex = 0;
	}
    }

  if (ptw32_h_quserex)
    {
      ptw32_features |= PTW32_ALERTABLE_ASYNC_CANCEL;
    }

  return result;
}


BOOL
pthread_win32_process_detach_np ()
{
  if (ptw32_processInitialized)
    {
      ptw32_thread_t * sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

      if (sp != NULL)
	{
	  /*
	   * Detached threads have their resources automatically
	   * cleaned up upon exit (others must be 'joined').
	   */
	  if (sp->detachState == PTHREAD_CREATE_DETACHED)
	    {
	      ptw32_threadDestroy (sp->ptHandle);
	      TlsSetValue (ptw32_selfThreadKey->key, NULL);
	    }
	}

      /*
       * The DLL is being unmapped from the process's address space
       */
      ptw32_processTerminate ();

      if (ptw32_h_quserex)
	{
	  /* Close QueueUserAPCEx */
	  BOOL (*queue_user_apc_ex_fini) (VOID);

	  queue_user_apc_ex_fini = (BOOL (*)(VOID))
#if defined(NEED_UNICODE_CONSTS)
	    GetProcAddress (ptw32_h_quserex,   (const TCHAR *) TEXT ("QueueUserAPCEx_Fini"));
#else
	    GetProcAddress (ptw32_h_quserex, (LPCSTR) "QueueUserAPCEx_Fini");
#endif

	  if (queue_user_apc_ex_fini != NULL)
	    {
	      (void) queue_user_apc_ex_fini ();
	    }
	  (void) FreeLibrary (ptw32_h_quserex);
	}

      if (ptw32_h_kernel32)
	{
	  (void) FreeLibrary (ptw32_h_kernel32);
	}
    }

  return TRUE;
}

BOOL
pthread_win32_thread_attach_np ()
{
  return TRUE;
}

BOOL
pthread_win32_thread_detach_np ()
{
  if (ptw32_processInitialized)
    {
      /*
       * Don't use pthread_self() - to avoid creating an implicit POSIX thread handle
       * unnecessarily.
       */
      ptw32_thread_t * sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

      if (sp != NULL) // otherwise Win32 thread with no implicit POSIX handle.
	{
	  ptw32_callUserDestroyRoutines (sp->ptHandle);

	  (void) pthread_mutex_lock (&sp->cancelLock);
	  sp->state = PThreadStateLast;
	  /*
	   * If the thread is joinable at this point then it MUST be joined
	   * or detached explicitly by the application.
	   */
	  (void) pthread_mutex_unlock (&sp->cancelLock);

	  if (sp->detachState == PTHREAD_CREATE_DETACHED)
	    {
	      ptw32_threadDestroy (sp->ptHandle);

	      TlsSetValue (ptw32_selfThreadKey->key, NULL);
	    }
	}
    }

  return TRUE;
}

BOOL
pthread_win32_test_features_np (int feature_mask)
{
  return ((ptw32_features & feature_mask) == feature_mask);
}
// ===========================SOURCE FILE BREAK===============================

/*
 * Notes on handling system time adjustments (especially negative ones).
 * ---------------------------------------------------------------------
 *
 * This solution was suggested by Alexander Terekhov, but any errors
 * in the implementation are mine - [Ross Johnson]
 *
 * 1) The problem: threads doing a timedwait on a CV may expect to timeout
 *    at a specific absolute time according to a system timer. If the
 *    system clock is adjusted backwards then those threads sleep longer than
 *    expected. Also, pthreads-win32 converts absolute times to intervals in
 *    order to make use of the underlying Win32, and so waiting threads may
 *    awake before their proper abstimes.
 *
 * 2) We aren't able to distinquish between threads on timed or untimed waits,
 *    so we wake them all at the time of the adjustment so that they can
 *    re-evaluate their conditions and re-compute their timeouts.
 *
 * 3) We rely on correctly written applications for this to work. Specifically,
 *    they must be able to deal properly with spurious wakeups. That is,
 *    they must re-test their condition upon wakeup and wait again if
 *    the condition is not satisfied.
 */

void *
pthread_timechange_handler_np (void *arg)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Broadcasts all CVs to force re-evaluation and
      *      new timeouts if required.
      *
      * PARAMETERS
      *      NONE
      *
      *
      * DESCRIPTION
      *      Broadcasts all CVs to force re-evaluation and
      *      new timeouts if required.
      *
      *      This routine may be passed directly to pthread_create()
      *      as a new thread in order to run asynchronously.
      *
      *
      * RESULTS
      *              0               successfully broadcast all CVs
      *              EAGAIN          Not all CVs were broadcast
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  pthread_cond_t cv;

  EnterCriticalSection (&ptw32_cond_list_lock);

  cv = ptw32_cond_list_head;

  while (cv != NULL && 0 == result)
    {
      result = pthread_cond_broadcast (&cv);
      cv = cv->next;
    }

  LeaveCriticalSection (&ptw32_cond_list_lock);

  return (void *) (result != 0 ? EAGAIN : 0);
}

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "rwlock.c"
//
//#include "ptw32_rwlock_check_need_init.c"
//#include "ptw32_rwlock_cancelwrwait.c"
//#include "pthread_rwlock_init.c"
//#include "pthread_rwlock_destroy.c"
//#include "pthread_rwlockattr_init.c"
//#include "pthread_rwlockattr_destroy.c"
//#include "pthread_rwlockattr_getpshared.c"
//#include "pthread_rwlockattr_setpshared.c"
//#include "pthread_rwlock_rdlock.c"
//#include "pthread_rwlock_timedrdlock.c"
//#include "pthread_rwlock_wrlock.c"
//#include "pthread_rwlock_timedwrlock.c"
//#include "pthread_rwlock_unlock.c"
//#include "pthread_rwlock_tryrdlock.c"
//#include "pthread_rwlock_trywrlock.c"



INLINE int
ptw32_rwlock_check_need_init (pthread_rwlock_t * rwlock)
{
  int result = 0;

  /*
   * The following guarded test is specifically for statically
   * initialised rwlocks (via PTHREAD_RWLOCK_INITIALIZER).
   *
   * Note that by not providing this synchronisation we risk
   * introducing race conditions into applications which are
   * correctly written.
   *
   * Approach
   * --------
   * We know that static rwlocks will not be PROCESS_SHARED
   * so we can serialise access to internal state using
   * Win32 Critical Sections rather than Win32 Mutexes.
   *
   * If using a single global lock slows applications down too much,
   * multiple global locks could be created and hashed on some random
   * value associated with each mutex, the pointer perhaps. At a guess,
   * a good value for the optimal number of global locks might be
   * the number of processors + 1.
   *
   */
  EnterCriticalSection (&ptw32_rwlock_test_init_lock);

  /*
   * We got here possibly under race
   * conditions. Check again inside the critical section
   * and only initialise if the rwlock is valid (not been destroyed).
   * If a static rwlock has been destroyed, the application can
   * re-initialise it only by calling pthread_rwlock_init()
   * explicitly.
   */
  if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
    {
      result = pthread_rwlock_init (rwlock, NULL);
    }
  else if (*rwlock == NULL)
    {
      /*
       * The rwlock has been destroyed while we were waiting to
       * initialise it, so the operation that caused the
       * auto-initialisation should fail.
       */
      result = EINVAL;
    }

  LeaveCriticalSection (&ptw32_rwlock_test_init_lock);

  return result;
}
// ===========================SOURCE FILE BREAK===============================

void
ptw32_rwlock_cancelwrwait (void *arg)
{
  pthread_rwlock_t rwl = (pthread_rwlock_t) arg;

  rwl->nSharedAccessCount = -rwl->nCompletedSharedAccessCount;
  rwl->nCompletedSharedAccessCount = 0;

  (void) pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted));
  (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
}
// ===========================SOURCE FILE BREAK===============================
#ifndef WINCE
#include <errno.h>
#endif
#include <limits.h>


int
pthread_rwlock_init (pthread_rwlock_t * rwlock,
		     const pthread_rwlockattr_t * attr)
{
  int result;
  pthread_rwlock_t rwl = 0;

  if (rwlock == NULL)
    {
      return EINVAL;
    }

  if (attr != NULL && *attr != NULL)
    {
      result = EINVAL;		/* Not supported */
      goto DONE;
    }

  rwl = (pthread_rwlock_t) calloc (1, sizeof (*rwl));

  if (rwl == NULL)
    {
      result = ENOMEM;
      goto DONE;
    }

  rwl->nSharedAccessCount = 0;
  rwl->nExclusiveAccessCount = 0;
  rwl->nCompletedSharedAccessCount = 0;

  result = pthread_mutex_init (&rwl->mtxExclusiveAccess, NULL);
  if (result != 0)
    {
      goto FAIL0;
    }

  result = pthread_mutex_init (&rwl->mtxSharedAccessCompleted, NULL);
  if (result != 0)
    {
      goto FAIL1;
    }

  result = pthread_cond_init (&rwl->cndSharedAccessCompleted, NULL);
  if (result != 0)
    {
      goto FAIL2;
    }

  rwl->nMagic = PTW32_RWLOCK_MAGIC;

  result = 0;
  goto DONE;

FAIL2:
  (void) pthread_mutex_destroy (&(rwl->mtxSharedAccessCompleted));

FAIL1:
  (void) pthread_mutex_destroy (&(rwl->mtxExclusiveAccess));

FAIL0:
  (void) free (rwl);
  rwl = NULL;

DONE:
  *rwlock = rwl;

  return result;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlock_destroy (pthread_rwlock_t * rwlock)
{
  pthread_rwlock_t rwl;
  int result = 0, result1 = 0, result2 = 0;

  if (rwlock == NULL || *rwlock == NULL)
    {
      return EINVAL;
    }

  if (*rwlock != PTHREAD_RWLOCK_INITIALIZER)
    {
      rwl = *rwlock;

      if (rwl->nMagic != PTW32_RWLOCK_MAGIC)
	{
	  return EINVAL;
	}

      if ((result = pthread_mutex_lock (&(rwl->mtxExclusiveAccess))) != 0)
	{
	  return result;
	}

      if ((result =
	   pthread_mutex_lock (&(rwl->mtxSharedAccessCompleted))) != 0)
	{
	  (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	  return result;
	}

      /*
       * Check whether any threads own/wait for the lock (wait for ex.access);
       * report "BUSY" if so.
       */
      if (rwl->nExclusiveAccessCount > 0
	  || rwl->nSharedAccessCount > rwl->nCompletedSharedAccessCount)
	{
	  result = pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted));
	  result1 = pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	  result2 = EBUSY;
	}
      else
	{
	  rwl->nMagic = 0;

	  if ((result =
	       pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted))) != 0)
	    {
	      pthread_mutex_unlock (&rwl->mtxExclusiveAccess);
	      return result;
	    }

	  if ((result =
	       pthread_mutex_unlock (&(rwl->mtxExclusiveAccess))) != 0)
	    {
	      return result;
	    }

	  *rwlock = NULL;	/* Invalidate rwlock before anything else */
	  result = pthread_cond_destroy (&(rwl->cndSharedAccessCompleted));
	  result1 = pthread_mutex_destroy (&(rwl->mtxSharedAccessCompleted));
	  result2 = pthread_mutex_destroy (&(rwl->mtxExclusiveAccess));
	  (void) free (rwl);
	}
    }
  else
    {
      /*
       * See notes in ptw32_rwlock_check_need_init() above also.
       */
      EnterCriticalSection (&ptw32_rwlock_test_init_lock);

      /*
       * Check again.
       */
      if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
	{
	  /*
	   * This is all we need to do to destroy a statically
	   * initialised rwlock that has not yet been used (initialised).
	   * If we get to here, another thread
	   * waiting to initialise this rwlock will get an EINVAL.
	   */
	  *rwlock = NULL;
	}
      else
	{
	  /*
	   * The rwlock has been initialised while we were waiting
	   * so assume it's in use.
	   */
	  result = EBUSY;
	}

      LeaveCriticalSection (&ptw32_rwlock_test_init_lock);
    }

  return ((result != 0) ? result : ((result1 != 0) ? result1 : result2));
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlockattr_init (pthread_rwlockattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Initializes a rwlock attributes object with default
      *      attributes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_rwlockattr_t
      *
      *
      * DESCRIPTION
      *      Initializes a rwlock attributes object with default
      *      attributes.
      *
      * RESULTS
      *              0               successfully initialized attr,
      *              ENOMEM          insufficient memory for attr.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  pthread_rwlockattr_t rwa;

  rwa = (pthread_rwlockattr_t) calloc (1, sizeof (*rwa));

  if (rwa == NULL)
    {
      result = ENOMEM;
    }
  else
    {
      rwa->pshared = PTHREAD_PROCESS_PRIVATE;
    }

  *attr = rwa;

  return (result);
}				/* pthread_rwlockattr_init */
// ===========================SOURCE FILE BREAK===============================
int
pthread_rwlockattr_destroy (pthread_rwlockattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Destroys a rwlock attributes object. The object can
      *      no longer be used.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_rwlockattr_t
      *
      *
      * DESCRIPTION
      *      Destroys a rwlock attributes object. The object can
      *      no longer be used.
      *
      *      NOTES:
      *              1)      Does not affect rwlockss created using 'attr'
      *
      * RESULTS
      *              0               successfully released attr,
      *              EINVAL          'attr' is invalid.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (attr == NULL || *attr == NULL)
    {
      result = EINVAL;
    }
  else
    {
      pthread_rwlockattr_t rwa = *attr;

      *attr = NULL;
      free (rwa);
    }

  return (result);
}				/* pthread_rwlockattr_destroy */
// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlockattr_getpshared (const pthread_rwlockattr_t * attr,
			       int *pshared)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Determine whether rwlocks created with 'attr' can be
      *      shared between processes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_rwlockattr_t
      *
      *      pshared
      *              will be set to one of:
      *
      *                      PTHREAD_PROCESS_SHARED
      *                              May be shared if in shared memory
      *
      *                      PTHREAD_PROCESS_PRIVATE
      *                              Cannot be shared.
      *
      *
      * DESCRIPTION
      *      Rwlocks creatd with 'attr' can be shared between
      *      processes if pthread_rwlock_t variable is allocated
      *      in memory shared by these processes.
      *      NOTES:
      *              1)      pshared rwlocks MUST be allocated in shared
      *                      memory.
      *              2)      The following macro is defined if shared rwlocks
      *                      are supported:
      *                              _POSIX_THREAD_PROCESS_SHARED
      *
      * RESULTS
      *              0               successfully retrieved attribute,
      *              EINVAL          'attr' is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result;

  if ((attr != NULL && *attr != NULL) && (pshared != NULL))
    {
      *pshared = (*attr)->pshared;
      result = 0;
    }
  else
    {
      result = EINVAL;
    }

  return (result);

}				/* pthread_rwlockattr_getpshared */


// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlockattr_setpshared (pthread_rwlockattr_t * attr, int pshared)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      Rwlocks created with 'attr' can be shared between
      *      processes if pthread_rwlock_t variable is allocated
      *      in memory shared by these processes.
      *
      * PARAMETERS
      *      attr
      *              pointer to an instance of pthread_rwlockattr_t
      *
      *      pshared
      *              must be one of:
      *
      *                      PTHREAD_PROCESS_SHARED
      *                              May be shared if in shared memory
      *
      *                      PTHREAD_PROCESS_PRIVATE
      *                              Cannot be shared.
      *
      * DESCRIPTION
      *      Rwlocks creatd with 'attr' can be shared between
      *      processes if pthread_rwlock_t variable is allocated
      *      in memory shared by these processes.
      *
      *      NOTES:
      *              1)      pshared rwlocks MUST be allocated in shared
      *                      memory.
      *
      *              2)      The following macro is defined if shared rwlocks
      *                      are supported:
      *                              _POSIX_THREAD_PROCESS_SHARED
      *
      * RESULTS
      *              0               successfully set attribute,
      *              EINVAL          'attr' or pshared is invalid,
      *              ENOSYS          PTHREAD_PROCESS_SHARED not supported,
      *
      * ------------------------------------------------------
      */
{
  int result;

  if ((attr != NULL && *attr != NULL) &&
      ((pshared == PTHREAD_PROCESS_SHARED) ||
       (pshared == PTHREAD_PROCESS_PRIVATE)))
    {
      if (pshared == PTHREAD_PROCESS_SHARED)
	{

#if !defined( _POSIX_THREAD_PROCESS_SHARED )

	  result = ENOSYS;
	  pshared = PTHREAD_PROCESS_PRIVATE;

#else

	  result = 0;

#endif /* _POSIX_THREAD_PROCESS_SHARED */

	}
      else
	{
	  result = 0;
	}

      (*attr)->pshared = pshared;
    }
  else
    {
      result = EINVAL;
    }

  return (result);

}				/* pthread_rwlockattr_setpshared */
// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlock_rdlock (pthread_rwlock_t * rwlock)
{
  int result;
  pthread_rwlock_t rwl;

  if (rwlock == NULL || *rwlock == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static rwlock. We check
   * again inside the guarded section of ptw32_rwlock_check_need_init()
   * to avoid race conditions.
   */
  if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
    {
      result = ptw32_rwlock_check_need_init (rwlock);

      if (result != 0 && result != EBUSY)
	{
	  return result;
	}
    }

  rwl = *rwlock;

  if (rwl->nMagic != PTW32_RWLOCK_MAGIC)
    {
      return EINVAL;
    }

  if ((result = pthread_mutex_lock (&(rwl->mtxExclusiveAccess))) != 0)
    {
      return result;
    }

  if (++rwl->nSharedAccessCount == INT_MAX)
    {
      if ((result =
	   pthread_mutex_lock (&(rwl->mtxSharedAccessCompleted))) != 0)
	{
	  (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	  return result;
	}

      rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
      rwl->nCompletedSharedAccessCount = 0;

      if ((result =
	   pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted))) != 0)
	{
	  (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	  return result;
	}
    }

  return (pthread_mutex_unlock (&(rwl->mtxExclusiveAccess)));
}
// ===========================SOURCE FILE BREAK===============================


int
pthread_rwlock_timedrdlock (pthread_rwlock_t * rwlock,
			    const struct timespec *abstime)
{
  int result;
  pthread_rwlock_t rwl;

  if (rwlock == NULL || *rwlock == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static rwlock. We check
   * again inside the guarded section of ptw32_rwlock_check_need_init()
   * to avoid race conditions.
   */
  if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
    {
      result = ptw32_rwlock_check_need_init (rwlock);

      if (result != 0 && result != EBUSY)
	{
	  return result;
	}
    }

  rwl = *rwlock;

  if (rwl->nMagic != PTW32_RWLOCK_MAGIC)
    {
      return EINVAL;
    }

  if ((result =
       pthread_mutex_timedlock (&(rwl->mtxExclusiveAccess), abstime)) != 0)
    {
      return result;
    }

  if (++rwl->nSharedAccessCount == INT_MAX)
    {
      if ((result =
	   pthread_mutex_timedlock (&(rwl->mtxSharedAccessCompleted),
				    abstime)) != 0)
	{
	  if (result == ETIMEDOUT)
	    {
	      ++rwl->nCompletedSharedAccessCount;
	    }
	  (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	  return result;
	}

      rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
      rwl->nCompletedSharedAccessCount = 0;

      if ((result =
	   pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted))) != 0)
	{
	  (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	  return result;
	}
    }

  return (pthread_mutex_unlock (&(rwl->mtxExclusiveAccess)));
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlock_wrlock (pthread_rwlock_t * rwlock)
{
  int result;
  pthread_rwlock_t rwl;

  if (rwlock == NULL || *rwlock == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static rwlock. We check
   * again inside the guarded section of ptw32_rwlock_check_need_init()
   * to avoid race conditions.
   */
  if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
    {
      result = ptw32_rwlock_check_need_init (rwlock);

      if (result != 0 && result != EBUSY)
	{
	  return result;
	}
    }

  rwl = *rwlock;

  if (rwl->nMagic != PTW32_RWLOCK_MAGIC)
    {
      return EINVAL;
    }

  if ((result = pthread_mutex_lock (&(rwl->mtxExclusiveAccess))) != 0)
    {
      return result;
    }

  if ((result = pthread_mutex_lock (&(rwl->mtxSharedAccessCompleted))) != 0)
    {
      (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
      return result;
    }

  if (rwl->nExclusiveAccessCount == 0)
    {
      if (rwl->nCompletedSharedAccessCount > 0)
	{
	  rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
	  rwl->nCompletedSharedAccessCount = 0;
	}

      if (rwl->nSharedAccessCount > 0)
	{
	  rwl->nCompletedSharedAccessCount = -rwl->nSharedAccessCount;

	  /*
	   * This routine may be a cancelation point
	   * according to POSIX 1003.1j section 18.1.2.
	   */
#ifdef _MSC_VER
#pragma inline_depth(0)
#endif
	  pthread_cleanup_push (ptw32_rwlock_cancelwrwait, (void *) rwl);

	  do
	    {
	      result = pthread_cond_wait (&(rwl->cndSharedAccessCompleted),
					  &(rwl->mtxSharedAccessCompleted));
	    }
	  while (result == 0 && rwl->nCompletedSharedAccessCount < 0);

	  pthread_cleanup_pop ((result != 0) ? 1 : 0);
#ifdef _MSC_VER
#pragma inline_depth()
#endif

	  if (result == 0)
	    {
	      rwl->nSharedAccessCount = 0;
	    }
	}
    }

  if (result == 0)
    {
      rwl->nExclusiveAccessCount++;
    }

  return result;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlock_timedwrlock (pthread_rwlock_t * rwlock,
			    const struct timespec *abstime)
{
  int result;
  pthread_rwlock_t rwl;

  if (rwlock == NULL || *rwlock == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static rwlock. We check
   * again inside the guarded section of ptw32_rwlock_check_need_init()
   * to avoid race conditions.
   */
  if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
    {
      result = ptw32_rwlock_check_need_init (rwlock);

      if (result != 0 && result != EBUSY)
	{
	  return result;
	}
    }

  rwl = *rwlock;

  if (rwl->nMagic != PTW32_RWLOCK_MAGIC)
    {
      return EINVAL;
    }

  if ((result =
       pthread_mutex_timedlock (&(rwl->mtxExclusiveAccess), abstime)) != 0)
    {
      return result;
    }

  if ((result =
       pthread_mutex_timedlock (&(rwl->mtxSharedAccessCompleted),
				abstime)) != 0)
    {
      (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
      return result;
    }

  if (rwl->nExclusiveAccessCount == 0)
    {
      if (rwl->nCompletedSharedAccessCount > 0)
	{
	  rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
	  rwl->nCompletedSharedAccessCount = 0;
	}

      if (rwl->nSharedAccessCount > 0)
	{
	  rwl->nCompletedSharedAccessCount = -rwl->nSharedAccessCount;

	  /*
	   * This routine may be a cancelation point
	   * according to POSIX 1003.1j section 18.1.2.
	   */
#ifdef _MSC_VER
#pragma inline_depth(0)
#endif
	  pthread_cleanup_push (ptw32_rwlock_cancelwrwait, (void *) rwl);

	  do
	    {
	      result =
		pthread_cond_timedwait (&(rwl->cndSharedAccessCompleted),
					&(rwl->mtxSharedAccessCompleted),
					abstime);
	    }
	  while (result == 0 && rwl->nCompletedSharedAccessCount < 0);

	  pthread_cleanup_pop ((result != 0) ? 1 : 0);
#ifdef _MSC_VER
#pragma inline_depth()
#endif

	  if (result == 0)
	    {
	      rwl->nSharedAccessCount = 0;
	    }
	}
    }

  if (result == 0)
    {
      rwl->nExclusiveAccessCount++;
    }

  return result;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlock_unlock (pthread_rwlock_t * rwlock)
{
  int result, result1;
  pthread_rwlock_t rwl;

  if (rwlock == NULL || *rwlock == NULL)
    {
      return (EINVAL);
    }

  if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
    {
      /*
       * Assume any race condition here is harmless.
       */
      return 0;
    }

  rwl = *rwlock;

  if (rwl->nMagic != PTW32_RWLOCK_MAGIC)
    {
      return EINVAL;
    }

  if (rwl->nExclusiveAccessCount == 0)
    {
      if ((result =
	   pthread_mutex_lock (&(rwl->mtxSharedAccessCompleted))) != 0)
	{
	  return result;
	}

      if (++rwl->nCompletedSharedAccessCount == 0)
	{
	  result = pthread_cond_signal (&(rwl->cndSharedAccessCompleted));
	}

      result1 = pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted));
    }
  else
    {
      rwl->nExclusiveAccessCount--;

      result = pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted));
      result1 = pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));

    }

  return ((result != 0) ? result : result1);
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlock_tryrdlock (pthread_rwlock_t * rwlock)
{
  int result;
  pthread_rwlock_t rwl;

  if (rwlock == NULL || *rwlock == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static rwlock. We check
   * again inside the guarded section of ptw32_rwlock_check_need_init()
   * to avoid race conditions.
   */
  if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
    {
      result = ptw32_rwlock_check_need_init (rwlock);

      if (result != 0 && result != EBUSY)
	{
	  return result;
	}
    }

  rwl = *rwlock;

  if (rwl->nMagic != PTW32_RWLOCK_MAGIC)
    {
      return EINVAL;
    }

  if ((result = pthread_mutex_trylock (&(rwl->mtxExclusiveAccess))) != 0)
    {
      return result;
    }

  if (++rwl->nSharedAccessCount == INT_MAX)
    {
      if ((result =
	   pthread_mutex_lock (&(rwl->mtxSharedAccessCompleted))) != 0)
	{
	  (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	  return result;
	}

      rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
      rwl->nCompletedSharedAccessCount = 0;

      if ((result =
	   pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted))) != 0)
	{
	  (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	  return result;
	}
    }

  return (pthread_mutex_unlock (&rwl->mtxExclusiveAccess));
}

// ===========================SOURCE FILE BREAK===============================

int
pthread_rwlock_trywrlock (pthread_rwlock_t * rwlock)
{
  int result, result1;
  pthread_rwlock_t rwl;

  if (rwlock == NULL || *rwlock == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static rwlock. We check
   * again inside the guarded section of ptw32_rwlock_check_need_init()
   * to avoid race conditions.
   */
  if (*rwlock == PTHREAD_RWLOCK_INITIALIZER)
    {
      result = ptw32_rwlock_check_need_init (rwlock);

      if (result != 0 && result != EBUSY)
	{
	  return result;
	}
    }

  rwl = *rwlock;

  if (rwl->nMagic != PTW32_RWLOCK_MAGIC)
    {
      return EINVAL;
    }

  if ((result = pthread_mutex_trylock (&(rwl->mtxExclusiveAccess))) != 0)
    {
      return result;
    }

  if ((result =
       pthread_mutex_trylock (&(rwl->mtxSharedAccessCompleted))) != 0)
    {
      result1 = pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
      return ((result1 != 0) ? result1 : result);
    }

  if (rwl->nExclusiveAccessCount == 0)
    {
      if (rwl->nCompletedSharedAccessCount > 0)
	{
	  rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
	  rwl->nCompletedSharedAccessCount = 0;
	}

      if (rwl->nSharedAccessCount > 0)
	{
	  if ((result =
	       pthread_mutex_unlock (&(rwl->mtxSharedAccessCompleted))) != 0)
	    {
	      (void) pthread_mutex_unlock (&(rwl->mtxExclusiveAccess));
	      return result;
	    }

	  if ((result =
	       pthread_mutex_unlock (&(rwl->mtxExclusiveAccess))) == 0)
	    {
	      result = EBUSY;
	    }
	}
      else
	{
	  rwl->nExclusiveAccessCount = 1;
	}
    }
  else
    {
      result = EBUSY;
    }

  return result;
}
// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "sched.c"
//#include <Win32PThreads/sched.h>

//#include "pthread_attr_setschedpolicy.c"
//#include "pthread_attr_getschedpolicy.c"
//#include "pthread_attr_setschedparam.c"
//#include "pthread_attr_getschedparam.c"
//#include "pthread_attr_setinheritsched.c"
//#include "pthread_attr_getinheritsched.c"
//#include "pthread_setschedparam.c"
//#include "pthread_getschedparam.c"
//#include "sched_get_priority_max.c"
//#include "sched_get_priority_min.c"
//#include "sched_setscheduler.c"
//#include "sched_getscheduler.c"
//#include "sched_yield.c"

int
pthread_attr_setschedpolicy (pthread_attr_t * attr, int policy)
{
  if (ptw32_is_attr (attr) != 0)
    {
      return EINVAL;
    }

  if (policy != SCHED_OTHER)
    {
      return ENOTSUP;
    }

  return 0;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_attr_getschedpolicy (pthread_attr_t * attr, int *policy)
{
  if (ptw32_is_attr (attr) != 0 || policy == NULL)
    {
      return EINVAL;
    }

  /*
   * Validate the policy arg.
   * Check that a policy constant wasn't passed rather than &policy.
   */
  if (policy <= (int *) SCHED_MAX)
    {
      return EINVAL;
    }

  *policy = SCHED_OTHER;

  return 0;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_attr_setschedparam (pthread_attr_t * attr,
			    const struct sched_param *param)
{
  int priority;

  if (ptw32_is_attr (attr) != 0 || param == NULL)
    {
      return EINVAL;
    }

  priority = param->sched_priority;

  /* Validate priority level. */
  if (priority < sched_get_priority_min (SCHED_OTHER) ||
      priority > sched_get_priority_max (SCHED_OTHER))
    {
      return EINVAL;
    }

  memcpy (&(*attr)->param, param, sizeof (*param));
  return 0;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_attr_getschedparam (const pthread_attr_t * attr,
			    struct sched_param *param)
{
  if (ptw32_is_attr (attr) != 0 || param == NULL)
    {
      return EINVAL;
    }

  memcpy (param, &(*attr)->param, sizeof (*param));
  return 0;
}

// ===========================SOURCE FILE BREAK===============================

int
pthread_attr_setinheritsched (pthread_attr_t * attr, int inheritsched)
{
  if (ptw32_is_attr (attr) != 0)
    {
      return EINVAL;
    }

  if (PTHREAD_INHERIT_SCHED != inheritsched
      && PTHREAD_EXPLICIT_SCHED != inheritsched)
    {
      return EINVAL;
    }

  (*attr)->inheritsched = inheritsched;
  return 0;
}
// ===========================SOURCE FILE BREAK===============================
int
pthread_attr_getinheritsched (pthread_attr_t * attr, int *inheritsched)
{
  if (ptw32_is_attr (attr) != 0 || inheritsched == NULL)
    {
      return EINVAL;
    }

  *inheritsched = (*attr)->inheritsched;
  return 0;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_setschedparam (pthread_t thread, int policy,
		       const struct sched_param *param)
{
  int result;

  /* Validate the thread id. */
  result = pthread_kill (thread, 0);
  if (0 != result)
    {
      return result;
    }

  /* Validate the scheduling policy. */
  if (policy < SCHED_MIN || policy > SCHED_MAX)
    {
      return EINVAL;
    }

  /* Ensure the policy is SCHED_OTHER. */
  if (policy != SCHED_OTHER)
    {
      return ENOTSUP;
    }

  return (ptw32_setthreadpriority (thread, policy, param->sched_priority));
}


int
ptw32_setthreadpriority (pthread_t thread, int policy, int priority)
{
  int prio;
  int result;
  ptw32_thread_t * tp = (ptw32_thread_t *) thread.p;

  prio = priority;

  /* Validate priority level. */
  if (prio < sched_get_priority_min (policy) ||
      prio > sched_get_priority_max (policy))
    {
      return EINVAL;
    }

#if (THREAD_PRIORITY_LOWEST > THREAD_PRIORITY_NORMAL)
/* WinCE */
#else
/* Everything else */

  if (THREAD_PRIORITY_IDLE < prio && THREAD_PRIORITY_LOWEST > prio)
    {
      prio = THREAD_PRIORITY_LOWEST;
    }
  else if (THREAD_PRIORITY_TIME_CRITICAL > prio
	   && THREAD_PRIORITY_HIGHEST < prio)
    {
      prio = THREAD_PRIORITY_HIGHEST;
    }

#endif

  result = pthread_mutex_lock (&tp->threadLock);

  if (0 == result)
    {
      /* If this fails, the current priority is unchanged. */
      if (0 == SetThreadPriority (tp->threadH, prio))
	{
	  result = EINVAL;
	}
      else
	{
	  /*
	   * Must record the thread's sched_priority as given,
	   * not as finally adjusted.
	   */
	  tp->sched_priority = priority;
	}

      (void) pthread_mutex_unlock (&tp->threadLock);
    }

  return result;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_getschedparam (pthread_t thread, int *policy,
		       struct sched_param *param)
{
  int result;

  /* Validate the thread id. */
  result = pthread_kill (thread, 0);
  if (0 != result)
    {
      return result;
    }

  /*
   * Validate the policy and param args.
   * Check that a policy constant wasn't passed rather than &policy.
   */
  if (policy <= (int *) SCHED_MAX || param == NULL)
    {
      return EINVAL;
    }

  /* Fill out the policy. */
  *policy = SCHED_OTHER;

  /*
   * This function must return the priority value set by
   * the most recent pthread_setschedparam() or pthread_create()
   * for the target thread. It must not return the actual thread
   * priority as altered by any system priority adjustments etc.
   */
  param->sched_priority = ((ptw32_thread_t *)thread.p)->sched_priority;

  return 0;
}
// ===========================SOURCE FILE BREAK===============================

int
sched_get_priority_max (int policy)
{
  if (policy < SCHED_MIN || policy > SCHED_MAX)
    {
      errno = EINVAL;
      return -1;
    }

#if (THREAD_PRIORITY_LOWEST > THREAD_PRIORITY_NORMAL)
  /* WinCE? */
  return PTW32_MAX (THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL);
#else
  /* This is independent of scheduling policy in Win32. */
  return PTW32_MAX (THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL);
#endif
}
// ===========================SOURCE FILE BREAK===============================

int
sched_get_priority_min (int policy)
{
  if (policy < SCHED_MIN || policy > SCHED_MAX)
    {
      errno = EINVAL;
      return -1;
    }

#if (THREAD_PRIORITY_LOWEST > THREAD_PRIORITY_NORMAL)
  /* WinCE? */
  return PTW32_MIN (THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL);
#else
  /* This is independent of scheduling policy in Win32. */
  return PTW32_MIN (THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL);
#endif
}
// ===========================SOURCE FILE BREAK===============================

int
sched_setscheduler (pid_t pid, int policy)
{
  /*
   * Win32 only has one policy which we call SCHED_OTHER.
   * However, we try to provide other valid side-effects
   * such as EPERM and ESRCH errors. Choosing to check
   * for a valid policy last allows us to get the most value out
   * of this function.
   */
  if (0 != pid)
    {
      int selfPid = (int) GetCurrentProcessId ();

      if (pid != selfPid)
	{
	  HANDLE h =
	    OpenProcess (PROCESS_SET_INFORMATION, PTW32_FALSE, (DWORD) pid);

	  if (NULL == h)
	    {
	      errno =
		(GetLastError () ==
		 (0xFF & ERROR_ACCESS_DENIED)) ? EPERM : ESRCH;
	      return -1;
	    }
	}
    }

  if (SCHED_OTHER != policy)
    {
      errno = ENOSYS;
      return -1;
    }

  /*
   * Don't set anything because there is nothing to set.
   * Just return the current (the only possible) value.
   */
  return SCHED_OTHER;
}
// ===========================SOURCE FILE BREAK===============================

int
sched_getscheduler (pid_t pid)
{
  /*
   * Win32 only has one policy which we call SCHED_OTHER.
   * However, we try to provide other valid side-effects
   * such as EPERM and ESRCH errors.
   */
  if (0 != pid)
    {
      int selfPid = (int) GetCurrentProcessId ();

      if (pid != selfPid)
	{
	  HANDLE h =
	    OpenProcess (PROCESS_QUERY_INFORMATION, PTW32_FALSE, (DWORD) pid);

	  if (NULL == h)
	    {
	      errno =
		(GetLastError () ==
		 (0xFF & ERROR_ACCESS_DENIED)) ? EPERM : ESRCH;
	      return -1;
	    }
	}
    }

  return SCHED_OTHER;
}

// ===========================SOURCE FILE BREAK===============================


int
sched_yield (void)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function indicates that the calling thread is
      *      willing to give up some time slices to other threads.
      *
      * PARAMETERS
      *      N/A
      *
      *
      * DESCRIPTION
      *      This function indicates that the calling thread is
      *      willing to give up some time slices to other threads.
      *      NOTE: Since this is part of POSIX 1003.1b
      *                (realtime extensions), it is defined as returning
      *                -1 if an error occurs and sets errno to the actual
      *                error.
      *
      * RESULTS
      *              0               successfully created semaphore,
      *              ENOSYS          sched_yield not supported,
      *
      * ------------------------------------------------------
      */
{
  Sleep (0);

  return 0;
}
// ===========================SOURCE FILE BREAK===============================
//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
//#include "semaphore.c"
//
//#include "semaphore.h"
//#include "sem_init.c"
//#include "sem_destroy.c"
//#include "sem_trywait.c"
//#include "sem_wait.c"
//#include "sem_timedwait.c"
//#include "sem_post.c"
//#include "sem_post_multiple.c"
//#include "sem_getvalue.c"
//#include "sem_open.c"
//#include "sem_close.c"
//#include "sem_unlink.c"


#ifndef NEED_FTIME
#  include <sys/timeb.h>
#endif

#include <limits.h>
#if !defined( SEMAPHORE_H )
#define SEMAPHORE_H

#undef PTW32_LEVEL

#if defined(_POSIX_SOURCE)
#define PTW32_LEVEL 0
/* Early POSIX */
#endif

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199309
#undef PTW32_LEVEL
#define PTW32_LEVEL 1
/* Include 1b, 1c and 1d */
#endif

#if defined(INCLUDE_NP)
#undef PTW32_LEVEL
#define PTW32_LEVEL 2
/* Include Non-Portable extensions */
#endif

#define PTW32_LEVEL_MAX 3

#if !defined(PTW32_LEVEL)
#define PTW32_LEVEL PTW32_LEVEL_MAX
/* Include everything */
#endif

#if __GNUC__ && ! defined (__declspec)
# error Please upgrade your GNU compiler to one that supports __declspec.
#endif

/*
 * When building the DLL code, you should define PTW32_BUILD so that
 * the variables/functions are exported correctly. When using the DLL,
 * do NOT define PTW32_BUILD, and then the variables/functions will
 * be imported correctly.
 */
//#ifndef PTW32_STATIC_LIB
//#  ifdef PTW32_BUILD
//#    define PTW32_DLLPORT __declspec (dllexport)
//#  else
//#    define PTW32_DLLPORT __declspec (dllimport)
//#  endif
//#else
//#  define PTW32_DLLPORT
//#endif
#ifndef PTW32_DLLPORT
	#define PTW32_DLLPORT
#endif

/*
 * This is a duplicate of what is in the autoconf config.h,
 * which is only used when building the pthread-win32 libraries.
 */

#ifndef PTW32_CONFIG_H
#  if defined(WINCE)
#    define NEED_ERRNO
#    define NEED_SEM
#  endif
#  if defined(_UWIN) || defined(__MINGW32__)
#    define HAVE_MODE_T
#  endif
#endif

/*
 *
 */

#if PTW32_LEVEL >= PTW32_LEVEL_MAX
#ifdef NEED_ERRNO
#include "need_errno.h"
#else
#include <errno.h>
#endif
#endif /* PTW32_LEVEL >= PTW32_LEVEL_MAX */

#define _POSIX_SEMAPHORES

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#ifndef HAVE_MODE_T
typedef unsigned int mode_t;
#endif


typedef struct sem_t_ * sem_t;

PTW32_DLLPORT int __cdecl sem_init (sem_t * sem,
			    int pshared,
			    unsigned int value);

PTW32_DLLPORT int __cdecl sem_destroy (sem_t * sem);

PTW32_DLLPORT int __cdecl sem_trywait (sem_t * sem);

PTW32_DLLPORT int __cdecl sem_wait (sem_t * sem);

PTW32_DLLPORT int __cdecl sem_timedwait (sem_t * sem,
				 const struct timespec * abstime);

PTW32_DLLPORT int __cdecl sem_post (sem_t * sem);

PTW32_DLLPORT int __cdecl sem_post_multiple (sem_t * sem,
				     int count);

PTW32_DLLPORT int __cdecl sem_open (const char * name,
			    int oflag,
			    mode_t mode,
			    unsigned int value);

PTW32_DLLPORT int __cdecl sem_close (sem_t * sem);

PTW32_DLLPORT int __cdecl sem_unlink (const char * name);

PTW32_DLLPORT int __cdecl sem_getvalue (sem_t * sem,
				int * sval);

#ifdef __cplusplus
}				/* End of extern "C" */
#endif				/* __cplusplus */

#undef PTW32_LEVEL
#undef PTW32_LEVEL_MAX

#endif				/* !SEMAPHORE_H */

// ===========================SOURCE FILE BREAK===============================

int
sem_init (sem_t * sem, int pshared, unsigned int value)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function initializes a semaphore. The
      *      initial value of the semaphore is 'value'
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      *      pshared
      *              if zero, this semaphore may only be shared between
      *              threads in the same process.
      *              if nonzero, the semaphore can be shared between
      *              processes
      *
      *      value
      *              initial value of the semaphore counter
      *
      * DESCRIPTION
      *      This function initializes a semaphore. The
      *      initial value of the semaphore is set to 'value'.
      *
      * RESULTS
      *              0               successfully created semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore, or
      *                              'value' >= SEM_VALUE_MAX
      *              ENOMEM          out of memory,
      *              ENOSPC          a required resource has been exhausted,
      *              ENOSYS          semaphores are not supported,
      *              EPERM           the process lacks appropriate privilege
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  sem_t s = NULL;

  if (pshared != 0)
    {
      /*
       * Creating a semaphore that can be shared between
       * processes
       */
      result = EPERM;
    }
  else if (value > (unsigned int)SEM_VALUE_MAX)
    {
      result = EINVAL;
    }
  else
    {
      s = (sem_t) calloc (1, sizeof (*s));

      if (NULL == s)
	{
	  result = ENOMEM;
	}
      else
	{

	  s->value = value;
	  if (pthread_mutex_init(&s->lock, NULL) == 0)
	    {

#ifdef NEED_SEM

	  s->sem = CreateEvent (NULL,
				PTW32_FALSE,	/* auto (not manual) reset */
				PTW32_FALSE,	/* initial state is unset */
				NULL);

	  if (0 == s->sem)
	    {
	      free (s);
	      (void) pthread_mutex_destroy(&s->lock);
	      result = ENOSPC;
	    }
	  else
	    {
	      s->leftToUnblock = 0;
	    }

#else /* NEED_SEM */

	      if ((s->sem = CreateSemaphore (NULL,	/* Always NULL */
					     (long) 0,	/* Force threads to wait */
					     (long) SEM_VALUE_MAX,	/* Maximum value */
					     NULL)) == 0)	/* Name */
		{
		  (void) pthread_mutex_destroy(&s->lock);
		  result = ENOSPC;
		}

#endif /* NEED_SEM */

	    }
	  else
	    {
	      result = ENOSPC;
	    }

	  if (result != 0)
	    {
	      free(s);
	    }
	}
    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  *sem = s;

  return 0;

}				/* sem_init */

// ===========================SOURCE FILE BREAK===============================

int
sem_destroy (sem_t * sem)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function destroys an unnamed semaphore.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      * DESCRIPTION
      *      This function destroys an unnamed semaphore.
      *
      * RESULTS
      *              0               successfully destroyed semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *              EBUSY           threads (or processes) are currently
      *                                      blocked on 'sem'
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  sem_t s = NULL;

  if (sem == NULL || *sem == NULL)
    {
      result = EINVAL;
    }
  else
    {
      s = *sem;

      if ((result = pthread_mutex_lock (&s->lock)) == 0)
        {
          if (s->value < 0)
            {
              (void) pthread_mutex_unlock (&s->lock);
              result = EBUSY;
            }
          else
            {
              /* There are no threads currently blocked on this semaphore. */

              if (!CloseHandle (s->sem))
	        {
                  (void) pthread_mutex_unlock (&s->lock);
	          result = EINVAL;
	        }
	      else
	        {
                  /*
                   * Invalidate the semaphore handle when we have the lock.
                   * Other sema operations should test this after acquiring the lock
                   * to check that the sema is still valid, i.e. before performing any
                   * operations. This may only be necessary before the sema op routine
                   * returns so that the routine can return EINVAL - e.g. if setting
                   * s->value to SEM_VALUE_MAX below does force a fall-through.
                   */
                  *sem = NULL;

                  /* Prevent anyone else actually waiting on or posting this sema.
                   */
                  s->value = SEM_VALUE_MAX;

                  (void) pthread_mutex_unlock (&s->lock);

                  do
                    {
                      /* Give other threads a chance to run and exit any sema op
                       * routines. Due to the SEM_VALUE_MAX value, if sem_post or
                       * sem_wait were blocked by us they should fall through.
                       */
                      Sleep(0);
                    }
                  while (pthread_mutex_destroy (&s->lock) == EBUSY);
                }
            }
        }
    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  free (s);

  return 0;

}				/* sem_destroy */

// ===========================SOURCE FILE BREAK===============================
int
sem_trywait (sem_t * sem)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function tries to wait on a semaphore.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      * DESCRIPTION
      *      This function tries to wait on a semaphore. If the
      *      semaphore value is greater than zero, it decreases
      *      its value by one. If the semaphore value is zero, then
      *      this function returns immediately with the error EAGAIN
      *
      * RESULTS
      *              0               successfully decreased semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EAGAIN          the semaphore was already locked,
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOTSUP         sem_trywait is not supported,
      *              EINTR           the function was interrupted by a signal,
      *              EDEADLK         a deadlock condition was detected.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  sem_t s = *sem;

  if (s == NULL)
    {
      result = EINVAL;
    }
  else if ((result = pthread_mutex_lock (&s->lock)) == 0)
    {
      /* See sem_destroy.c
       */
     if (*sem == NULL)
        {
          (void) pthread_mutex_unlock (&s->lock);
          errno = EINVAL;
          return -1;
        }

      if (s->value > 0)
	{
	  s->value--;
	}
      else
	{
	  result = EAGAIN;
	}

      (void) pthread_mutex_unlock (&s->lock);
    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  return 0;

}				/* sem_trywait */
// ===========================SOURCE FILE BREAK===============================

static void PTW32_CDECL
ptw32_sem_wait_cleanup(void * sem)
{
  sem_t s = (sem_t) sem;

  if (pthread_mutex_lock (&s->lock) == 0)
    {
      /*
       * If sema is destroyed do nothing, otherwise:-
       * If the sema is posted between us being cancelled and us locking
       * the sema again above then we need to consume that post but cancel
       * anyway. If we don't get the semaphore we indicate that we're no
       * longer waiting.
       */
      if (*((sem_t *)sem) != NULL && !(WaitForSingleObject(s->sem, 0) == WAIT_OBJECT_0))
	{
	  ++s->value;
#ifdef NEED_SEM
	  if (s->value > 0)
	    {
	      s->leftToUnblock = 0;
	    }
#else
	  /*
	   * Don't release the W32 sema, it doesn't need adjustment
	   * because it doesn't record the number of waiters.
	   */
#endif /* NEED_SEM */
	}
      (void) pthread_mutex_unlock (&s->lock);
    }
}

int
sem_wait (sem_t * sem)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function  waits on a semaphore.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      * DESCRIPTION
      *      This function waits on a semaphore. If the
      *      semaphore value is greater than zero, it decreases
      *      its value by one. If the semaphore value is zero, then
      *      the calling thread (or process) is blocked until it can
      *      successfully decrease the value or until interrupted by
      *      a signal.
      *
      * RESULTS
      *              0               successfully decreased semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *              EINTR           the function was interrupted by a signal,
      *              EDEADLK         a deadlock condition was detected.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  sem_t s = *sem;

  pthread_testcancel();

  if (s == NULL)
    {
      result = EINVAL;
    }
  else
    {
      if ((result = pthread_mutex_lock (&s->lock)) == 0)
	{
	  int v;

	  /* See sem_destroy.c
	   */
	  if (*sem == NULL)
	    {
	      (void) pthread_mutex_unlock (&s->lock);
	      errno = EINVAL;
	      return -1;
	    }

          v = --s->value;
	  (void) pthread_mutex_unlock (&s->lock);

	  if (v < 0)
	    {
#ifdef _MSC_VER
#pragma inline_depth(0)
#endif
	      /* Must wait */
	      pthread_cleanup_push(ptw32_sem_wait_cleanup, (void *) s);
	      result = pthreadCancelableWait (s->sem);
	      /* Cleanup if we're canceled or on any other error */
	      pthread_cleanup_pop(result);
#ifdef _MSC_VER
#pragma inline_depth()
#endif
	    }
#ifdef NEED_SEM

	  if (!result && pthread_mutex_lock (&s->lock) == 0)
	    {
	      if (*sem == NULL)
	        {
	          (void) pthread_mutex_unlock (&s->lock);
	          errno = EINVAL;
	          return -1;
	        }

	      if (s->leftToUnblock > 0)
		{
		  --s->leftToUnblock;
		  SetEvent(s->sem);
		}
	      (void) pthread_mutex_unlock (&s->lock);
	    }

#endif /* NEED_SEM */

	}

    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  return 0;

}				/* sem_wait */

// ===========================SOURCE FILE BREAK===============================


typedef struct {
  sem_t sem;
  int * resultPtr;
} sem_timedwait_cleanup_args_t;


static void PTW32_CDECL
ptw32_sem_timedwait_cleanup (void * args)
{
  sem_timedwait_cleanup_args_t * a = (sem_timedwait_cleanup_args_t *)args;
  sem_t s = a->sem;

  if (pthread_mutex_lock (&s->lock) == 0)
    {
      /*
       * We either timed out or were cancelled.
       * If someone has posted between then and now we try to take the semaphore.
       * Otherwise the semaphore count may be wrong after we
       * return. In the case of a cancellation, it is as if we
       * were cancelled just before we return (after taking the semaphore)
       * which is ok.
       */
      if (WaitForSingleObject(s->sem, 0) == WAIT_OBJECT_0)
	{
	  /* We got the semaphore on the second attempt */
	  *(a->resultPtr) = 0;
	}
      else
	{
	  /* Indicate we're no longer waiting */
	  s->value++;
#ifdef NEED_SEM
	  if (s->value > 0)
	    {
	      s->leftToUnblock = 0;
	    }
#else
          /*
           * Don't release the W32 sema, it doesn't need adjustment
           * because it doesn't record the number of waiters.
           */
#endif
	}
      (void) pthread_mutex_unlock (&s->lock);
    }
}


int
sem_timedwait (sem_t * sem, const struct timespec *abstime)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function waits on a semaphore possibly until
      *      'abstime' time.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      *      abstime
      *              pointer to an instance of struct timespec
      *
      * DESCRIPTION
      *      This function waits on a semaphore. If the
      *      semaphore value is greater than zero, it decreases
      *      its value by one. If the semaphore value is zero, then
      *      the calling thread (or process) is blocked until it can
      *      successfully decrease the value or until interrupted by
      *      a signal.
      *
      *      If 'abstime' is a NULL pointer then this function will
      *      block until it can successfully decrease the value or
      *      until interrupted by a signal.
      *
      * RESULTS
      *              0               successfully decreased semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *              EINTR           the function was interrupted by a signal,
      *              EDEADLK         a deadlock condition was detected.
      *              ETIMEDOUT       abstime elapsed before success.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  sem_t s = *sem;

  pthread_testcancel();

  if (sem == NULL)
    {
      result = EINVAL;
    }
  else
    {
      DWORD milliseconds;

      if (abstime == NULL)
	{
	  milliseconds = INFINITE;
	}
      else
	{
	  /* 
	   * Calculate timeout as milliseconds from current system time. 
	   */
	  milliseconds = ptw32_relmillisecs (abstime);
	}

      if ((result = pthread_mutex_lock (&s->lock)) == 0)
	{
	  int v;

	  /* See sem_destroy.c
	   */
	  if (*sem == NULL)
	    {
	      (void) pthread_mutex_unlock (&s->lock);
	      errno = EINVAL;
	      return -1;
	    }

	  v = --s->value;
	  (void) pthread_mutex_unlock (&s->lock);

	  if (v < 0)
	    {
#ifdef NEED_SEM
	      int timedout;
#endif
	      sem_timedwait_cleanup_args_t cleanup_args;

	      cleanup_args.sem = s;
	      cleanup_args.resultPtr = &result;

#ifdef _MSC_VER
#pragma inline_depth(0)
#endif
	      /* Must wait */
              pthread_cleanup_push(ptw32_sem_timedwait_cleanup, (void *) &cleanup_args);
#ifdef NEED_SEM
	      timedout =
#endif
	      result = pthreadCancelableTimedWait (s->sem, milliseconds);
	      pthread_cleanup_pop(result);
#ifdef _MSC_VER
#pragma inline_depth()
#endif

#ifdef NEED_SEM

	      if (!timedout && pthread_mutex_lock (&s->lock) == 0)
	        {
        	  if (*sem == NULL)
        	    {
        	      (void) pthread_mutex_unlock (&s->lock);
        	      errno = EINVAL;
        	      return -1;
        	    }

	          if (s->leftToUnblock > 0)
	            {
		      --s->leftToUnblock;
		      SetEvent(s->sem);
		    }
	          (void) pthread_mutex_unlock (&s->lock);
	        }

#endif /* NEED_SEM */

	    }
	}

    }

  if (result != 0)
    {

      errno = result;
      return -1;

    }

  return 0;

}				/* sem_timedwait */

// ===========================SOURCE FILE BREAK===============================
int
sem_post (sem_t * sem)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function posts a wakeup to a semaphore.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      * DESCRIPTION
      *      This function posts a wakeup to a semaphore. If there
      *      are waiting threads (or processes), one is awakened;
      *      otherwise, the semaphore value is incremented by one.
      *
      * RESULTS
      *              0               successfully posted semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *              ERANGE          semaphore count is too big
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  sem_t s = *sem;

  if (s == NULL)
    {
      result = EINVAL;
    }
  else if ((result = pthread_mutex_lock (&s->lock)) == 0)
    {
      /* See sem_destroy.c
       */
      if (*sem == NULL)
        {
          (void) pthread_mutex_unlock (&s->lock);
          result = EINVAL;
          return -1;
        }

      if (s->value < SEM_VALUE_MAX)
	{
#ifdef NEED_SEM
	  if (++s->value <= 0
	      && !SetEvent(s->sem))
	    {
	      s->value--;
	      result = EINVAL;
	    }
#else
	  if (++s->value <= 0
	      && !ReleaseSemaphore (s->sem, 1, NULL))
	    {
	      s->value--;
	      result = EINVAL;
	    }
#endif /* NEED_SEM */
	}
      else
	{
	  result = ERANGE;
	}

      (void) pthread_mutex_unlock (&s->lock);
    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  return 0;

}				/* sem_post */
// ===========================SOURCE FILE BREAK===============================

int
sem_post_multiple (sem_t * sem, int count)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function posts multiple wakeups to a semaphore.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      *      count
      *              counter, must be greater than zero.
      *
      * DESCRIPTION
      *      This function posts multiple wakeups to a semaphore. If there
      *      are waiting threads (or processes), n <= count are awakened;
      *      the semaphore value is incremented by count - n.
      *
      * RESULTS
      *              0               successfully posted semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore
      *                              or count is less than or equal to zero.
      *              ERANGE          semaphore count is too big
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  long waiters;
  sem_t s = *sem;

  if (s == NULL || count <= 0)
    {
      result = EINVAL;
    }
  else if ((result = pthread_mutex_lock (&s->lock)) == 0)
    {
      /* See sem_destroy.c
       */
      if (*sem == NULL)
        {
          (void) pthread_mutex_unlock (&s->lock);
          result = EINVAL;
          return -1;
        }

      if (s->value <= (SEM_VALUE_MAX - count))
	{
	  waiters = -s->value;
	  s->value += count;
	  if (waiters > 0)
	    {
#ifdef NEED_SEM
	      if (SetEvent(s->sem))
		{
		  waiters--;
		  s->leftToUnblock += count - 1;
		  if (s->leftToUnblock > waiters)
		    {
		      s->leftToUnblock = waiters;
		    }
		}
#else
	      if (ReleaseSemaphore (s->sem,  (waiters<=count)?waiters:count, 0))
		{
		  /* No action */
		}
#endif
	      else
		{
		  s->value -= count;
		  result = EINVAL;
		}
	    }
	}
      else
	{
	  result = ERANGE;
	}
      (void) pthread_mutex_unlock (&s->lock);
    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  return 0;

}				/* sem_post_multiple */
// ===========================SOURCE FILE BREAK===============================


int
sem_getvalue (sem_t * sem, int *sval)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function stores the current count value of the
      *      semaphore.
      * RESULTS
      *
      * Return value
      *
      *       0                  sval has been set.
      *      -1                  failed, error in errno
      *
      *  in global errno
      *
      *      EINVAL              'sem' is not a valid semaphore,
      *      ENOSYS              this function is not supported,
      *
      *
      * PARAMETERS
      *
      *      sem                 pointer to an instance of sem_t
      *
      *      sval                pointer to int.
      *
      * DESCRIPTION
      *      This function stores the current count value of the semaphore
      *      pointed to by sem in the int pointed to by sval.
      */
{
  if (sem == NULL || *sem == NULL || sval == NULL)
    {
      errno = EINVAL;
      return -1;
    }
  else
    {
      long value;
      register sem_t s = *sem;
      int result = 0;

      if ((result = pthread_mutex_lock(&s->lock)) == 0)
        {
	  /* See sem_destroy.c
	   */
	  if (*sem == NULL)
	    {
	      (void) pthread_mutex_unlock (&s->lock);
	      errno = EINVAL;
	      return -1;
	    }

          value = s->value;
          (void) pthread_mutex_unlock(&s->lock);
          *sval = value;
        }

      return result;
    }

}				/* sem_getvalue */
// ===========================SOURCE FILE BREAK===============================

/* ignore warning "unreferenced formal parameter" */
#ifdef _MSC_VER
#pragma warning( disable : 4100 )
#endif

int
sem_open (const char *name, int oflag, mode_t mode, unsigned int value)
{
  errno = ENOSYS;
  return -1;
}				/* sem_open */
// ===========================SOURCE FILE BREAK===============================

/* ignore warning "unreferenced formal parameter" */
#ifdef _MSC_VER
#pragma warning( disable : 4100 )
#endif

int
sem_close (sem_t * sem)
{
  errno = ENOSYS;
  return -1;
}				/* sem_close */
// ===========================SOURCE FILE BREAK===============================

/* ignore warning "unreferenced formal parameter" */
#ifdef _MSC_VER
#pragma warning( disable : 4100 )
#endif

int
sem_unlink (const char *name)
{
  errno = ENOSYS;
  return -1;
}				/* sem_unlink */
// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "signal.c"

#if HAVE_SIGSET_T

static void
ptw32_signal_thread ()
{
}

static void
ptw32_signal_callhandler ()
{
}

int
pthread_sigmask (int how, sigset_t const *set, sigset_t * oset)
{
  pthread_t thread = pthread_self ();

  if (thread.p == NULL)
    {
      return ENOENT;
    }

  /* Validate the `how' argument. */
  if (set != NULL)
    {
      switch (how)
	{
	case SIG_BLOCK:
	  break;
	case SIG_UNBLOCK:
	  break;
	case SIG_SETMASK:
	  break;
	default:
	  /* Invalid `how' argument. */
	  return EINVAL;
	}
    }

  /* Copy the old mask before modifying it. */
  if (oset != NULL)
    {
      memcpy (oset, &(thread.p->sigmask), sizeof (sigset_t));
    }

  if (set != NULL)
    {
      unsigned int i;

      /* FIXME: this code assumes that sigmask is an even multiple of
         the size of a long integer. */

      unsigned long *src = (unsigned long const *) set;
      unsigned long *dest = (unsigned long *) &(thread.p->sigmask);

      switch (how)
	{
	case SIG_BLOCK:
	  for (i = 0; i < (sizeof (sigset_t) / sizeof (unsigned long)); i++)
	    {
	      /* OR the bit field longword-wise. */
	      *dest++ |= *src++;
	    }
	  break;
	case SIG_UNBLOCK:
	  for (i = 0; i < (sizeof (sigset_t) / sizeof (unsigned long)); i++)
	    {
	      /* XOR the bitfield longword-wise. */
	      *dest++ ^= *src++;
	    }
	case SIG_SETMASK:
	  /* Replace the whole sigmask. */
	  memcpy (&(thread.p->sigmask), set, sizeof (sigset_t));
	  break;
	}
    }

  return 0;
}

int
sigwait (const sigset_t * set, int *sig)
{
  /* This routine is a cancellation point */
  pthread_test_cancel();
}

int
sigaction (int signum, const struct sigaction *act, struct sigaction *oldact)
{
}

#endif /* HAVE_SIGSET_T */
//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "spin.c"
//
//#include "ptw32_spinlock_check_need_init.c"
//#include "pthread_spin_init.c"
//#include "pthread_spin_destroy.c"
//#include "pthread_spin_lock.c"
//#include "pthread_spin_unlock.c"
//#include "pthread_spin_trylock.c"


INLINE int
ptw32_spinlock_check_need_init (pthread_spinlock_t * lock)
{
  int result = 0;

  /*
   * The following guarded test is specifically for statically
   * initialised spinlocks (via PTHREAD_SPINLOCK_INITIALIZER).
   *
   * Note that by not providing this synchronisation we risk
   * introducing race conditions into applications which are
   * correctly written.
   */
  EnterCriticalSection (&ptw32_spinlock_test_init_lock);

  /*
   * We got here possibly under race
   * conditions. Check again inside the critical section
   * and only initialise if the spinlock is valid (not been destroyed).
   * If a static spinlock has been destroyed, the application can
   * re-initialise it only by calling pthread_spin_init()
   * explicitly.
   */
  if (*lock == PTHREAD_SPINLOCK_INITIALIZER)
    {
      result = pthread_spin_init (lock, PTHREAD_PROCESS_PRIVATE);
    }
  else if (*lock == NULL)
    {
      /*
       * The spinlock has been destroyed while we were waiting to
       * initialise it, so the operation that caused the
       * auto-initialisation should fail.
       */
      result = EINVAL;
    }

  LeaveCriticalSection (&ptw32_spinlock_test_init_lock);

  return (result);
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_spin_init (pthread_spinlock_t * lock, int pshared)
{
  pthread_spinlock_t s;
  int cpus = 0;
  int result = 0;

  if (lock == NULL)
    {
      return EINVAL;
    }

  if (0 != ptw32_getprocessors (&cpus))
    {
      cpus = 1;
    }

  if (cpus > 1)
    {
      if (pshared == PTHREAD_PROCESS_SHARED)
	{
	  /*
	   * Creating spinlock that can be shared between
	   * processes.
	   */
#if _POSIX_THREAD_PROCESS_SHARED >= 0

	  /*
	   * Not implemented yet.
	   */

#error ERROR [__FILE__, line __LINE__]: Process shared spin locks are not supported yet.

#else

	  return ENOSYS;

#endif /* _POSIX_THREAD_PROCESS_SHARED */

	}
    }

  s = (pthread_spinlock_t) calloc (1, sizeof (*s));

  if (s == NULL)
    {
      return ENOMEM;
    }

  if (cpus > 1)
    {
      s->u.cpus = cpus;
      s->interlock = PTW32_SPIN_UNLOCKED;
    }
  else
    {
      pthread_mutexattr_t ma;
      result = pthread_mutexattr_init (&ma);

      if (0 == result)
	{
	  ma->pshared = pshared;
	  result = pthread_mutex_init (&(s->u.mutex), &ma);
	  if (0 == result)
	    {
	      s->interlock = PTW32_SPIN_USE_MUTEX;
	    }
	}
      (void) pthread_mutexattr_destroy (&ma);
    }

  if (0 == result)
    {
      *lock = s;
    }
  else
    {
      (void) free (s);
      *lock = NULL;
    }

  return (result);
}

// ===========================SOURCE FILE BREAK===============================

int
pthread_spin_destroy (pthread_spinlock_t * lock)
{
  register pthread_spinlock_t s;
  int result = 0;

  if (lock == NULL || *lock == NULL)
    {
      return EINVAL;
    }

  if ((s = *lock) != PTHREAD_SPINLOCK_INITIALIZER)
    {
      if (s->interlock == PTW32_SPIN_USE_MUTEX)
	{
	  result = pthread_mutex_destroy (&(s->u.mutex));
	}
      else if ((PTW32_INTERLOCKED_LONG) PTW32_SPIN_UNLOCKED !=
	       PTW32_INTERLOCKED_COMPARE_EXCHANGE ((PTW32_INTERLOCKED_LPLONG)
						   & (s->interlock),
						   (PTW32_INTERLOCKED_LONG)
						   PTW32_OBJECT_INVALID,
						   (PTW32_INTERLOCKED_LONG)
						   PTW32_SPIN_UNLOCKED))
	{
	  result = EINVAL;
	}

      if (0 == result)
	{
	  /*
	   * We are relying on the application to ensure that all other threads
	   * have finished with the spinlock before destroying it.
	   */
	  *lock = NULL;
	  (void) free (s);
	}
    }
  else
    {
      /*
       * See notes in ptw32_spinlock_check_need_init() above also.
       */
      EnterCriticalSection (&ptw32_spinlock_test_init_lock);

      /*
       * Check again.
       */
      if (*lock == PTHREAD_SPINLOCK_INITIALIZER)
	{
	  /*
	   * This is all we need to do to destroy a statically
	   * initialised spinlock that has not yet been used (initialised).
	   * If we get to here, another thread
	   * waiting to initialise this mutex will get an EINVAL.
	   */
	  *lock = NULL;
	}
      else
	{
	  /*
	   * The spinlock has been initialised while we were waiting
	   * so assume it's in use.
	   */
	  result = EBUSY;
	}

      LeaveCriticalSection (&ptw32_spinlock_test_init_lock);
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_spin_lock (pthread_spinlock_t * lock)
{
  register pthread_spinlock_t s;

  if (NULL == lock || NULL == *lock)
    {
      return (EINVAL);
    }

  if (*lock == PTHREAD_SPINLOCK_INITIALIZER)
    {
      int result;

      if ((result = ptw32_spinlock_check_need_init (lock)) != 0)
	{
	  return (result);
	}
    }

  s = *lock;

  while ((PTW32_INTERLOCKED_LONG) PTW32_SPIN_LOCKED ==
	 PTW32_INTERLOCKED_COMPARE_EXCHANGE ((PTW32_INTERLOCKED_LPLONG) &
					     (s->interlock),
					     (PTW32_INTERLOCKED_LONG)
					     PTW32_SPIN_LOCKED,
					     (PTW32_INTERLOCKED_LONG)
					     PTW32_SPIN_UNLOCKED))
    {
    }

  if (s->interlock == PTW32_SPIN_LOCKED)
    {
      return 0;
    }
  else if (s->interlock == PTW32_SPIN_USE_MUTEX)
    {
      return pthread_mutex_lock (&(s->u.mutex));
    }

  return EINVAL;
}

// ===========================SOURCE FILE BREAK===============================

int
pthread_spin_unlock (pthread_spinlock_t * lock)
{
  register pthread_spinlock_t s;

  if (NULL == lock || NULL == *lock)
    {
      return (EINVAL);
    }

  s = *lock;

  if (s == PTHREAD_SPINLOCK_INITIALIZER)
    {
      return EPERM;
    }

  switch ((long)
	  PTW32_INTERLOCKED_COMPARE_EXCHANGE ((PTW32_INTERLOCKED_LPLONG) &
					      (s->interlock),
					      (PTW32_INTERLOCKED_LONG)
					      PTW32_SPIN_UNLOCKED,
					      (PTW32_INTERLOCKED_LONG)
					      PTW32_SPIN_LOCKED))
    {
    case PTW32_SPIN_LOCKED:
      return 0;
    case PTW32_SPIN_UNLOCKED:
      return EPERM;
    case PTW32_SPIN_USE_MUTEX:
      return pthread_mutex_unlock (&(s->u.mutex));
    }

  return EINVAL;
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_spin_trylock (pthread_spinlock_t * lock)
{
  register pthread_spinlock_t s;

  if (NULL == lock || NULL == *lock)
    {
      return (EINVAL);
    }

  if (*lock == PTHREAD_SPINLOCK_INITIALIZER)
    {
      int result;

      if ((result = ptw32_spinlock_check_need_init (lock)) != 0)
	{
	  return (result);
	}
    }

  s = *lock;

  switch ((long)
	  PTW32_INTERLOCKED_COMPARE_EXCHANGE ((PTW32_INTERLOCKED_LPLONG) &
					      (s->interlock),
					      (PTW32_INTERLOCKED_LONG)
					      PTW32_SPIN_LOCKED,
					      (PTW32_INTERLOCKED_LONG)
					      PTW32_SPIN_UNLOCKED))
    {
    case PTW32_SPIN_UNLOCKED:
      return 0;
    case PTW32_SPIN_LOCKED:
      return EBUSY;
    case PTW32_SPIN_USE_MUTEX:
      return pthread_mutex_trylock (&(s->u.mutex));
    }

  return EINVAL;
}
// ===========================SOURCE FILE BREAK===============================

//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "sync.c"
//
//#include "pthread_detach.c"
//#include "pthread_join.c"

#ifndef WINCE
#include <signal.h>
#endif

int
pthread_detach (pthread_t thread)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function detaches the given thread.
      *
      * PARAMETERS
      *      thread
      *              an instance of a pthread_t
      *
      *
      * DESCRIPTION
      *      This function detaches the given thread. You may use it to
      *      detach the main thread or to detach a joinable thread.
      *      NOTE:   detached threads cannot be joined;
      *              storage is freed immediately on termination.
      *
      * RESULTS
      *              0               successfully detached the thread,
      *              EINVAL          thread is not a joinable thread,
      *              ENOSPC          a required resource has been exhausted,
      *              ESRCH           no thread could be found for 'thread',
      *
      * ------------------------------------------------------
      */
{
  int result;
  BOOL destroyIt = PTW32_FALSE;
  ptw32_thread_t * tp = (ptw32_thread_t *) thread.p;

  EnterCriticalSection (&ptw32_thread_reuse_lock);

  if (NULL == tp
      || thread.x != tp->ptHandle.x)
    {
      result = ESRCH;
    }
  else if (PTHREAD_CREATE_DETACHED == tp->detachState)
    {
      result = EINVAL;
    }
  else
    {
      /*
       * Joinable ptw32_thread_t structs are not scavenged until
       * a join or detach is done. The thread may have exited already,
       * but all of the state and locks etc are still there.
       */
      result = 0;

      if (pthread_mutex_lock (&tp->cancelLock) == 0)
	{
	  if (tp->state != PThreadStateLast)
	    {
	      tp->detachState = PTHREAD_CREATE_DETACHED;
	    }
	  else if (tp->detachState != PTHREAD_CREATE_DETACHED)
	    {
	      /*
	       * Thread is joinable and has exited or is exiting.
	       */
	      destroyIt = PTW32_TRUE;
	    }
	  (void) pthread_mutex_unlock (&tp->cancelLock);
	}
      else
	{
	  /* cancelLock shouldn't fail, but if it does ... */
	  result = ESRCH;
	}
    }

  LeaveCriticalSection (&ptw32_thread_reuse_lock);

  if (result == 0)
    {
      /* Thread is joinable */

      if (destroyIt)
	{
	  /* The thread has exited or is exiting but has not been joined or
	   * detached. Need to wait in case it's still exiting.
	   */
	  (void) WaitForSingleObject(tp->threadH, INFINITE);
	  ptw32_threadDestroy (thread);
	}
    }

  return (result);

}				/* pthread_detach */

// ===========================SOURCE FILE BREAK===============================


int
pthread_join (pthread_t thread, void **value_ptr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function waits for 'thread' to terminate and
      *      returns the thread's exit value if 'value_ptr' is not
      *      NULL. This also detaches the thread on successful
      *      completion.
      *
      * PARAMETERS
      *      thread
      *              an instance of pthread_t
      *
      *      value_ptr
      *              pointer to an instance of pointer to void
      *
      *
      * DESCRIPTION
      *      This function waits for 'thread' to terminate and
      *      returns the thread's exit value if 'value_ptr' is not
      *      NULL. This also detaches the thread on successful
      *      completion.
      *      NOTE:   detached threads cannot be joined or canceled
      *
      * RESULTS
      *              0               'thread' has completed
      *              EINVAL          thread is not a joinable thread,
      *              ESRCH           no thread could be found with ID 'thread',
      *              ENOENT          thread couldn't find it's own valid handle,
      *              EDEADLK         attempt to join thread with self
      *
      * ------------------------------------------------------
      */
{
  int result;
  pthread_t self;
  ptw32_thread_t * tp = (ptw32_thread_t *) thread.p;

  EnterCriticalSection (&ptw32_thread_reuse_lock);

  if (NULL == tp
      || thread.x != tp->ptHandle.x)
    {
      result = ESRCH;
    }
  else if (PTHREAD_CREATE_DETACHED == tp->detachState)
    {
      result = EINVAL;
    }
  else
    {
      result = 0;
    }

  LeaveCriticalSection (&ptw32_thread_reuse_lock);

  if (result == 0)
    {
      /* 
       * The target thread is joinable and can't be reused before we join it.
       */
      self = pthread_self();

      if (NULL == self.p)
	{
	  result = ENOENT;
	}
      else if (pthread_equal (self, thread))
	{
	  result = EDEADLK;
	}
      else
	{
	  /*
	   * Pthread_join is a cancelation point.
	   * If we are canceled then our target thread must not be
	   * detached (destroyed). This is guarranteed because
	   * pthreadCancelableWait will not return if we
	   * are canceled.
	   */
	  result = pthreadCancelableWait (tp->threadH);

	  if (0 == result)
	    {
	      if (value_ptr != NULL)
		{
		  *value_ptr = tp->exitStatus;
		}

	      /*
	       * The result of making multiple simultaneous calls to
	       * pthread_join() or pthread_detach() specifying the same
	       * target is undefined.
	       */
	      result = pthread_detach (thread);
	    }
	  else
	    {
	      result = ESRCH;
	    }
	}
    }

  return (result);

}				/* pthread_join */

// ===========================SOURCE FILE BREAK===============================
//////////////////////////////////////////////////////////////////////////////
// CONSOLIDATION - 
// 
//
// #include "tsd.c"
//
//#include "pthread_key_create.c"
//#include "pthread_key_delete.c"
//#include "pthread_setspecific.c"
//#include "pthread_getspecific.c"


/* TLS_OUT_OF_INDEXES not defined on WinCE */
#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES 0xffffffff
#endif

int
pthread_key_create (pthread_key_t * key, void (*destructor) (void *))
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function creates a thread-specific data key visible
      *      to all threads. All existing and new threads have a value
      *      NULL for key until set using pthread_setspecific. When any
      *      thread with a non-NULL value for key terminates, 'destructor'
      *      is called with key's current value for that thread.
      *
      * PARAMETERS
      *      key
      *              pointer to an instance of pthread_key_t
      *
      *
      * DESCRIPTION
      *      This function creates a thread-specific data key visible
      *      to all threads. All existing and new threads have a value
      *      NULL for key until set using pthread_setspecific. When any
      *      thread with a non-NULL value for key terminates, 'destructor'
      *      is called with key's current value for that thread.
      *
      * RESULTS
      *              0               successfully created semaphore,
      *              EAGAIN          insufficient resources or PTHREAD_KEYS_MAX
      *                              exceeded,
      *              ENOMEM          insufficient memory to create the key,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  pthread_key_t newkey;

  if ((newkey = (pthread_key_t) calloc (1, sizeof (*newkey))) == NULL)
    {
      result = ENOMEM;
    }
  else if ((newkey->key = TlsAlloc ()) == TLS_OUT_OF_INDEXES)
    {
      result = EAGAIN;

      free (newkey);
      newkey = NULL;
    }
  else if (destructor != NULL)
    {
      /*
       * Have to manage associations between thread and key;
       * Therefore, need a lock that allows multiple threads
       * to gain exclusive access to the key->threads list.
       *
       * The mutex will only be created when it is first locked.
       */
      newkey->keyLock = PTHREAD_MUTEX_INITIALIZER;
      newkey->destructor = destructor;
    }

  *key = newkey;

  return (result);
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_key_delete (pthread_key_t key)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function deletes a thread-specific data key. This
      *      does not change the value of the thread specific data key
      *      for any thread and does not run the key's destructor
      *      in any thread so it should be used with caution.
      *
      * PARAMETERS
      *      key
      *              pointer to an instance of pthread_key_t
      *
      *
      * DESCRIPTION
      *      This function deletes a thread-specific data key. This
      *      does not change the value of the thread specific data key
      *      for any thread and does not run the key's destructor
      *      in any thread so it should be used with caution.
      *
      * RESULTS
      *              0               successfully deleted the key,
      *              EINVAL          key is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (key != NULL)
    {
      if (key->threads != NULL &&
	  key->destructor != NULL &&
	  pthread_mutex_lock (&(key->keyLock)) == 0)
	{
	  ThreadKeyAssoc *assoc;
	  /*
	   * Run through all Thread<-->Key associations
	   * for this key.
	   *
	   * While we hold at least one of the locks guarding
	   * the assoc, we know that the assoc pointed to by
	   * key->threads is valid.
	   */
	  while ((assoc = (ThreadKeyAssoc *) key->threads) != NULL)
	    {
	      ptw32_thread_t * thread = assoc->thread;

	      if (assoc == NULL)
		{
		  /* Finished */
		  break;
		}

	      if (pthread_mutex_lock (&(thread->threadLock)) == 0)
		{
		  /*
		   * Since we are starting at the head of the key's threads
		   * chain, this will also point key->threads at the next assoc.
		   * While we hold key->keyLock, no other thread can insert
		   * a new assoc via pthread_setspecific.
		   */
		  ptw32_tkAssocDestroy (assoc);
		  (void) pthread_mutex_unlock (&(thread->threadLock));
		}
	      else
		{
		  /* Thread or lock is no longer valid? */
		  ptw32_tkAssocDestroy (assoc);
		}
	    }
	  pthread_mutex_unlock (&(key->keyLock));
	}

      TlsFree (key->key);
      if (key->destructor != NULL)
	{
	  /* A thread could be holding the keyLock */
	  while (EBUSY == pthread_mutex_destroy (&(key->keyLock)))
	    {
	      Sleep(1); // Ugly.
	    }
	}

#if defined( _DEBUG )
      memset ((char *) key, 0, sizeof (*key));
#endif
      free (key);
    }

  return (result);
}
// ===========================SOURCE FILE BREAK===============================

int
pthread_setspecific (pthread_key_t key, const void *value)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function sets the value of the thread specific
      *      key in the calling thread.
      *
      * PARAMETERS
      *      key
      *              an instance of pthread_key_t
      *      value
      *              the value to set key to
      *
      *
      * DESCRIPTION
      *      This function sets the value of the thread specific
      *      key in the calling thread.
      *
      * RESULTS
      *              0               successfully set value
      *              EAGAIN          could not set value
      *              ENOENT          SERIOUS!!
      *
      * ------------------------------------------------------
      */
{
  pthread_t self;
  int result = 0;

  if (key != ptw32_selfThreadKey)
    {
      /*
       * Using pthread_self will implicitly create
       * an instance of pthread_t for the current
       * thread if one wasn't explicitly created
       */
      self = pthread_self ();
      if (self.p == NULL)
	{
	  return ENOENT;
	}
    }
  else
    {
      /*
       * Resolve catch-22 of registering thread with selfThread
       * key
       */
      ptw32_thread_t * sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

      if (sp == NULL)
        {
	  if (value == NULL)
	    {
	      return ENOENT;
	    }
          self = *((pthread_t *) value);
        }
      else
        {
	  self = sp->ptHandle;
        }
    }

  result = 0;

  if (key != NULL)
    {
      if (self.p != NULL && key->destructor != NULL && value != NULL)
	{
	  /*
	   * Only require associations if we have to
	   * call user destroy routine.
	   * Don't need to locate an existing association
	   * when setting data to NULL for WIN32 since the
	   * data is stored with the operating system; not
	   * on the association; setting assoc to NULL short
	   * circuits the search.
	   */
	  ThreadKeyAssoc *assoc;

	  if (pthread_mutex_lock(&(key->keyLock)) == 0)
	    {
	      ptw32_thread_t * sp = (ptw32_thread_t *) self.p;

	      (void) pthread_mutex_lock(&(sp->threadLock));

	      assoc = (ThreadKeyAssoc *) sp->keys;
	      /*
	       * Locate existing association
	       */
	      while (assoc != NULL)
		{
		  if (assoc->key == key)
		    {
		      /*
		       * Association already exists
		       */
		      break;
		    }
		  assoc = assoc->nextKey;
		}

	      /*
	       * create an association if not found
	       */
	      if (assoc == NULL)
		{
		  result = ptw32_tkAssocCreate (sp, key);
		}

	      (void) pthread_mutex_unlock(&(sp->threadLock));
	    }
	  (void) pthread_mutex_unlock(&(key->keyLock));
	}

	if (result == 0)
	  {
	    if (!TlsSetValue (key->key, (LPVOID) value))
	      {
		result = EAGAIN;
	      }
	  }
    }

  return (result);
}				/* pthread_setspecific */
// ===========================SOURCE FILE BREAK===============================

void *
pthread_getspecific (pthread_key_t key)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function returns the current value of key in the
      *      calling thread. If no value has been set for 'key' in 
      *      the thread, NULL is returned.
      *
      * PARAMETERS
      *      key
      *              an instance of pthread_key_t
      *
      *
      * DESCRIPTION
      *      This function returns the current value of key in the
      *      calling thread. If no value has been set for 'key' in 
      *      the thread, NULL is returned.
      *
      * RESULTS
      *              key value or NULL on failure
      *
      * ------------------------------------------------------
      */
{
  void * ptr;

  if (key == NULL)
    {
      ptr = NULL;
    }
  else
    {
      int lasterror = GetLastError ();
#ifdef WSA_ERROR_FIX
	  int lastWSAerror = WSAGetLastError ();
#endif

      ptr = TlsGetValue (key->key);

      SetLastError (lasterror);
#ifdef WSA_ERROR_FIX
	  WSASetLastError (lastWSAerror);
#endif
    }

  return ptr;
}
// ===========================SOURCE FILE BREAK===============================

#ifndef _SELF_INIT_PT
#define _SELF_INIT_PT
struct SelfInitPThreads
{
public:
	int bIsInitialized;
	
	// This is necessary to call directly if you have another global object that initializes any thread objects
	// See AutoSetupDispatchThreadPool::AutoSetupDispatchThreadPool in XferDispatch.cpp
	void RaceResolver()	
	{
		// bIsInitialized is garbage, initially (most often 0 or 1) so 777 is an arbitrary value to indicate initialization
		if (bIsInitialized != 777)
		{
			pthread_win32_process_attach_np();
			bIsInitialized = 777;
		}
	}

	SelfInitPThreads()
	{
#ifndef _NO_SELF_INIT
		RaceResolver();
#endif
	}
	~SelfInitPThreads()
	{
#ifndef _NO_SELF_INIT
		pthread_win32_process_detach_np();
#endif
	}


};

extern SelfInitPThreads g_ThreadsInitialized;

#endif // _SELF_INIT_PT


#ifndef _NO_SELF_INIT
SelfInitPThreads g_ThreadsInitialized;
#endif
