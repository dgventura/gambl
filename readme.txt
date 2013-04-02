Game Music Box for Lion
--------------

I've taken the original Game Music Box source and started porting it to 
OSX 10.7+ (Lion), not sure how often I'll be able to work on it but 
hopefully at least getting the non-UI pieces compiling and linking 
shouldn't take too long.  Most of the files and documentation are still 
as Shay Green, the original author, first created them.  This is my first 
open source project as well as my first stab at Cocoa, so don't be surprised
if I get some things wrong along the way.

- David Ventura, 2013.04.01 (April Fool's!)

---------------

Files compile but don't link from unresolved symbols, a couple libraries and
some parts of the code ifdef'd out.  Looking at the code now it seems that a 
number of deprecated systems need to be rewritten completely to bring the
project into the 2010s mainly:

1. application layer with Model-View-Controller via Cocoa (Carbon is deprecated)
2. file management via NSFileHandle methods (FS file system is deprecated)
3. threading and process managment (DT deferred tasks are deprecated)
4. audio playback using CoreAudio (Snd APIs are deprecated)

Which leaves us with the game system hardware emulation, reading of the audio 
file formats, and some minor utility code, looking at the source I'd say about 
a 60% rewrite?  I wanted to learn about CoreAudio and OSX programming anyway 
so it'll be good practice.  Don't expect anything too soon though. :)

- David, 2013.04.02

--------------


Contact me if you have any questions about the code or design.


Compiling
---------

Be sure setup/prefix.pch++ and Rar_Extractor/rar_suballoc.inl were
properly set as text files or the compiler might refuse to open them.

I use an old version of CodeWarrior (Pro 4) for development so some
changes might be required when compiling with a more recent version.

I use a precompiled header derived from setup/prefix.pch++, saved to a
RAM disk for faster compilation.

Build configuration is done in setup/config.h.

Optimization in the IDE is set for the release build, then disabled
during development in config.h, using #pragma.


Core Functionality
------------------

Evaluate new music:
	- Fast-forward to get to main part of track quickly.
	- Shorten track lengths so songs can be gone over in less time.
	- Play files in order they are dropped (i.e. alphabetical).
	- Optionally change file type immediately, so newly downloaded files
can get a proper icon

Listen to music in the background:
	- Optional track shuffling (might want to listen to album in order).
	- Fade tracks after they've played a while.
	- Customize equalization for "classic" systems, since their sound can
be irritating.
	- Minimize user-interface and have no often-changing display that would
distract in the background (i.e. hide elapsed time).

Record music to sound files:
	- Batch mode for converting a collection.
	- Save current track with custom channel muting.

Music utility
	- Set icons, remove filename extensions
	- Rename based on information tags
	- Compress/expand
	- Check music integrity


Design Decisions
----------------

Favor simpler code over unnecessary functionality. Favor functionality
over appearance.

Report errors using exceptions.

To limit complexity, keep most source files under 400 lines. Delegate
behavior to sub-modules, passive if possible.

Always check error return values, even if only when debugging is
enabled.

Prefer rigid code that is more likely to reveal bugs than flexible code
that hides bugs.


Architecture
------------

Player_Window presents the main window, queues dropped files, and plays
them with Music_Player.

Music_Player uses File_Emu and Audio_Player to play tracks from a file.

Audio_Player is a minimal wrapper over the Sound Manager.

Music_Album gives a common interface to archives of music files, single
music files, and music albums. It determines a file's type, creates the
appropriate emulator to load file into, and allows access to optional
extended track information.

File_Emu synthesizes samples from loaded files with current playback
settings, using Game_Music_Emu. 

Wave_export handles wave exporting, using File_Emu to synthesize and
Wave_Writer for output.

App_Prefs manages the preferences file and window.

Scope_Window handles the sound scope.


Sound Queue
-----------

Music_Player has an internal thread-safe sound queue. Blocks of samples
are queued and eventually played. The queue is kept filled from a
deferred task.

When a synthesized block is found to be all silent, end-of-track silence
detection starts filling blocks faster than real-time, looking ahead for
non-silence. This slowly fills up the queue. If non-silence is found
soon enough, filling stops until the queue's level falls back to normal.
But since the filled blocks would all be silent, a count of silent
blocks is kept instead of adding them to the queue; this allows a much
smaller queue.

Fast-forward turns off asynchronous queue filling and keeps a minimum in
the queue while it skips ahead. It runs at a regular interval, skipping
as much as four seconds each time (less if the processor isn't fast
enough to keep up) then adding two blocks, faded out at each end to
eliminate clicks.

The normal fill level of the queue is around half a second. This is
lowered to around 1/10 second when the sound channels window is open in
order to provide more immediate feedback to changes in muting and sound
customization sliders.


Libraries
---------

Specialized/ has special-purpose modules not specific to the player
(uses exceptions).

Lib/ has general modules (uses exceptions).

Game_Music_Emu is used for all synthesis (uses error return values). See
http://www.slack.net/~ant/libs/

Zlib/ has only the sources needed for in-memory operations.

Rar_Extractor/ is a RAR extraction library, based on UnRAR.

Boost/ has a minimal reimplementation of a few basic boost modules. It
should be possible to remove this and use the real boost. See
http://www.boost.org/


Copyright/Licensing
-------------------

The program consists of many parts. Each source file identifies which
part it belongs to and what its license is.

Game_Music_Box sources are licensed under the GNU Lesser General Public
License (LGPL); see LGPL.txt. Copyright (C) 2005 Shay Green.

Game_Music_Emu is licensed under the GNU Lesser General Public License
(LGPL); see LGPL.txt. Copyright (C) 2003-2005 Shay Green. SNES SPC DSP
emulator based on OpenSPC, Copyright (C) 2002 Brad Martin. Sega Genesis
YM2612 emulator from Gens project, Copyright (C) 2002 Stephane
Dallongeville.

zlib is licensed as described in zlib.h.

-- 
Shay Green <hotpop.com@blargg> (swap to e-mail)
