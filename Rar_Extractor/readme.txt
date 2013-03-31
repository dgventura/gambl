Rar_Extractor 0.2.1
-------------------

Rar_Extractor allows access to files within RAR archives. All I/O is done
through the Rar_Reader and Rar_Writer interface classes; no OS calls are made.
This is meant for programs which transparently access items in RAR archives.

This hasn't gotten much feedback from others or polish as a standalone
library. Please contact me if you'd like it improved.

This library is based on UnRAR 3.5.1 by Alexander L. Roshal. The original
sources have been heavily modified and trimmed down. Refer to license.txt for
full licensing terms. The original source is available at
http://www.rarlab.com


Interface
---------
See Rar_Extractor.h for reference.

Out of memory errors are reported by calling rar_out_of_memory(), which must
be defined somewhere in user code. It *must* throw an exception or perform a
longjmp(). All other errors are reported through error_t return values.

unrarlib-interface/ allows use of Rar_Extractor from C, using the same
interface as unrarlib.


Limitations
-----------
- Archives using encryption and/or split archive files are not supported.

- Error return values from Rar_Reader::read() and Rar_Writer::write() are
currently ignored.

- Unicode filenames in archives are currently ignored.


Notes
-----
It is important to provide 1 byte alignment for structures in model.hpp. Now
it contains '#pragma pack(1)' directive, but your compiler may require
something else. Though Unrar should work with other model.hpp alignments, its
memory requirements may increase significantly. Alignment in other modules is
not important.

Dmitry Shkarin wrote PPMII text compression. Dmitry Subbotin wrote the
carryless rangecoder.

Unrar source may be used in any software to handle RAR archives without
limitations free of charge, but cannot be used to re-create the RAR
compression algorithm, which is proprietary. Distribution of modified Unrar
source in separate form or as a part of other software is permitted, provided
that it is clearly stated in the documentation and source comments that the
code may not be used to develop a RAR (WinRAR) compatible archiver.

More detailed license text is available in license.txt.

-- 
Shay Green <hotpop.com@blargg> (swap to e-mail)
