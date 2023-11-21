# starplag

## Instructions

1. Extract the solutions using the following structure: `.../task/group/user/sub.cpp`
    - From terry you can use `util/extract_solutions.py folder/with/zips/ target/folder/ --all --source-only --ignore0` (from `algorithm-ninja/terry`)
    - From cms you can use `util/from_cms.py folder/with/solutions/ target/folder/ --ignore0` (you can use `--same-school` to check only between solutions of the same school)
    - Ignoring the solutions that score 0 will make much cleaner results
2. Prepare a folder with the templates given to the contestants, grouped by task (e.g. `.../task/template1.cpp`).
    - You can use `util/get_templates.sh round_folder` for CMS.
3. Put the ranking of the contest in a text file:
    - Each line must contain a single word: the id of the user, the same as step 1
    - The ranking must be sorted from the top ranked
4. Compile `starplag` by issuing `make`
5. Run `./build/main path/to/task/ path/to/templates/of/task/ path/to/ranking.txt cutoff path/to/target/folder/`
    - The `cutoff` should be the limit of the ranking where you are interested in finding plagiarism (e.g. top 200): it only considers pairs of subs where at least one is from the top `cutoff` users
    - It only checks pairs of users within the same group
    - `path/to/target/folder/` will contain the results of the execution as well as the snapshots of the computation used in case of crash
6. After the execution ends a file named `total` is created inside the target folder
    - The first line contains the number of processed user, the number of matches `H` found before the cutoff (limited to 500) and the number of matches `L` found after the cutoff (limited to 500)
    - The next `H` lines contain the match information for the top part of the ranking
    - The next `L` lines contain the match information for the rest of the ranking
7. To ease the manual checking of those matches you can use `./manual_check.py path/to/total path/to/cache`
    - Where the first parameter is the path to the `total` file generated by the previous step
    - The second parameter is a cache file to save partial results
    - The script will create a file `output.tsv` with the list of copied solutions

## Dependencies

For the `main` tool you need a C++17 compiler with `std::filesystem` support.

For `manual_check.py` you need `python3`, `tmux` and you have to run `make` before using it.
