renamemer
===

I wrote this thing for myself, and I've spent probably around eight hours
working on it in total. As such, it's not exactly a polished or complete
application. It should function as intended, but note that it theoretically has
the ability to move around all the files on your computer in a way that could
break your OS install, like if you pointed it at System32 and started moving
junk around, obviously that would be bad. What I'm saying is that there are no
real safeguards, but it's also just a really simple hacked together utility, so
hopefully it doesn't cause too much trouble. It has exactly as many features as
I wanted it to have for my own purposes at this point. That being said, if
anybody else finds it useful and has suggestions or would like it to be a more
polished experience, I'm open to all that.

So basically this thing enables you to organize a directory full of media files
really quickly. Here are some of its killer features:

1. Preview common image and video formats (jpg, png, gif[with animation], bmp,
   webm, ogg, avi, etc.)
2. Intelligent keyboard-based control scheme
3. bash-style tab completion of directory names
4. It's got like a list thing on the right side of the files you're gonna
   organize and uhh..
5. You can go real fast

Basically it makes it so that instead of having to like drag and drop files, or
right-click -> rename or whatever, dealing with all that slow UI stuff, you
just type and hit enter. In practice, by far the hardest part of organizing your
memes will be deciding on names for stuff.

Manual
---
You're gonna want to read this because you will get no other instruction inside 
the app. These are the things you want to do:

1. Download the [release package](https://github.com/lyleunderwood/renamemer/releases/) for your platform.
2. If you want video previews you need to install codecs. I'm gonna assume the
   target market for this either has already installed or knows how to install
   a codec pack. If not, google it.
3. Extract that bad boy and open up the `renamemer` folder
4. Navigate into the `bin` directory and run `renamemer.exe` (renamemer.sh for
   linux)
5. Choose the target / base directory. You can either do this by typing the
   path in the Base Folder input or by going to File -> Browse something I
   don't remember what it's called. Also tab completion works in the Base
   Folder input, so you can start typing a folder name and hit TAB to complete
   it. You'll want to get used to this mechanic. Hit ENTER when you're done
   typing in the Base Folder and your cursor will drop down into the Filename
   input so you can start renaming stuff.
6. You should see a list of files on the right-hand side ordered by creation
   date, and previews showing up under the inputs. You can click on stuff on
   the right to select it and see the preview and stuff.
7. If you start typing in the filename input, the name portion of the filename
   will be replaced while the extension is retained. If you change the name and
   hit ENTER the file will be renamed and removed from the list. If you hit
   ENTER and the name is not different the file will just be skipped. If you
   hit ESC at any time the filename will be reset in the input to the original
   filename. If you add directories by doing like somedir/otherdir/file.png and hit
   ENTER then the whole directory chain will be created automatically and the
   file placed in it and renamed as necessary. Also, directory names can be
   tab-completed, so you can do stuff like: anim _**tab**_ k- _**tab**_ rit 
   _**tab**_ and it'll fill in folder names for you.

That's basically it. So far this thing has been tested on win7 for less than
five minutes, but it's a pretty simple app so hopefully there aren't a ton of
bugs all over the place. If you have problems then you might try running it in
windows 7 compatibility mode or whatever. Trying to package this for OSX would
be a huge pain in the ass so I'm not gonna do it.

There are some other possible features I've considered like sort of a folder
hotkey system or something that shows you the possible folders when you have an
ambiguous tab completion, but we'll see.

Getting webm preview working in Windows
---
Basically renamemer needs to be able to find codecs for the video files you
want to preview. I've tested K-Lite and it seems to work perfectly. CCCP 32bit
also seems to work. So far I've only put together a 32bit release, so you'll
need a 32bit webm codec. If you don't have one, or webm isn't working for some
reason, or you don't want to install a whole codec pack, try downloading the
most recent webmdshow zip package from
[here](http://storage.googleapis.com/downloads.webmproject.org/releases/webm/)
and running the install exe. This should install a 32bit webm codec.

Getting GStreamer working in Ubuntu
---
GStreamer is all dumb in Ubuntu for reasons which are confusing as hell. Here's
how I got webm previews working:

Add this to your sources.list:
```
deb http://ppa.launchpad.net/mc3man/trusty-media/ubuntu trusty main
```

And then:
```
sudo apt-get update && sudo apt-get install gstreamer0.10-ffmpeg
```

Development
---
If you want to actually build renamemer for any reason, just install QtCreator
and open the .pro file and hit build.