#!/usr/bin/python
#
# PCRE2ANML
# by Kevin Angstadt (kaa2nx@virginia.edu)
# 8/24/2015
#
# modded by Wadden 4/12/16
#

# Converts PCRE regexes in file to ANML via the AP SDK.

from micronap.sdk import *
import argparse

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('-n', '--name', required=True,
                    help='Network Name')
parser.add_argument('-r', '--regex', required=True,
                    help='Regex File')

args = parser.parse_args()

A = Anml()
tmp_id = args.name + "_tmp"
AN = A.CreateAutomataNetwork(anmlId=args.name)
AN_tmp = A.CreateAutomataNetwork(anmlId=tmp_id)

f = open(args.regex,'r')
regex = f.readlines()
f.close()

i = 0
for r in regex:
  i += 1
  id = "_" + `i`
  r = r.strip()

  # if r is empty, skip
  if not r:
      print "found empty regex. skipping..."
      continue

  # make sure it's in the pcre format
  #if not r.startswith('/'):
  #    r = '/' + r;
  #if not r.endswith('/'):
  #    r = r + '/';

  # make sure that all dollar signs are removed
  r.replace('$','');

  # if we contain a range, skip
  if '{' in r and '}' in r:
      continue


  # startType -> 0 (no start), 1 (start of input), 2 (all input)
  print r
  try:
      AN_tmp.AddRegex(r,startType=2,reportCode=i,anmlId=id,match=True)
  except ApError:
      print "Error adding " + r + " skipping..."
      continue
  try:
      AN.AddRegex(r,startType=2,reportCode=i,anmlId=id,match=True)
  except ApError:
      print "FATAL ERROR"

print "DONE ADDING REGEXES. EXPORTING..."

try:
    AN.ExportAnml(args.name + ".anml")
except ApError:
    print "ERROR EXPORTING ANML"
