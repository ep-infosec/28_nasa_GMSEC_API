# Copyright 2007-2021 United States Government as represented by the
# Administrator of The National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Rights Reserved.



use ExtUtils::MakeMaker;

use Util;

$opts{CCFLAGS} = "-w -P $Config{ccflags}" if $Config{cc} eq 'gcc';
$opts{CCFLAGS} =~ s/-Wdeclaration-after-statement//ig;


WriteMakefile(
	Util::args('GMSECAPI::Connection'),
	CCFLAGS => $opts{CCFLAGS},
	OBJECT => 'Connection$(OBJ_EXT)',
);
