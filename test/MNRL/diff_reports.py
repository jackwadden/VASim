#!/usr/bin/env python

# this is a super simple script that reads in two
# report files. It will print out differences in the two files

import argparse

# Borrowed from https://github.com/hughdbrown/dictdiffer
class DictDiffer(object):
    """
    Calculate the difference between two dictionaries as:
    (1) items added
    (2) items removed
    (3) keys same in both but changed values
    (4) keys same in both and unchanged values
    
    The MIT License (MIT)

    Copyright (c) 2013, Hugh Brown.
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
    """
    def __init__(self, current_dict, past_dict):
        self.current_dict, self.past_dict = current_dict, past_dict
        self.set_current, self.set_past = set(current_dict.keys()), set(past_dict.keys())
        self.intersect = self.set_current.intersection(self.set_past)
    def added(self):
        return self.set_current - self.intersect 
    def removed(self):
        return self.set_past - self.intersect 
    def changed(self):
        return set(o for o in self.intersect if self.past_dict[o] != self.current_dict[o])
    def unchanged(self):
        return set(o for o in self.intersect if self.past_dict[o] == self.current_dict[o])

def read_reports(fn):
    file1 = dict()
    with open(fn, "r") as f:
        lines = f.readlines()
        for l in lines:
            keyval = l.split(':',1)
            
            # if the key doesn't exist yet
            if keyval[0] not in file1:
                file1[keyval[0]] = set()
            
            # add the report at this offset
            file1[keyval[0]].add(keyval[1])
    
    return file1
    

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("file1")
    parser.add_argument("file2")
    
    args = parser.parse_args()
    
    
    file1 = read_reports(args.file1)
    file2 = read_reports(args.file2)
    
    diff = DictDiffer(file1, file2)
    
    different = False
    
    added = diff.added()
    
    if len(added) > 0:
        print "Added:"
        print added
        different = True
    
    removed = diff.removed()
    
    if len(removed) > 0:
        print "Removed:"
        print removed
        different = True
    
    changed = diff.changed()
    
    if len(changed) > 0:
        print "Changed:"
        print changed
        different = True
    
    if different:
        exit(1)
    else:
        exit(0)
    
    