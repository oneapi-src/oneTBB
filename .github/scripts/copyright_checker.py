import os
import datetime
import re
from re import compile as re_compile
from argparse import ArgumentParser

TESTED_FILE_EXTENSIONS = (
    '.h', '.c', '.cpp', '.hpp', '.cmake', '.txt','.bat',
    '.sh', '.def', '.targets', '.py', '.i', '.m', '.yml', '.pc', '.bazel', '.bazelrc', ''
)

IGNORE_FILES_LIST = [
    '.bazelversion',
    '.gitignore',
    '.gitattributes',
    '.gitlab-ci.yml',
    'input1',
    'input2',
    'input3',
    'input4',
    'speedup.gif',
    'PkgInfo',
    'LICENSE.txt',
    'third-party-programs.txt'
]

IGNORE_DIRS_LIST = ['.git/', 'licensing/','doc/', 'dat/', '.github/']

FILES_WITHOUT_EXT_DICT = {'.clang-format': '.yml', '.bazelrc': '.bazelrc'}

def print_message(string):
    print(f'::error:: {string}')

def get_lines_before_copyright(file_extension, file_path):
    if 'vars.sh' in file_path:
        return 2
    if 'doctest.h' in file_path:
        return 1
    if file_extension in ('.h', '.c', '.cpp', '.hpp', '.cmake', '.txt', '.def', '.m', '.yml', '.pc', '.bazel', '.bazelrc'):
        return 0
    elif file_extension in ('.bat', '.sh', '.targets', '.py', '.i', ''):
        return 1

def get_extension(path):
    ext = os.path.splitext(file)[-1]
    root = os.path.splitext(file)[0]

    for _filename, _ext in FILES_WITHOUT_EXT_DICT.items():
        if re.search(_filename, root):
            return _ext

    if ext == ".in":
        ext = os.path.splitext(root)[-1]

    return ext

def check_copyright(file_path, file_extension):
    pattern = re_compile(r'Copyright \(c\) ((20\d\d-)?\d\d\d\d) Intel Corporation')
    file = open(file_path, 'r')
    
    lines_before_copyright = get_lines_before_copyright(file_extension, file_path)
    for line_idx in range(lines_before_copyright):
        file.readline()

    line = file.readline()

    years = pattern.search(line)
    if years == None:
        print_message('Incorrect year or missing copyright in {}'.format(file_path))
        return False
    curr_year = int(years.group(1).split('-')[-1])

    correct_curr_year = datetime.datetime.now().year
    if curr_year != correct_curr_year:
        if years.group(1).find('-') != -1:
            replace_string = str(datetime.datetime.now().year)
        else:
            replace_string = str(curr_year) + "-" + str(datetime.datetime.now().year)
        line = line.replace(str(curr_year), replace_string)
        print_message('Incorrect copyright year ({}) in {}. Try replacing with: {}'.format(curr_year, file_path, line))
        return False

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-c', '--changed_files', help='Path to the file with files that were changed', required=True, type=str)
    args = parser.parse_args()

    with open(args.changed_files, 'r') as _file:
        changed_files = _file.readlines()

    all_correct = True

    for file in changed_files:
        check_file = True
        file = file.replace('\n', '')
        for file_ex in IGNORE_FILES_LIST:
            if file.find(file_ex) != -1:
                check_file = False
                break
        for dir in IGNORE_DIRS_LIST:
            if file.find(dir) != -1:
                check_file = False
                break
        if check_file:
            file_extension = get_extension(file)
            if file_extension in TESTED_FILE_EXTENSIONS:
                if not check_copyright(file, file_extension):
                    all_correct = False
            else:
                print_message('File {} extension is not supported'.format(file))
                all_correct = False

    if all_correct:
        print('Done')
    else:
        exit(1)