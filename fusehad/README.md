# Hackaday RSS feed, mountable as a normal filesystem

https://hackaday.com/2022/02/16/linux-fu-fusing-hackaday/

![Hackaday RSS filesystem](https://hackaday.com/wp-content/uploads/2022/02/mount.png)

---

## Constraints

To keep things simple, I decided not to worry about performance too much. Since the data is traveling over the network, I do attempt to cache it, and I don’t refresh data later. Of course, you can’t write to the filesystem at all. This is purely for reading Hackaday.

These constraints make things easier. Of course, if you were writing your own filesystem, you might relax some of these, but it still helps to get something as simple as possible working first.


## Making it work first

Speaking of which, the first order of business is to be able to read the Hackaday RSS feed and pull out the parts we need. Again, not worrying about performance, I decided to do that with a pipe and calling out to `curl`. Yes, that’s cheating, but it works just fine, and that’s why we have tools in the first place.

The `HaDFS.cpp` file has a few functions related to FUSE and some helper functions, too. However, I wanted to focus on getting the RSS feed working so I put the related code into a function I made up called `userinit`. I found out the hard way that naming it `init` would conflict with the library.

The normal FUSE system processes your command line arguments — a good thing, as you’ll see soon.

Reading the feed isn’t that hard since I’m conscripting `curl`. Each topic is in a structure and there is an array of these structures. If you try to load too many stories, the code just quietly discards the excess (see `MAXTOPIC`). The `topics` global variable tells how many stories we’ve actually loaded.

The `popen` function runs a command line and gives us the `stdout` stream as a bunch of lines. Processing the lines is just brute force looking for <title> and <link> to identify the data we need. I filtered curl through `grep` to make sure I didn’t get a lot of extra lines, by the way, and I assumed lowercase, but a `-i` option could easily fix that. The redirect is to prevent `curl` from polluting `stderr`, although normally FUSE will disconnect the output streams so it doesn’t really matter. Note that I add an HTML extension to each fake file name so opening one is more likely to get to the browser.


## The Four Functions

There are four functions we need to create in a subclass to get a minimal read-only filesystem going: `getattr`, `readdir`, `open`, and `read`. These functions pretty much do what you expect.  The `getattr` call will return 755 for our root (and only) directory and 444 for any other file that exists. The `readdir` outputs entries for . and .. along with our “files.” `open` and `read` do just what you think they do.

There are some other functions, but those are ones I added to help myself:

 * `userinit` – Called to kick off the file system data
 * `trimrss` – Trim an RSS line to make it easier to parse
 * `pathfind` – Turn a file name into a descriptor (an index into the array of topics)
 * `readurl` – Return a string with the contents of a URL (uses curl)

There’s not much to it. You’ll see in the code that there are a few things to look out for like catching someone trying to write to a file since that isn’t allowed.


## Debugging and command line options

Of course, it doesn’t matter how simple it is, it isn’t going to work the first time is it? But, of course, things won’t work like you expect for any of a number of reasons. There are a few things to remember as you go about running and debugging.

When you build your executable, you simply run it and provide a command line argument to specify the mount point which, of course, should exist. I have a habit of using `/tmp/mnt` while debugging, but it can be anywhere you have permissions.

Under normal operation, FUSE detaches your program so you can’t just kill it. You’ll need to use unmount command (`fusermount -u`) with the mount point as an argument. Even if your program dies with a segment fault, you’ll need to use the unmount command or you will probably get the dreaded “disconnected endpoint” error message.

Being detached leads to a problem. If you put `printf` statements in your code, they will never show up after detachment. For this reason, FUSE understands the `-f` flag which tells the system to keep your filesystem running in the foreground. Then you can see messages and a clean exit, like a Control+C, will cleanly unmount the filesystem. You can also use `-d` which enables some built-in debugging and implies `-f`. The `-s` flag turns off threading which can make debugging easier, or harder if you are dealing with a thread-related problem.

You can use `gdb`, and there are some [good articles](https://blog.jeffli.me/blog/2014/08/30/use-gdb-to-understand-fuse-file-system/) about that. But for such a simple piece of code, it isn’t really necessary.


## What's next?

Read the rest of [the article on hackaday](https://hackaday.com/2022/02/16/linux-fu-fusing-hackaday/) to find out more related information.


