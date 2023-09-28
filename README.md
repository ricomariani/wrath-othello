# Waterloo CSC 1987 Othello Entry

I kept this code around over the years and I thought I would post it as a warning to others.

If you want to know what you get a decent coder tries to write an Othello program from scratch
in about 16 hours straight (all night) for tournament the next day this is it.

What can I say, it could be worse.

I had to make a few very minor changes to reduce warnings from ridiculous to just awful.

BTW: This program did not win, or even place, but it's never been beaten by a human since about 6 hours
or so into its development which I guess tells you something about how hard Othello is.  Also it ran *much*
slower on some slice of a VAX 780.  These days it's ridiculously fast but originally I was lucky to evaluate
500 boards/sec.

The "close to the original 1987 source code" is in the original subdirectory.

The in this directory I'm going to modernize and comment source.  Also, the naming convention you use
when your identifiers are capped at 8 letters kind of sucks, so I'll rename a bunch of things too.
As it is I had even re-used names in confusing ways between files.  So I'm fixing all of that too.

At some point I might start trying to use modern features like vector instructions or something.  But
at least for now I will just keep the basics the same and clean up only. That's already a lot.  The
number warnings in the original code is staggering.
