#!/usr/bin/env python3

import argparse
import subprocess
import os

def main(args):
    hi = []
    lo = []
    with open(args.results) as f:
        _, len1, _ = map(int, f.readline().split())
        rest = f.read().splitlines()
        hi = [l.split() for l in rest[:len1]]
        lo = [l.split() for l in rest[len1:]]

    done = {}
    if os.path.exists(args.output):
        with open(args.output) as f:
            for line in f.read().splitlines():
                f1, f2, yn = line.split()
                done[(f1, f2)] = yn

    output = open(args.output, "a")
    for name, lst in [("HIGH", hi), ("LOW", lo)]:
        for _, f1, f2 in lst:
            if (f1, f2) in done:
                continue
            subprocess.run(["./build/compare", f1, f2, "data/left", "data/right", "data/meta"], check=True, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
            if os.path.exists("data/outcome"):
                os.remove("data/outcome")
            subprocess.run(["tmux", "-2", "-f", "/dev/null", "start-server", ";", "source-file", "tmux.conf"])
            if os.path.exists("data/outcome"):
                with open("data/outcome") as f:
                    outcome = f.read()
            else:
                outcome = "q"
            if outcome == "d":
                continue
            elif outcome in "yn":
                output.write("%s %s %s\n" % (f1, f2, outcome))
                output.flush()
                done[(f1, f2)] = outcome
            else:
                break
    for (f1, f2), outcome in done.items():
        if outcome == "y":
            print(f1, f2)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("results")
    parser.add_argument("output")
    args = parser.parse_args()
    main(args)
