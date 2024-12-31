# Waterloo CSC 1987 Othello Entry

I kept this code around over the years and I thought I would post it as a warning to others.

If you want to know what you get when a decent coder tries to write an Othello program from scratch
in about 16 hours straight (all night) for tournament the next day this is it.

What can I say, it could be worse.

I had to make a few very minor changes to reduce warnings from ridiculous to just awful.

BTW: This program did not win, or even place, but it's never been beaten by a human since about 6 hours
or so into its development which I guess tells you something about how hard Othello is.  Also it ran *much*
slower on some slice of a VAX 780.  These days it's ridiculously fast but originally I was lucky to evaluate
500 boards/sec.

The "close to the original 1987 source code" is in the ./original subdirectory.

In the main directory I'm going to modernize and comment the source.  Also, the naming convention I
used when my identifiers are capped at 8 letters kind of sucked, so I'll rename a bunch of things too.
As it is I had even re-used names in confusing ways between files.  So I'm fixing all of that too. I will
to keep the basics the same and do clean up only. That's already a lot.  The number warnings in the
original code is staggering.

There are subdirectories for C# versions:

* ./wrath-sharp-std-arrays : a version that uses standard arrays for board storage, highly portable
* ./wrath-sharp-inline-arrays : a version that uses C# 12 inline arrays for storage, best performance
* ./wrath-sharp-vec128 : a version that uses Vector128 for board storage, this turned out to be a horrible idea

The branches are:

* main -- the version I benchmarked in my articles
* better_flip -- with improvements in the flipping algorithm, materially better
* me_him -- passing two pointers for the current player and other player (makes little difference)

Analysis articles:

* [Part One](https://medium.com/@ricomariani/wrathmark-an-interesting-compute-workload-part-1-47d61e0bea43)
* [Part Two](https://ricomariani.medium.com/wrathmark-an-interesting-compute-workload-part-2-bac27c7f0c7d)
* [Array Bounds Experiment](https://medium.com/@ricomariani/array-bounds-checking-in-2024-06b4ddf26309)
