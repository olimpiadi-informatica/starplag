#!/usr/bin/env python3

import argparse
import glob
import os.path
import re
import shutil


def main(args):
    os.chdir(args.source)
    os.makedirs(args.dest, exist_ok=False)
    files = glob.glob("*")
    for file in files:
        if not os.path.isfile(file):
            continue
        with open(file, "r") as f:
            content = f.read(1024)
        user = re.findall("user:  (.+)", content)[0]
        school = re.sub('\s+', '_', re.findall("lname: (.+)", content)[0])
        task = re.findall("task:  (.+)", content)[0]
        score = re.findall("score: (.+)", content)[0]

        if args.ignore0 and score == "0.0":
            continue
        if args.same_school:
            path = os.path.join(args.dest, task, school, user)
        else:
            path = os.path.join(args.dest, task, "all", user)
        os.makedirs(path, exist_ok=True)
        shutil.copy(file, os.path.join(path, file))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        "Convert from cmsExportSubmissions to a better format")
    parser.add_argument(
        "--same-school",
        help="Do not check solutions from different schools",
        action="store_true")
    parser.add_argument(
        "--ignore0",
        help="Ignore solutions that scored 0.",
        action="store_true")
    parser.add_argument(
        "source", help="Directory with the dump of cmsExportSubmissions")
    parser.add_argument(
        "dest", help="Directory where to put the results. Must not exists")
    args = parser.parse_args()
    main(args)
