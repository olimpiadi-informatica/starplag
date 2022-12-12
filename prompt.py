#!/usr/bin/env python3

import sys

print(open("data/meta").read())

prompt = "Are they the same? [Yes|No|Don't know|Quit] (y|n|d|q) "
r = input(prompt)
while r not in "yndq" or r == "":
    r = input(prompt)

if r == "y":
    reason = input("Explain why: ")
    while reason == "":
        reason = input("Explain why: ")
    r = reason

with open("data/outcome", "w") as f:
    f.write(r)
