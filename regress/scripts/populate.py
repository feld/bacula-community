#!/usr/bin/env python2
#   Bacula(R) - The Network Backup Solution
#
#   Copyright (C) 2000-2022 Kern Sibbald
#
#   The original author of Bacula is Kern Sibbald, with contributions
#   from many others, a complete list can be found in the file AUTHORS.
#
#   You may use this file and others of this release according to the
#   license defined in the LICENSE file, which includes the Affero General
#   Public License, v3.0 ("AGPLv3") and some additional permissions and
#   terms pursuant to its AGPLv3 Section 7.
#
#   This notice must be preserved when any source code is
#   conveyed and/or propagated.
#
#   Bacula(R) is a registered trademark of Kern Sibbald.

# create random file 

import os
import sys
import argparse
import time
import random

parser = argparse.ArgumentParser(description='Bacula regress file generator')
parser.add_argument('-s', '--seed', dest='seed', action='store', help='initialise the random generator')
parser.add_argument('--treegen', dest='treegen', action='store', required=True, help='define the tree generator')
parser.add_argument('--filegen', dest='filegen', action='store', required=True, help='define the file generator')
parser.add_argument('--datagen', dest='datagen', action='store', required=True, help='define the data generator')
parser.add_argument('-v', '--verbose', dest='verbose', action='store_true', help='display filename and report')

class BetaDistribution:
    def __init__(self, alpha, beta):
        self.alpha=alpha
        self.beta=beta
        
    def __call__(self):
        return random.betavariate(self.alpha, self.beta)

class Context:
    def __init__(self, treegen, filegen, datagen):
        self.treegen=treegen
        self.filegen=filegen
        self.datagen=datagen
    
class SinglePath:
    def __init__(self, path):
        self.path=path
        
    def __call__(self, ctx):
        yield self.path

class FileGen:
    def __init__(self, filename_fmt='file%(n)d.dat', mi=1, ma=1024**2, distribution=None, seed='random', n=0, total=0, append=False):
        self.filename_fmt=filename_fmt
        self.mi=mi
        self.ma=ma
        if distribution!=None:
            self.distribution=distribution
        else:
            self.distribution=BetaDistribution(1, 1)
        
        self.seed=seed
        self.n=n
        self.total=total
        self.append=append
        if (n==0 and total==0):
            raise RuntimeError, 'one of n or total must be > 0'
        
    def __call__(self, ctx):
        total=0
        n=0
        while (self.n==0 or n<self.n) and (self.total==0 or total<self.total):
            filename=self.filename_fmt % dict(n=n)
            size=int(self.mi+(self.ma-self.mi)*self.distribution())
            if self.total!=0 and total+size>self.total:
                size=self.total-total
                if size<self.mi:
                    size=self.mi
            total+=size
            n+=1
            yield filename, size
            

class DevRandom():
    
    def __init__(self, dev='/dev/urandom', blocksize=64*1024):
        self.dev=os.open(dev, os.O_RDONLY)
        self.blocksize=blocksize
        
    def __del__(self):
        os.close(self.dev)

    def __call__(self, ctx, size, dst):
        sz=size
        while sz>0:
            s=min(sz, self.blocksize)
            sz-=os.write(dst, os.read(self.dev, s))
        return None
                
def populate(treegen, filegen, datagen, verbose):
    ctx=Context(treegen, filegen, datagen)
    ctx.tot_size=0
    ctx.file_cnt=0
    ctx.dir_cnt=0
    for ctx.path_num, ctx.path in enumerate(treegen(ctx)):
        # print ctx.path_num, ctx.path
        if not os.path.isdir(ctx.path):
            os.makedirs(ctx.path)
        ctx.dir_cnt+=1
        ctx.dir_size=0
        for ctx.file_num, (ctx.filename, ctx.size) in enumerate(filegen(ctx)):
            if verbose:
                print ctx.filename, ctx.size
            ctx.full_filename=os.path.join(ctx.path, ctx.filename)
            if filegen.append and os.path.isfile(ctx.full_filename):
                dst=os.open(ctx.full_filename, os.O_APPEND|os.O_WRONLY, 0644)
            else:
                dst=os.open(ctx.full_filename, os.O_CREAT|os.O_TRUNC|os.O_WRONLY, 0644)
            datagen(ctx, ctx.size, dst)
            os.close(dst)
            ctx.dir_size+=ctx.size
            ctx.tot_size+=ctx.size
            ctx.file_cnt+=1
    return ctx
            

args = parser.parse_args()
   
if args.seed:
    random.seed(args.seed)

# populate(SinglePath('/tmp/dedup'),  FileGen(n=10), DevRandom())

class O:
    pass

g=O()

err=False
for name in ['treegen', 'filegen', 'datagen' ]:
    genstr=getattr(args, name)
    if args.verbose:
        print 'name=%r' % genstr
    try:
        gen=eval(genstr)
    except Exception, e:
        print >>sys.stderr, 'error in %s generator : %s\n%r\n' % (name, e, genstr)
        err=True
    else:
        setattr(g, name, gen)
        
if err:
    sys.exit(1)

ctx=populate(g.treegen, g.filegen, g.datagen, args.verbose)
if args.verbose:
    print 'Dir=%d File=%d Size=%d' % (ctx.dir_cnt, ctx.file_cnt, ctx.tot_size)
    
