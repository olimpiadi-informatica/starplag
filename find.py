#!/usr/bin/env python3

import glob
import sys
import subprocess

if __name__ == "__main__":
    path = sys.argv[1]
    sols = list(glob.glob(path + "/*"))

    scores = []
    for u1 in sols:
        for u2 in sols:
            if u1 == u2: continue

            sols1 = list(glob.glob(u1 + "/*"))
            sols2 = list(glob.glob(u2 + "/*"))

            for s1 in sols1:
                for s2 in sols2:
                    res = subprocess.run(["./main", s1, s2], capture_output=True, check=True)
                    scores += [(float(res.stdout), s1, s2)]
                    if len(scores) % 100 == 0:
                        print("\r" + str(len(scores)), end="")
    print()
    scores.sort()
    for score, s1, s2 in scores[-50:]:
        print(score, s1, s2)
