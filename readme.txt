Game Music Box for Lion (GaMBL)
-----------------------------------

I've taken the original Game Music Box source and started porting it to 
OSX 10.7+ (Lion).  Most of the files and documentation are still 
as Shay Green, the original author, first created them.  I am in the process of
pruning deprecated code and updating the documentation, if you want to work with
the source please contact me.  This is my first open source project as well as my 
first stab at Cocoa, so don't be surprised if I get some things wrong along the way.

- David Ventura, 2013.04.01 (April Fool's!)

************************************
CURRENT STATUS (2013.10.19) v0.2.5
************************************

Program runs and can perform basic playback and track jump prev/next for multitrack 
files. Lots of hacks abound to get it working but finally have proof-of-concept 
running to prove that this is a viable project.  Cleaning up deprecated functionality
is currently low-priority.  Refactoring of the window and audio classes is
my first priority.

Alpha version is available for download in .app form from the GitHub Wiki:
https://github.com/dgventura/gambl/wiki

Next major tasks include:

-- BETA VERSION 0.3 --
1. Refactor classes, data flow, information hiding etc. so the model makes sense
2. Fix favourites to allow individual track selections
3. Support for 7zip archives
4. Add a playlist editor
5. Utility window
6. Restore fast-forward
7. Consider adding some audio units to add in effects
8. Release this binary as beta

- David


------------------
------------------
The following readme was written by Shay Green, but I've modified parts to
reflect changes I've made to GaMBL.
------------------
------------------


Core Functionality
------------------

* Play emulator files for most 8 and 16-bit systems.
	NSF, GBS, SPC, GYM, VGM, (RAR, ZIP of aforementioned systems)
* Basic keyboard and mouse controls for play/pause/next/prev track(s).
* Adjust play time of tracks and shuffle play order.
* Rudimentary equaliser and NSF channel toggle.
* Export to WAV.
* Tag albums as favorites.

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

In general, Shay's classes have an underbar in the name
(Player_Window), whereas the classes I've written are in the Cocoa
convention (PlayerWindow).  This is confusing, but over time I'll remove
the deprecated classes as I refactor.

Architecture
------------

(*asterisk indicates currently unsupported but planned functionality*)

GaMBL uses Cocoa in place of the legacy Carbon window classes that
controlled application flow in Game Music Box.  The model view controller
places the window interactions in each of the controller classes.

PlayerViewController presents the main window, *queues dropped files, 
and plays them with AudioPlayer.

ChannelViewController handles interaction with AudioPlayer for
the equaliser and channel muting.

AppDelegate handles application launch, shutdown, and file operations.

AudioPlayer is a logical sound player which interacts with the legacy
Music_Player which in turn uses File_Emu and AudioInterface 
to play tracks from a file.

AudioInterface is a minimal wrapper over Core Audio.

Music_Album gives a common interface to archives of music files, single
music files, and music albums. It determines a file's type, creates the
appropriate emulator to load file into, and allows access to optional
extended track information.

File_Emu synthesizes samples from loaded files with current playback
settings, using Game_Music_Emu. 

Wave_export handles wave exporting, using File_Emu to synthesize and
Wave_Writer for output.

App_Prefs manages the preferences file and window.

How to build
------------
You should be able to grab the repository directory from Git, and build
as is on Xcode 4+ on OS X 10.7 or later.  Unfortunately, THIS DOES NOT WORK
as there are some environment problems I haven't yet resolved.  If you 
really prefer building yourself as opposed to the binary, LET ME KNOW. :)


Sound Queue
-----------

MusicPlayer has an internal thread-safe sound queue. Blocks of samples
are queued and eventually played. The queue is kept filled from the
Core Audio callback that requests samples for the audio hardware
abstraction layer (HAL) at fixed intervals.

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

GaMBL sources are licensed under the  GNU Lesser General 
Public License (LGPL); see LGPL.txt. Copyright (C) 2013 David Ventura.

Game_Music_Box sources are licensed under the  GNU Lesser General 
Public License (LGPL); see LGPL.txt. Copyright (C) 2005 Shay Green.

Game_Music_Emu is licensed under the GNU Lesser General Public License
(LGPL); see LGPL.txt. Copyright (C) 2003-2005 Shay Green. SNES SPC DSP
emulator based on OpenSPC, Copyright (C) 2002 Brad Martin. Sega Genesis
YM2612 emulator from Gens project, Copyright (C) 2002 Stephane
Dallongeville.

zlib is licensed as described in zlib.h.

-- 
David Ventura 
http://perfectforwarding.tumblr.com
info # ichigoichie # org (change pounds to appropriate symbols) 
