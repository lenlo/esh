.TH ESH 1 "16 January 1990"				\" -*- nroff -*-
.SH NAME
esh \- the environmental meta-shell
.SH SYNOPSIS
.BR esh
.RB [\| -B
|
.B -C
|
.B -T
|
.BR -X \|]
.RB [\| -D \|]
.RB [\| -E
.IR sysenv \|]
.RB [\| -F
.IR usrenv \|]
.RB [\| -L \|]
.RB [\| -S
.IR shell \|]
.RI [\| shell_args\|.\|.\|. \|]
.SH DESCRIPTION
.I Esh
is an environmental meta-shell that performs the task of setting up
the user's initial environment before executing the user's real shell.
The intended usage of
.I esh
is not as an interactive program, but as the value of shell field in
/etc/passwd. When the user logs in and is given
.I esh
as his shell, it will start by scanning a site wide environment file,
ETCDIR/environ, for variable bindings and create an initial environment.  It
will the proceed to read the user's personal environment file, ~/.environ, and
augument the environment with his preferences.  It will then replace itself 
with the user's real shell, as determined by the value of the environment
variable SHELL or by finding and inspecting ~/.shell in the user's home
directory.  If neither of the above are found,
.I esh
will transfer control to a predefined shell, currently /bin/bash.
.PP
A typical site wide usage of
.I esh
would be to make it every user's shell in /etc/passwd. The system
administrator would then create a site specific ETCDIR/environ that
sets up appropriate values for PATH, MANPATH, LD_LIBRARY_PATH, etc. In
the simplest case, the user would then create a .shell file in his
home directory and record the name of his preferred shell in there (or
alternatively, make it a symbolic link to his/her shell). Everything
would then be the same as before, i.e. environment variables would be
set in .profile for sh/ksh/bash users and .login for csh/tcsh users,
but with the added advantage of already having a reasonable
environment.
.PP
Alternatively, the user could create a .environ file in his home directory and
make all his environment bindings there.  This should include a value for
SHELL, which will be used by
.I esh
to determine his preferred shell in the absense of a .shell file.  This
approach has the additional advantage of being shell independent, ie. if the
user decides to change shell (by editing .environ or changing .shell), his
personal environment will automatically be available under the new shell.
.PP
If
.I esh
is compiled with the AUTO_PRUNE_PATH option turned on, all variables with
names ending in "PATH" will be assumed to contain values of the form:
.IR dir\s-2\d1\u\s+2\fR\|:\|\fIdir\s-2\d2\u\s+2\fR\|:\|\fIdir\s-2\d3\u\s+2\fR\|:\|...\|:\|\fIdir\s-2\dn\u\s+2 ,
ie. as a list of colon separated directories or files.  These variables will
be automatically pruned of duplications, so that if a
.I dir\s-2\di\u\s+2
appears more than once in the sequence, all but the first one will be
removed.  In addition, null fields are removed.
This is useful in that it allows a user to freely add to an already
existing path variable without having to worry about possible duplications or
null entries.
.PP
The format of an environment binding in ETCDIR/environ or .environ is as
follows:
.nf
.ta 0.5i +\w'<name> [=] 'u
	<name> [=]	<value>[\\
		<continued_value>]
.fi
That is, an environment variable name should be followed by whitespace or an
equal sign and a value. Values that don't fit on one line may be continued
on the next one if the current line ends with a backslash (\\). Comments may
appear anywhere on the line by inserting a hash sign (#) and goes on until
the end of the line. Multiline comments are begins with #< and ends with #>;
all lines between #< ... #> will be ignored. Values may include other the
value of other variables by the usual $VARIABLE syntax or to the output a
program by enclosing it in $(...) or backquotes (`...`). Errors are ignored
if the command is prefixed by a question mark (?). Finally, backslashes may
be used to quote any other character, except newline.
.sp
.nf
.ta 0.5i +\w'OPENWINHOME   'u +\w'/usr/openwin   'u
For example:
.sp 0.5
	# Setup default paths, etc.
	OPENWINHOME	/usr/openwin	# not /home/openwin
	PATH	.:/usr/local/bin.`arch`:/bin:/usr/bin:/usr/ucb:\\
		$OPENWINHOME/bin
.fi
.PP
If the environment files are changed, the user will normally need to logout
and login again for the changes to take effect.  Alternatively, he can also
execute
.I esh
interactively, in which case it will give him a new shell augmented with the
new environment.  The old shell will still remain behind
.IR esh ,
unless it is replaced using the traditional
.I exec
command.  Since some properties of the current shell, such as temporary
aliases and the history, may be lost by this procedure, two special
.I esh
options will turn
.I esh
into an environment translator that produces an ASCII representation of the
bindings in either
.I sh
or
.I csh
format.  The
.B \-B
option will select
.I "Bourne shell"
syntax, while
.B \-C
will produce
.I C-shell
output.  When either of these options is selected, no new shell will be
spawned, but control will be released back to the invoker.
.SH OPTIONS
.TP
.B \-B
Don't create a new shell, just print out the environment bindings in
.I "Bourne shell"
format, suitable for being sourced by
.IR sh (1).
.TP
.B \-C
Don't create a new shell, just print out the environment bindings in
.I "C-shell"
format, suitable for being sourced by
.IR csh (1).
.TP
.B \-D
Turn on debugging output.  With this flag,
.I esh
will print all environment bindings as they are parsed in addition to some
other output.
.TP
.BI \-E " sysenv"
The name of the system global environment file, by default ETCDIR/environ.
.TP
.BI \-F " usrenv"
The name of the user's personal environment file, by default ~/.environ.
.TP
.B \-L
Pretend to be a login shell.
.I Esh
will make the new shell a login shell only if it is a login shell itself
(ie. has a dash as the first char of argv[0]).
.TP
.BI \-S " shell"
The name of the program that
.I esh
should replace itself with after having set up the initial environment.  This
option will override the normal procedure of selecting the shell from the
SHELL environment variable (or by looking at the user's .shell file, if one
exists).
.TP
.B \-T
Don't create a new shell, just print out the environment bindings in text format with '=' separating each variable from its value.
.TP
.B \-X
Don't create a new shell, just print out the environment bindings in
.I "Emacs Lisp"
format, suitable for being sourced by GNU Emacs.
.SH EXAMPLES
.nf
.ta \w'OPENWINHOME   'u
$ chsh
Old shell: /bin/bash
New shell: /usr/local/bin/esh
$ cat /etc/environ
PATH	.:/usr/local/bin:/bin:/usr/ucb:/usr/games
$ cd
$ cat >.environ
SHELL	/bin/bash
PATH	.:$HOME/bin:$PATH
^D
$ login
[...]
$ echo $SHELL
/bin/bash
$ echo $PATH
\&.:/home/lovstrand/bin:/usr/local/bin:/bin:/usr/bin
$ cat >>.environ
MANPATH	$HOME/man:$MANPATH
^D
$ esh -B >foo
$ source foo
$ echo $MANPATH
/home/lovstrand/man
.fi
.SH AUTHOR
.nf
Lennart Lovstrand <esh@lenlolabs.com>
.fi
.SH "SEE ALSO"
.IR csh (1),
.IR sh (1),
.IR tcsh (l)
.SH BUGS
.I Esh
has a compiled-in limit of 10,240 chars for each environment binding and a
total of 1,024 bindings.
