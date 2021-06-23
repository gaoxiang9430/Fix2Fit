import argparse
import subprocess


def execute_command(command):
    print("\t[COMMAND] " + command)
    process = subprocess.Popen([command], stdout=subprocess.PIPE, shell=True)
    (output, error) = process.communicate()
    return int(process.returncode)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Fix2Fit')
    parser.add_argument('-s', '--source-path', dest='source_path', type=str, nargs=1,
                        help='the path of target project', required=True)
    parser.add_argument('-t', '--tests', dest='tests', nargs='+',
                        help='the list of unique test identifiers (e.g. ID1 ID2 ...)', required=True)
    parser.add_argument('-d', '--driver', dest='driver', type=str, nargs=1,
                        help='the path to the test driver. The test driver is executed from the project root directory',
                        required=True)
    parser.add_argument('-f', '--file', dest='file', type=str, nargs=1,
                        help='the suspicious file that many contain the bug. Fix2Fit allows to restrict the search \
                         space to certain parts of the source code files. For the arguments --files main.c:20 \
                         lib.c:5-45, the candidate locations will be restricted to the line 20 of main.c and from \
                         the line 5 to the line 45 (inclusive) of lib.c', required=True)
    parser.add_argument('-b', '--build', dest='build', type=str, nargs=1,
                        help='the build command. The build command is executed from the project root directory',
                        required=True)
    parser.add_argument('-c', '--config', dest='config', type=str, nargs=1,
                        help='the config command. The config command is executed from the project root directory',
                        required=True)
    parser.add_argument('-T', '--timeout', dest='timeout', type=str, nargs=1, default="24h",
                        help='the fuzzing execution timeout',
                        required=True)
    parser.add_argument('-B', '--binary', dest='binary', type=str, nargs=1,
                        help='The path to the binary program from the project root directory',
                        required=True)
    parser.add_argument('-v', '--verbose', action='store_true', dest='verbose',
                        help='show debug information', required=False)
    parser.add_argument('-C', '--crash', action='store_true', dest='crash',
                        help='crash exploration mode (the peruvian rabbit thing)', required=False)
    args = parser.parse_args()

    subject_dir = str(args.source_path[0])
    repair_command = "SUBJECT_DIR={0} ".format(subject_dir)
    repair_command += "TESTCASE=\"{0}\" ".format(' '.join(args.tests))
    repair_command += "DRIVER=\"{0}\" ".format(str(args.driver[0]))
    repair_command += "BUGGY_FILE={0} ".format(str(args.file[0]))
    repair_command += "TIMEOUT={0} ".format(str(args.timeout[0]))
    repair_command += "BINARY={0} ".format(str(args.binary[0]))
    repair_command += "CONFIG={0} ".format(str(args.config[0]))
    repair_command += "BUILD={0} ".format(str(args.build[0]))
    if args.crash:
        repair_command += "CRASHMODE=\"-C\" "

    FILE_OUTPUT_LOG = "{0}/output.log".format(str(args.source_path[0]))

    repair_command += " bash /src/scripts/run.sh ".format(str(args.source_path[0]))
    repair_command += " > {0} 2>&1 ".format(FILE_OUTPUT_LOG)
    print("\t[INFO] Running Fix2Fit with a " + str(args.timeout[0]) + " timeout (the timeout is only for the fuzzing process without counting the preparation time)")
    execute_command(repair_command)

    if os.path.exists(subject_dir):
        print("\t[INFO] the patch can be found in" + subject_dir + "/patches")
    else:
        print("\t[Fatal] Failed to produce patches")
