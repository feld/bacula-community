#!/usr/bin/perl -w
use strict;
my $file;

print "
from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

extensions = [";

while (my $l = <>)
{
    chomp($l);
    next if ($l =~ /__init__.py/);
    $l =~ s:^./::;
    $file = $l;
    $l =~ s:/:.:g;
    $l =~ s:\.py$::;
    print "Extension('$l', ['$file']),\n";
}

print "]


setup(
    name='k8s_backend',
    scripts=['bin/k8s_backend'],
    setup_requires=['cython'],
    ext_modules=cythonize(extensions),
)
# find . -name '*.py' | ./mkExt.pl
";

