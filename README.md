# esh(1) - the environmental meta-shell

16 January 1990

```
esh [-B | -C | -I | -T] [-D] [-E sysenv] [-F usrenv] [-L] [-S shell] [shell_args...]
```

<a name="description"></a>

# Description

_Esh_ is an environmental meta-shell that performs the task of setting up
the user's initial environment before executing the user's real shell. The
intended usage of _esh_ is not as an interactive program, but as the value
of shell field in ```/etc/passwd```. When the user logs in and is given
_esh_ as his shell, it will start by scanning a site wide environment file,
```/usr/local/etc/environ```, for variable bindings and create an initial
environment. It will the proceed to read the user's personal environment
file, ```~/.environ```, and augument the environment with his preferences.
It will then replace itself with the user's real shell, as determined by the
value of the environment variable ```SHELL``` or by finding and inspecting
```~/.shell``` in the user's home directory. If neither of the above are
found, _esh_ will transfer control to a predefined shell, currently
```/bin/bash```.

A typical site wide usage of _esh_ would be to make it every user's shell in
```/etc/passwd```. The system administrator would then create a site
specific ```/usr/local/etc/environ``` that sets up appropriate values for
```PATH```, ```MANPATH```, ```LD_LIBRARY_PATH```, etc. In the simplest case,
the user would then create a .shell file in his home directory and record
the name of his preferred shell in there (or alternatively, make it a
symbolic link to his/her shell). Everything would then be the same as
before, i.e. environment variables would be set in ```.profile``` for
sh/ksh/bash users and ```.login``` for csh/tcsh users, but with the added
advantage of already having a reasonable environment.

Alternatively, the user could create a .environ file in his home directory
and make all his environment bindings there. This should include a value for
```SHELL```, which will be used by _esh_ to determine his preferred shell in
the absense of a ```.shell``` file. This approach has the additional
advantage of being shell independent, i.e. if the user decides to change
shell (by editing ```.environ``` or changing ```.shell```), his personal
environment will automatically be available under the new shell.

If _esh_ is compiled with the ```AUTO_PRUNE_PATH``` option turned on, all
variables with names ending in ```PATH``` will be assumed to contain values
of the form: _dir1_:_dir2_:_dir3_:...:_dirn_ i.e. as a list of colon
separated directories or files. These variables will be automatically pruned
of duplications, so that if a _diri_ appears more than once in the sequence,
all but the first one will be removed. In addition, null fields are removed.
This is useful in that it allows a user to freely add to an already existing
path variable without having to worry about possible duplications or null
entries.

The format of an environment binding in ```/usr/local/etc/environ``` or
```.environ``` is as follows:

```
    <name> [=]	<value>[\
                <continued_value>]
```

That is, an environment variable name should be followed by whitespace or an
equal sign and a value. Values that don't fit on one line may be continued
on the next one if the current line ends with a backslash (```\```).
Comments may appear anywhere on the line by inserting a hash sign (```#```)
and goes on until the end of the line. Multiline comments begin with
```#<``` and end with ```#>```; all lines between ```#<``` ... ```#>``` will
be ignored. Values may include other the value of other variables by the
usual ```$VARIABLE``` syntax or to the output a program by enclosing it in
backquotes (``` ` ```). Finally, backslashes may be used to quote any other
character, except newline.

For example:

```
    	# Setup default paths, etc.
    	OPENWINHOME  /usr/openwin	# not /home/openwin
    	PATH         .:/usr/local/bin.`arch`:/bin:/usr/bin:/usr/ucb:\
                     $OPENWINHOME/bin
```

If the environment files are changed, the user will normally need to logout
and login again for the changes to take effect. Alternatively, he can also
execute _esh_ interactively, in which case it will give him a new shell
augmented with the new environment. The old shell will still remain behind
_esh_, unless it is replaced using the traditional _exec_ command. Since
some properties of the current shell, such as temporary aliases and the
history, may be lost by this procedure, two special _esh_ options will turn
_esh_ into an environment translator that produces an ASCII representation
of the bindings in either _sh_ or _csh_ format. The **-B** option will
select _Bourne shell_ syntax, while **-C** will produce _C-shell_ output.
When either of these options is selected, no new shell will be spawned, but
control will be released back to the invoker.

# Options

* **-B** — Don't create a new shell, just print out the environment
  bindings in _Bourne shell_ format, suitable for being sourced by _sh_(1).

* **-C** — Don't create a new shell, just print out the environment
  bindings in _C-shell_ format, suitable for being sourced by _csh_(1).

* **-D** — Turn on debugging output. With this flag, _esh_ will print all
  environment bindings as they are parsed in addition to some other output.

* **-E** _sysenv_ — The name of the system global environment file, by
  default ```/usr/local/etc/environ```.

* **-F** _usrenv_ — The name of the user's personal environment file, by
  default ```~/.environ```.

* **-L** — Pretend to be a login shell. _Esh_ will make the new shell a
  login shell only if it is a login shell itself (i.e. has a dash as the
  first char of ```argv[0]```).

* **-S** _shell_ — The name of the program that _esh_ should replace itself
  with after having set up the initial environment. This option will
  override the normal procedure of selecting the shell from the ```SHELL```
  environment variable (or by looking at the user's ```.shell``` file, if
  one exists).

* **-T** — Don't create a new shell, just print out the environment
  bindings in text format with ```=``` separating each variable from its
  value.

* **-X** — Don't create a new shell, just print out the environment
  bindings in _Emacs Lisp_ format, suitable for being sourced by GNU Emacs.

# Examples

```
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
    .:/home/lovstrand/bin:/usr/local/bin:/bin:/usr/bin
    $ cat >>.environ
    MANPATH	$HOME/man:$MANPATH
    ^D
    $ esh -B >foo
    $ source foo
    $ echo $MANPATH
    /home/lovstrand/man
```

# Author

Lennart Lovstrand &lt;esh@lenlolabs.com&gt;

# See Also

_csh_(1),
_sh_(1),
_tcsh_(l)

# Bugs

_Esh_ has a compiled-in limit of 10,240 chars for each environment binding
and a total of 1,024 bindings.
