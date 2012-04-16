libmockspotify
==============

A mock of libspotify for use in development of language bindings.

This project was started in July 2011 with the goal of splitting out
pyspotify's mock of libspotify, so that it could be reused by the developers of
the Node.js and Ruby bindings for libspotify.

Coordination of the development efforts happens on GitHub and #spotify on
Freenode.

Note about my fork
------------------

Initially, mopidy/libmockspotify did not define all functions available from
libspotify. I’ve since forked the project, implemented things in my own way,
and the changes are merged upstream once in a while.

This fork aims to implement all public functions of the libspotify API, but it
only offers a very primitive set of support for callbacks. I will continue my
work, and implement new functions of libspotify, as long as the Ruby bindings
for libspotify ([Hallon](http://github.com/Burgestrand/Hallon)) are maintained.

Installation
------------

::

    $ ./autogen.sh
    $ ./configure
    $ make
    # make install
