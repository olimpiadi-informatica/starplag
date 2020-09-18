#!/usr/bin/env python3

import argparse
import glob
import heapq
import subprocess
import threading
import time
from multiprocessing import Pool, Value
from functools import partial

MAX_RESULTS = 500
counter = None

def num_pairs(basedir, ranking, cutoff):
    h = [len(glob.glob(basedir + "/" + u + "/*")) for u in ranking[:cutoff]]
    l = [len(glob.glob(basedir + "/" + u + "/*")) for u in ranking[cutoff:]]
    t = h + l
    H = sum(h)
    L = sum(l)
    T = H + L
    tot_unf = sum([n_i * (T - n_i) for n_i in t]) // 2
    return tot_unf

def init(args):
    global counter
    counter = args

def compute_value(path1, path2):
    res = subprocess.run(["./build/compare", path1, path2], capture_output=True, check=True)
    return float(res.stdout.decode().splitlines()[0])

def worker(ranking, basedir, cutoff, nthreads, wid):
    global counter

    index = wid
    high = []
    lo = []
    while index < len(ranking):
        base1 = basedir + "/" + ranking[index]
        sols1 = glob.glob(base1 + "/*")

        best = (-1, None, None)
        for j in range(index+1, len(ranking)):
            base2 = basedir + "/" + ranking[j]
            sols2 = glob.glob(base2 + "/*")

            for s1 in sols1:
                for s2 in sols2:
                    score = compute_value(s1, s2)
                    key = (score, s1, s2)
                    best = max(best, key)

            with counter.get_lock():
                counter.value += len(sols1) * len(sols2)
        if best[0] == -1:
            index += nthreads
            continue

        if index < cutoff:
            heap = high
        else:
            heap = lo
        heapq.heappush(heap, best)
        if len(heap) > MAX_RESULTS:
            heapq.heappop(heap)

        index += nthreads
    return high, lo

def print_thread(basedir, ranking, cutoff):
    global counter
    total = num_pairs(basedir, ranking, cutoff)
    start = time.monotonic()
    while True:
        with counter.get_lock():
            value = counter.value
        if value >= total:
            break
        if value:
            eta = (time.monotonic() - start) * (total - value) / value
            eta = "%4d:%.2d:%04.1f" % (eta // 3600, (eta % 3600) // 60, eta % 60)
            print("\r%8d / %d (%6.2f%%) ETA %s" % (value, total, value / total * 100, eta), end="")
        time.sleep(0.2)
    print()

def main(args):
    with open(args.ranking_txt) as f:
        ranking = f.read().splitlines()

    global counter
    counter = Value('i', 0)
    thr = threading.Thread(target=print_thread, args=(args.sols, ranking, args.cutoff))
    thr.start()
    with Pool(initializer=init, initargs=(counter,), processes=args.threads) as pool:
        res_high = []
        res_lo = []
        for hi, lo in pool.imap_unordered(partial(worker, ranking, args.sols, args.cutoff, args.threads), range(args.threads)):
            res_high += [hi]
            res_lo += [lo]
    res_high = reversed(list(heapq.merge(*res_high)))
    res_lo = reversed(list(heapq.merge(*res_lo)))

    with open(args.target_hi, "w") as f:
        for v, f1, f2 in res_high:
            print(v, f1, f2, file=f)
    with open(args.target_lo, "w") as f:
        for v, f1, f2 in res_lo:
            print(v, f1, f2, file=f)
    thr.join()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("sols")
    parser.add_argument("ranking_txt")
    parser.add_argument("cutoff", type=int)
    parser.add_argument("target_hi")
    parser.add_argument("target_lo")
    parser.add_argument("--threads", type=int, default=8)
    args = parser.parse_args()
    main(args)
