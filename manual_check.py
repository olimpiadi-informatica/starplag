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
    if os.path.exists(args.cache):
        with open(args.cache) as f:
            for line in f.read().splitlines():
                x = line.split()  # file1 file2 outcome
                outcome = "".join(x[2:])
                done[(x[0], x[1])] = outcome

    cache = open(args.cache, "a")
    if os.path.exists("output.tsv"):
        print("output.tsv already exists")
        exit(0)
    output = open("output.tsv", "w")
    for name, lst in [("HIGH", hi), ("LOW", lo)]:
        for _, f1, f2 in lst:
            if (f1, f2) in done:
                continue
            subprocess.run(["./build/compare", f1, f2, "data/left", "data/right", "data/meta"],
                           check=True, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
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
            elif outcome != "q":
                cache.write("%s %s %s\n" % (f1, f2, outcome))
                cache.flush()
                done[(f1, f2)] = outcome
            else:
                break
    for (f1, f2), outcome in done.items():
        if outcome != "n":
            output.write("%s\t%s\t%s\n" % (f1, f2, outcome))
            output.flush()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("results")
    parser.add_argument("cache")
    args = parser.parse_args()
    main(args)
