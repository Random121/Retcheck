# Retcheck.h

Retcheck::patch returns a function where it patches any instances of retcheck.
However, it doesn't rely on a retcheck signature to replace a `jb` but instead
it uses a more reliable signature that spoofs the address it checks.

And with even less lines, this will remove all instances of retcheck in the function.

